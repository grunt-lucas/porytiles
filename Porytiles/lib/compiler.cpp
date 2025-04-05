#include "compiler.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include <algorithm>
#include <bitset>
#include <deque>
#include <filesystem>
#include <memory>
#include <png.hpp>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "emitter.h"
#include "errors_warnings.h"
#include "importer.h"
#include "logger.h"
#include "palette_assignment.h"
#include "porytiles_context.h"
#include "porytiles_exception.h"
#include "types.h"

namespace porytiles {

/**
 * Insert an rgba32 color into a normalized palette. The color will be converted to bgr15 format in the process,
 * and possibly deduped (depending on user settings). Transparent alpha pixels will be treated as transparent, as
 * will pixels that are of transparent color (again, set by the user but default to magenta). Fails if a tile
 * contains too many unique colors or if an invalid alpha value is detected.
 */
static std::size_t insertRGBA(PorytilesContext &ctx, const CompilerMode compilerMode, const RGBATile &rgbaFrame,
                              const RGBA32 &transparencyColor, NormalizedPalette &palette, const RGBA32 &rgba,
                              std::size_t row, std::size_t col, const bool errWarn) {
    const auto transparencyBgr = rgbaToBgr(transparencyColor);
    const auto pixelBgr = rgbaToBgr(rgba);

    /*
     * Porytiles does not allow users to specify an alpha value that isn't fully opaque or transparent.
     */
    if (rgba.alpha != ALPHA_TRANSPARENT && rgba.alpha != ALPHA_OPAQUE) {
        if (errWarn) {
            error_invalidAlphaValue(ctx.err, rgbaFrame, rgba.alpha, row, col);
        }
        return INVALID_INDEX_PIXEL_VALUE;
    }

    if (!rgba.equalsIgnoringAlphaChannel(transparencyColor) && pixelBgr == transparencyBgr && errWarn) {
        /*
         * Porytiles has gone through a few iterations of behavior when it hits this case. Originally, this was a
         * compilation error. Later, it became a warning that would not stop compilation, but would insert the bgr into
         * the regular colors section of the NormalizedPalette. However, that behavior made for an extremely unintuitive
         * user experience. We have settled on this behavior: if a pixel collapses to transparent under BGR conversion,
         * then treat it as transparent but warn the user in case this was not their intention. This is the best option
         * because it retains end-user visibility while not potentially causing confounding compilation errors later on
         * in the compilation pipeline.
         */
        ctx.diag->report(W_TRANSPARENCY_COLLAPSE, ctx.diag->bold(rgba.jasc()),
                         ctx.diag->bold(compilerModeString(compilerMode)), ctx.diag->bold(rgbaFrame.prettify()),
                         ctx.diag->bold(col), ctx.diag->bold(row));
        ctx.diag->report_partner(W_TRANSPARENCY_COLLAPSE, 0);
        return 0;
    }

    if (rgba.alpha == ALPHA_TRANSPARENT || rgba == transparencyColor) {
        return 0;
    }

    if (rgba.alpha == ALPHA_OPAQUE) {
        if (ctx.compilerContext.bgrToRgba.contains(pixelBgr) &&
            std::get<0>(ctx.compilerContext.bgrToRgba.at(pixelBgr)) != rgba) {
            /*
             * We lost color precision here, so let's warn the user that two distinct RGBA colors they used
             * in the master sheet are going to collapse to one BGR color on the GBA.
             */
            if (errWarn) {
                // TODO : we can probably pass rgbaFrame here as a pointer
                ctx.diag->report(W_COLOR_PRECISION_LOSS, rgbaFrame, rgba.jasc(), compilerModeString(compilerMode), row,
                                 col);
                ctx.diag->report_partner(W_COLOR_PRECISION_LOSS, 0,
                                         std::get<1>(ctx.compilerContext.bgrToRgba.at(pixelBgr)),
                                         std::get<0>(ctx.compilerContext.bgrToRgba.at(pixelBgr)).jasc(),
                                         std::get<2>(ctx.compilerContext.bgrToRgba.at(pixelBgr)),
                                         std::get<3>(ctx.compilerContext.bgrToRgba.at(pixelBgr)));
                ctx.compilerContext.bgrToRgba.at(pixelBgr) = std::tuple{rgba, rgbaFrame, row, col};
            }
        }
        if (errWarn) {
            // Only update the map for the non-flipped version of each tile
            ctx.compilerContext.bgrToRgba.insert_or_assign(pixelBgr, std::tuple{rgba, rgbaFrame, row, col});
        }

        const auto itrAtBgr =
            std::find(std::begin(palette.colors) + 1, std::begin(palette.colors) + palette.size, pixelBgr);
        const auto bgrPosInPalette = itrAtBgr - std::begin(palette.colors);
        if (bgrPosInPalette == palette.size) {
            // palette size will grow as we add to it
            if (palette.size == PAL_SIZE) {
                if (errWarn) {
                    error_tooManyUniqueColorsInTile(ctx.err, rgbaFrame, row, col);
                }
                return INVALID_INDEX_PIXEL_VALUE;
            }
            palette.colors.at(palette.size++) = pixelBgr;
        }
        return bgrPosInPalette;
    }

    internalerror("compiler::insertRGBA unreachable code path");
    return INVALID_INDEX_PIXEL_VALUE;
}

static NormalizedTile candidate(PorytilesContext &ctx, const CompilerMode compilerMode, const RGBA32 &transparencyColor,
                                const std::vector<RGBATile> &rgbaFrames, const bool hFlip, const bool vFlip,
                                const bool errWarn) {
    /*
     * NOTE: This only produces a _candidate_ normalized tile (a different choice of hFlip/vFlip might be the normal
     * form). We'll use this to generate candidates to find the true normal form.
     */
    NormalizedTile candidateTile{transparencyColor};
    candidateTile.hFlip = hFlip;
    candidateTile.vFlip = vFlip;
    candidateTile.frames.resize(rgbaFrames.size());

    std::size_t frame = 0;
    for (const auto &rgba : rgbaFrames) {
        for (std::size_t row = 0; row < TILE_SIDE_LENGTH_PIX; row++) {
            for (std::size_t col = 0; col < TILE_SIDE_LENGTH_PIX; col++) {
                const std::size_t rowWithFlip = vFlip ? TILE_SIDE_LENGTH_PIX - 1 - row : row;
                const std::size_t colWithFlip = hFlip ? TILE_SIDE_LENGTH_PIX - 1 - col : col;
                const std::size_t pixelValue =
                    insertRGBA(ctx, compilerMode, rgba, transparencyColor, candidateTile.palette,
                               rgba.getPixel(rowWithFlip, colWithFlip), row, col, errWarn);
                candidateTile.setPixel(frame, row, col, pixelValue);
            }
        }
        frame++;
    }

    return candidateTile;
}

static NormalizedTile normalize(PorytilesContext &ctx, CompilerMode compilerMode,
                                const std::vector<RGBATile> &rgbaFrames) {
    /*
     * Normalize the given tile by checking each of the 4 possible flip states, and choosing the one that comes first in
     * "lexicographic" order, where this order is determined by the std::array spaceship operator.
     */
    auto noFlipsTile =
        candidate(ctx, compilerMode, ctx.compilerConfig.transparencyColor, rgbaFrames, false, false, true);

    // Short-circuit because transparent tiles are common in metatiles and trivially in normal form.
    if (noFlipsTile.transparent()) {
        if (rgbaFrames.at(0).type == TileType::LAYERED) {
            pt_logln(ctx, stderr, "{}:{}:{} = transparent", layerString(rgbaFrames.at(0).layer),
                     rgbaFrames.at(0).metatileIndex, subtileString(rgbaFrames.at(0).subtile));
        }
        return noFlipsTile;
    }

    auto hFlipTile = candidate(ctx, compilerMode, ctx.compilerConfig.transparencyColor, rgbaFrames, true, false, false);
    auto vFlipTile = candidate(ctx, compilerMode, ctx.compilerConfig.transparencyColor, rgbaFrames, false, true, false);
    auto bothFlipsTile =
        candidate(ctx, compilerMode, ctx.compilerConfig.transparencyColor, rgbaFrames, true, true, false);

    std::array<const NormalizedTile *, 4> candidates = {&noFlipsTile, &hFlipTile, &vFlipTile, &bothFlipsTile};
    auto normalizedTile = std::min_element(std::begin(candidates), std::end(candidates), [](auto tile1, auto tile2) {
        return tile1->keyFrame() < tile2->keyFrame();
    });

    if (rgbaFrames.at(0).type == TileType::LAYERED) {
        pt_logln(ctx, stderr, "{}:{}:{} = [hFlip: {}, vFlip: {}]", layerString(rgbaFrames.at(0).layer),
                 rgbaFrames.at(0).metatileIndex, subtileString(rgbaFrames.at(0).subtile), (*normalizedTile)->hFlip,
                 (*normalizedTile)->vFlip);
    }

    return **normalizedTile;
}

static std::tuple<std::vector<IndexAndNormTile>, std::vector<NormalizedTile>, std::vector<NormalizedTile>>
normalizeDecompTiles(PorytilesContext &ctx, CompilerMode compilerMode, const DecompiledTileset &decompiledTileset,
                     const std::vector<RGBATile> &palettePrimers, const std::vector<RGBATile> &paletteOverrides) {
    /*
     * For each tile in the decomp tileset, normalize it and tag it with its index in the decomp tileset. We tag the
     * animated tiles first, then tag the regular assignment tiles. Finally, we tag the primers and the overrides.
     */
    std::vector<IndexAndNormTile> normalizedTiles{};
    std::vector<NormalizedTile> normalizedPrimers{};
    std::vector<NormalizedTile> normalizedOverrides{};

    std::unordered_set<RGBA32> primerRgbColors{};
    std::unordered_map<RGBA32, std::vector<std::string>> primerRgbColorPaths{};
    std::unordered_set<RGBA32> overrideRgbColors{};
    std::unordered_map<RGBA32, std::vector<std::string>> overrideRgbColorPaths{};
    std::unordered_set<RGBA32> layerAndAnimRgbColors{};

    // Load palette primers first
    for (const auto &primerTile : palettePrimers) {
        for (std::size_t i = 0; i < primerTile.primerSize; i++) {
            const auto &rgb = primerTile.pixels.at(i);
            primerRgbColors.insert(rgb);
            if (!primerRgbColorPaths.contains(rgb)) {
                std::vector path = {primerTile.primerFilename};
                primerRgbColorPaths.insert({rgb, path});
            } else {
                primerRgbColorPaths.at(rgb).push_back(primerTile.primerFilename);
            }
        }

        std::vector singleFramePrimerTile = {primerTile};
        auto normalizedPrimerTile = normalize(ctx, compilerMode, singleFramePrimerTile);
        normalizedPrimerTile.copyMetadataFrom(primerTile);
        normalizedPrimers.emplace_back(normalizedPrimerTile);
    }

    // Then load palette overrides
    for (const auto &overrideTile : paletteOverrides) {
        for (std::size_t i = 0; i < overrideTile.overrideSize; i++) {
            const auto &rgb = overrideTile.pixels.at(i);
            overrideRgbColors.insert(rgb);
            if (!overrideRgbColorPaths.contains(rgb)) {
                std::vector path = {overrideTile.overrideFilename};
                overrideRgbColorPaths.insert({rgb, path});
            } else {
                overrideRgbColorPaths.at(rgb).push_back(overrideTile.overrideFilename);
            }
        }

        std::vector singleFrameOverrideTile = {overrideTile};
        auto normalizedOverrideTile = normalize(ctx, compilerMode, singleFrameOverrideTile);
        normalizedOverrideTile.copyMetadataFrom(overrideTile);
        normalizedOverrides.emplace_back(normalizedOverrideTile);
    }

    // Then load animations
    for (std::size_t animIndex = 0; animIndex < decompiledTileset.anims.size(); animIndex++) {
        const auto &anim = decompiledTileset.anims.at(animIndex);
        // We have already validated that all frames have identical dimensions, so we can use the key frame here
        for (std::size_t tileIndex = 0; tileIndex < anim.keyFrame().size(); tileIndex++) {
            std::vector<RGBATile> multiFrameTile{};
            // For each tile, push all frames of the tile into a vector
            multiFrameTile.reserve(anim.size());
            for (std::size_t frameIndex = 0; frameIndex < anim.size(); frameIndex++) {
                const auto &tile = anim.frames.at(frameIndex).tiles.at(tileIndex);
                for (const auto &rgb : tile.pixels) {
                    layerAndAnimRgbColors.insert(rgb);
                }
                multiFrameTile.push_back(tile);
            }
            DecompiledIndex index{};
            auto normalizedTile = normalize(ctx, compilerMode, multiFrameTile);
            normalizedTile.copyMetadataFrom(multiFrameTile.at(0));
            index.animated = true;
            index.animIndex = animIndex;
            index.tileIndex = tileIndex;
            normalizedTiles.emplace_back(index, normalizedTile);
        }
    }

    // Finally load regular layer PNG tiles
    std::size_t tileIndex = 0;
    for (const auto &tile : decompiledTileset.tiles) {
        for (const auto &rgb : tile.pixels) {
            layerAndAnimRgbColors.insert(rgb);
        }

        std::vector singleFrameTile = {tile};
        auto normalizedTile = normalize(ctx, compilerMode, singleFrameTile);
        normalizedTile.copyMetadataFrom(tile);
        DecompiledIndex index{};
        index.tileIndex = tileIndex++;
        normalizedTiles.emplace_back(index, normalizedTile);
    }

    /*
     * Here we warn the user if there were RGB colors in their primers/overrides that weren't actually present in any
     * anim or layer tile. This usually indicates a user error.
     */
    for (const auto &elem : layerAndAnimRgbColors) {
        primerRgbColors.erase(elem);
    }
    for (const auto &elem : layerAndAnimRgbColors) {
        overrideRgbColors.erase(elem);
    }
    for (const auto &remainingRgb : primerRgbColors) {
        for (const auto &path : primerRgbColorPaths.at(remainingRgb)) {
            ctx.diag->report(W_UNUSED_MANUAL_PAL_COLOR, ctx.diag->bold(path), ctx.diag->bold(remainingRgb.jasc()));
        }
    }
    for (const auto &remainingRgb : overrideRgbColors) {
        for (const auto &path : overrideRgbColorPaths.at(remainingRgb)) {
            ctx.diag->report(W_UNUSED_MANUAL_PAL_COLOR, ctx.diag->bold(path), ctx.diag->bold(remainingRgb.jasc()));
        }
    }

    if (ctx.err.errCount > 0 || ctx.diag->in_flight_count_for_level(diag_level::error) > 0) {
        die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       "errors generated during tile normalization");
    }

    return std::tuple{normalizedTiles, normalizedPrimers, normalizedOverrides};
}

static std::pair<std::unordered_map<BGR15, std::size_t>, std::unordered_map<std::size_t, BGR15>>
buildColorIndexMaps(const PorytilesContext &ctx, const CompilerMode compilerMode,
                    const std::vector<IndexAndNormTile> &normalizedTiles,
                    const std::unordered_map<BGR15, std::size_t> &primaryIndexMap,
                    const std::vector<NormalizedTile> &primerTiles, const std::vector<NormalizedTile> &overrideTiles) {
    /*
     * Iterate over every color in each tile's NormalizedPalette, adding it to the map if not already present. We end up
     * with a map of colors to unique indexes. Optionally, we will populate the map with colors from the paired primary
     * set so that secondary tiles can possibly make use of these palettes without doubling up colors.
     */
    std::unordered_map<BGR15, std::size_t> colorIndexes;
    std::unordered_map<std::size_t, BGR15> indexesToColors;
    if (!primaryIndexMap.empty()) {
        for (const auto &[color, index] : primaryIndexMap) {
            if (auto [insertedValue, wasInserted] = colorIndexes.insert({color, index}); !wasInserted) {
                internalerror("compiler::buildColorIndexMaps colorIndexes.insert failed");
            }
            if (auto [_, wasInserted2] = indexesToColors.insert(std::pair{index, color}); !wasInserted2) {
                internalerror("compiler::buildColorIndexMaps indexesToColors.insert failed");
            }
        }
    }

    // TODO : try using C++20's std::views here
    std::size_t colorIndex = primaryIndexMap.size();
    for (const auto &[_, normalizedTile] : normalizedTiles) {
        // i starts at 1, since first color in each palette is the transparency color
        for (int i = 1; i < normalizedTile.palette.size; i++) {
            if (const BGR15 &color = normalizedTile.palette.colors[i];
                colorIndexes.insert(std::pair{color, colorIndex}).second) {
                indexesToColors.insert(std::pair{colorIndex, color});
                colorIndex++;
            }
        }
    }

    /*
     * We shouldn't actually hit the inner map insert for either of these. The only time we would hit the insert is when
     * the user specified a primer or override that wasn't used in their layer or anim tiles. In that case, they'll see
     * a Wunused-manual-pal-color warning if they had it enabled. The reason we don't throw the Wunused-manual-pal-color
     * from here directly is that by the time we get here, we've lost the original RGBA color. We want to report that to
     * the user for clarity, so we do some extra work to throw the warning earlier.
     */
    for (const auto &normalizedTile : primerTiles) {
        for (int i = 1; i < normalizedTile.palette.size; i++) {
            if (const BGR15 &color = normalizedTile.palette.colors[i];
                colorIndexes.insert(std::pair{color, colorIndex}).second) {
                indexesToColors.insert(std::pair{colorIndex, color});
                colorIndex++;
            }
        }
    }
    for (const auto &normalizedTile : overrideTiles) {
        for (int i = 1; i < normalizedTile.palette.size; i++) {
            if (const BGR15 &color = normalizedTile.palette.colors[i];
                colorIndexes.insert(std::pair{color, colorIndex}).second) {
                indexesToColors.insert(std::pair{colorIndex, color});
                colorIndex++;
            }
        }
    }

    /*
     * This error is merely a fail-early heuristic. I.e. just because a primary tileset passes this check does not mean
     * it is actually allocatable.
     */
    if (compilerMode == CompilerMode::PRIMARY) {
        if (const std::size_t size = (PAL_SIZE - 1) * ctx.fieldmapConfig.numPalettesInPrimary; colorIndex > size) {
            fatalerror_tooManyUniqueColorsTotal(ctx.err, ctx.compilerSrcPaths, compilerMode, size, colorIndex);
        }
    } else if (compilerMode == CompilerMode::SECONDARY) {
        // use numPalettesTotal since secondary tiles can use colors from the primary set
        if (const std::size_t size = (PAL_SIZE - 1) * ctx.fieldmapConfig.numPalettesTotal; colorIndex > size) {
            fatalerror_tooManyUniqueColorsTotal(ctx.err, ctx.compilerSrcPaths, compilerMode, size, colorIndex);
        }
    } else {
        internalerror_unknownCompilerMode("compiler::buildColorIndexMaps");
    }

    return {colorIndexes, indexesToColors};
}

static ColorSet toColorSet(const std::unordered_map<BGR15, std::size_t> &colorIndexMap, const NormalizedTile &tile) {
    /*
     * Set a color set based on a given palette. Each bit in the ColorSet represents if the color at the given index in
     * the supplied color map was present in the palette. E.g. suppose the color map has 12 unique colors. The supplied
     * palette has two colors in it, which correspond to index 2 and index 11. The ColorSet bitset would be:
     * 0010 0000 0001
     */
    ColorSet colorSet{};
    // starts at 1, skip the transparent color at slot 0 in the normalized palette
    for (int i = 1; i < tile.palette.size; i++) {
        colorSet.first.set(colorIndexMap.at(tile.palette.colors.at(i)));
    }
    colorSet.second = tile.overridePaletteIndex;
    return colorSet;
}

static std::tuple<std::vector<IndexedNormTileWithColorSet>, std::vector<ColorSet>, std::vector<ColorSet>,
                  std::vector<ColorSet>>
matchNormalizedWithColorSets(const std::unordered_map<BGR15, std::size_t> &colorIndexMap,
                             const std::vector<IndexAndNormTile> &indexedNormalizedTiles,
                             const std::vector<NormalizedTile> &normalizedPrimers,
                             const std::vector<NormalizedTile> &normalizedOverrides) {
    std::vector<IndexedNormTileWithColorSet> indexedNormTilesWithColorSets{};
    std::unordered_set<ColorSet> uniqueColorSets{};
    std::vector<ColorSet> colorSets{};
    std::unordered_set<ColorSet> uniquePrimerColorSets{};
    std::vector<ColorSet> primerColorSets{};

    /*
     * We don't have a uniqueness set for overrides. This is because we allow users to specify non-unique palette
     * overrides if they really want. E.g. 01.pal and 03.pal could have identical overridden colors, just in different
     * pal slots. I don't see a reason users would ever need to do this, but it's not logically incoherent and thus
     * allowed.
     */
    // TODO : Perhaps at some point we should disallow the above and throw an error? I could see it being confusing
    std::vector<ColorSet> overrideColorSets{};

    for (const auto &[index, normalizedTile] : indexedNormalizedTiles) {
        // Compute the ColorSet for this normalized tile, then add it to our indexes
        auto colorSet = toColorSet(colorIndexMap, normalizedTile);
        indexedNormTilesWithColorSets.emplace_back(index, normalizedTile, colorSet);
        if (!uniqueColorSets.contains(colorSet)) {
            colorSets.push_back(colorSet);
            uniqueColorSets.insert(colorSet);
        }
    }

    // Special primer ColorSets
    for (const auto &normalizedPrimerTile : normalizedPrimers) {
        // Compute the ColorSet for this normalized tile, then add it to our indexes
        auto colorSet = toColorSet(colorIndexMap, normalizedPrimerTile);
        if (!uniquePrimerColorSets.contains(colorSet)) {
            primerColorSets.push_back(colorSet);
            uniquePrimerColorSets.insert(colorSet);
        }
    }

    // Special override ColorSets
    for (const auto &normalizedOverrideTile : normalizedOverrides) {
        // Compute the ColorSet for this normalized tile, then add it to our indexes
        auto colorSet = toColorSet(colorIndexMap, normalizedOverrideTile);
        overrideColorSets.push_back(colorSet);
    }

    return std::tuple{indexedNormTilesWithColorSets, colorSets, primerColorSets, overrideColorSets};
}

static GBATile makeTile(const NormalizedTile &normalizedTile, const std::size_t frame, const GBAPalette &palette) {
    GBATile gbaTile{};
    std::array<std::uint8_t, PAL_SIZE> paletteIndexes{};
    paletteIndexes.at(0) = 0;
    for (int i = 1; i < normalizedTile.palette.size; i++) {
        const auto it =
            std::find(std::begin(palette.colors) + 1, std::end(palette.colors), normalizedTile.palette.colors[i]);
        if (it == std::end(palette.colors)) {
            internalerror(fmt::format("compiler::makeTile it == std::end(palette.colors) for color {}",
                                      bgrToRgba(normalizedTile.palette.colors[i]).jasc()));
        }
        paletteIndexes.at(i) = it - std::begin(palette.colors);
    }

    for (std::size_t i = 0; i < normalizedTile.frames.at(frame).colorIndexes.size(); i++) {
        gbaTile.colorIndexes.at(i) = paletteIndexes.at(normalizedTile.frames.at(frame).colorIndexes.at(i));
    }
    return gbaTile;
}

static void assignTilesPrimary(PorytilesContext &ctx, CompiledTileset &compiled,
                               const std::vector<IndexedNormTileWithColorSet> &indexedNormTilesWithColorSets,
                               const std::vector<ColorSet> &assignedPalsSolution) {
    std::unordered_map<GBATile, std::size_t> tileIndexes{};
    std::unordered_map<GBATile, bool> usedKeyFrameTiles{};

    // force tile 0 to be a transparent tile that uses palette 0
    tileIndexes.insert({GBA_TILE_TRANSPARENT, 0});
    compiled.tiles.push_back(GBA_TILE_TRANSPARENT);
    compiled.paletteIndexesOfTile.push_back(0);

    /*
     * Process animated tiles, we want frame 0 of each animation to be at the beginning of the tiles.png in a stable
     * location.
     */
    for (const auto &indexedNormTile : indexedNormTilesWithColorSets) {
        auto index = std::get<0>(indexedNormTile);
        auto &normTile = std::get<1>(indexedNormTile);
        auto &colorSet = std::get<2>(indexedNormTile);

        // Skip regular tiles, since we will process them next
        if (!index.animated) {
            continue;
        }

        pt_logln(ctx, stderr, "found anim tile (frame count = {}) for anim={}, tile={}", normTile.frames.size(),
                 index.animIndex, index.tileIndex);
        auto it = std::find_if(std::begin(assignedPalsSolution), std::end(assignedPalsSolution),
                               [&colorSet](const auto &assignedPal) {
                                   // Find which of the assignedSolution palettes this tile belongs to
                                   return (colorSet.first & ~assignedPal.first).none();
                               });
        if (it == std::end(assignedPalsSolution)) {
            internalerror("compiler::assignTilesPrimary it == std::end(assignedPalsSolution)");
        }
        std::size_t paletteIndex = it - std::begin(assignedPalsSolution);

        // Create the GBATile for this tile's key frame
        GBATile keyFrameTile = makeTile(normTile, NormalizedTile::keyFrameIndex(), compiled.palettes.at(paletteIndex));

        if (tileIndexes.contains(keyFrameTile) && tileIndexes.at(keyFrameTile) == 0) {
            /*
             * Fatal error if the user provided a transparent key frame tile. This is not allowed, since there would be
             * no way to tell if a user provided tile on the layer sheet referred to the true index 0 transparent tile,
             * or if it was a reference into this particular animation.
             */
            fatalerror_transparentKeyFrameTile(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY, normTile.anim,
                                               normTile.tileIndex);
        }

        // Insert this tile's key frame into the seen tiles map
        auto inserted = tileIndexes.insert({keyFrameTile, compiled.tiles.size()});

        // Insertion happened
        if (inserted.second) {
            // Insert this tile's key frame into the tiles.png
            compiled.tiles.push_back(keyFrameTile);
            compiled.paletteIndexesOfTile.push_back(paletteIndex);
            // Fill out the anim structure
            compiled.anims.at(index.animIndex).frames.at(NormalizedTile::keyFrameIndex()).tiles.push_back(keyFrameTile);
            /*
             * Insert this key frame tile into the 'used' map with 'false'. Will use this later to generate a nice
             * warning if the user doesn't ever use a key frame they specified.
             */
            usedKeyFrameTiles.insert(std::pair{keyFrameTile, false});
        } else if (tileIndexes.contains(keyFrameTile)) {
            fatalerror_duplicateKeyFrameTile(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY, normTile.anim,
                                             normTile.tileIndex);
        } else {
            internalerror("compiler::assignTilesPrimary third key tile insertion branch, should be unreachable");
        }

        // Put the rest of this tile's frames into the anim structure for the emitter
        for (std::size_t frameIndex = 1; frameIndex < normTile.frames.size(); frameIndex++) {
            GBATile frameNTile = makeTile(normTile, frameIndex, compiled.palettes.at(paletteIndex));
            compiled.anims.at(index.animIndex).frames.at(frameIndex).tiles.push_back(frameNTile);
        }
    }

    /*
     * Process regular tiles. The user may have used frame 0 of an animated tile to indicate that a particular metatile
     * has an animated component. Since we already processed animated tiles, we can now link up any animated tile
     * metatile entries to the animation tile bank at the beginning of tile.png. Regular tiles will be added and linked
     * at this time.
     */
    for (const auto &indexedNormTile : indexedNormTilesWithColorSets) {
        auto index = std::get<0>(indexedNormTile);
        auto &normTile = std::get<1>(indexedNormTile);
        auto &colorSet = std::get<2>(indexedNormTile);

        // Skip animated tiles since we already processed them
        if (index.animated) {
            continue;
        }

        auto it = std::find_if(std::begin(assignedPalsSolution), std::end(assignedPalsSolution),
                               [&colorSet](const auto &assignedPal) {
                                   // Find which of the assignedSolution palettes this tile belongs to
                                   return (colorSet.first & ~assignedPal.first).none();
                               });
        if (it == std::end(assignedPalsSolution)) {
            internalerror("compiler::assignTilesPrimary it == std::end(assignedPalsSolution)");
        }
        std::size_t paletteIndex = it - std::begin(assignedPalsSolution);
        GBATile gbaTile = makeTile(normTile, NormalizedTile::keyFrameIndex(), compiled.palettes.at(paletteIndex));

        if (usedKeyFrameTiles.contains(gbaTile)) {
            // if this gbaTile was present in key frames, mark it as used
            usedKeyFrameTiles.at(gbaTile) = true;
        }

        // insert only updates the map if the key is not already present
        auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
        if (inserted.second) {
            compiled.tiles.push_back(gbaTile);
            compiled.paletteIndexesOfTile.push_back(paletteIndex);
        }
        std::size_t tileIndex = inserted.first->second;
        compiled.metatileEntries.at(index.tileIndex) = {tileIndex, paletteIndex, normTile.hFlip, normTile.vFlip,
                                                        normTile.attributes};
    }
    compiled.tileIndexes = tileIndexes;

    // Warn user if there are any key frame tiles that did not appear in the metatileEntries
    for (std::size_t animIndex = 0; animIndex < compiled.anims.size(); animIndex++) {
        for (std::size_t tileIndex = 0; tileIndex < compiled.anims.at(animIndex).keyFrame().tiles.size(); tileIndex++) {
            const auto &keyTile = compiled.anims.at(animIndex).keyFrame().tiles.at(tileIndex);
            if (!usedKeyFrameTiles.at(keyTile)) {
                ctx.diag->report(W_KEY_FRAME_NO_MATCHING_TILE, ctx.diag->bold(compiled.anims.at(animIndex).animName),
                                 ctx.diag->bold(tileIndex));
            }
        }
    }

    // error out if there were too many unique tiles
    if (compiled.tiles.size() > ctx.fieldmapConfig.numTilesInPrimary) {
        fatalerror_tooManyUniqueTiles(ctx.err, ctx.compilerSrcPaths, CompilerMode::PRIMARY, compiled.tiles.size(),
                                      ctx.fieldmapConfig.numTilesInPrimary);
    }

    // exit if there were any other errors
    if (ctx.err.errCount > 0 || ctx.diag->in_flight_count_for_level(diag_level::error) > 0) {
        die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::PRIMARY),
                       "errors generated during primary tile assignment");
    }
}

static void assignTilesSecondary(PorytilesContext &ctx, CompiledTileset &compiled,
                                 const std::vector<IndexedNormTileWithColorSet> &indexedNormTilesWithColorSets,
                                 const std::vector<ColorSet> &primaryPaletteColorSets,
                                 const std::vector<ColorSet> &assignedPalsSolution) {
    std::vector<ColorSet> allColorSets{};
    allColorSets.insert(allColorSets.end(), primaryPaletteColorSets.begin(), primaryPaletteColorSets.end());
    allColorSets.insert(allColorSets.end(), assignedPalsSolution.begin(), assignedPalsSolution.end());
    std::unordered_map<GBATile, std::size_t> tileIndexes{};
    std::unordered_map<GBATile, bool> usedKeyFrameTiles{};

    /*
     * Process animated tiles, we want frame 0 of each animation to be at the beginning of the tiles.png in a stable
     * location.
     */
    for (const auto &indexedNormTile : indexedNormTilesWithColorSets) {
        auto index = std::get<0>(indexedNormTile);
        auto &normTile = std::get<1>(indexedNormTile);
        auto &colorSet = std::get<2>(indexedNormTile);

        // Skip regular tiles, since we will process them next
        if (!index.animated) {
            continue;
        }

        pt_logln(ctx, stderr, "found anim tile (frame count = {}) for anim={}, tile={}", normTile.frames.size(),
                 index.animIndex, index.tileIndex);
        auto it = std::find_if(std::begin(allColorSets), std::end(allColorSets), [&colorSet](const auto &assignedPal) {
            // Find which of the allColorSets palettes this tile belongs to
            return (colorSet.first & ~assignedPal.first).none();
        });
        if (it == std::end(allColorSets)) {
            internalerror("compiler::assignTilesSecondary it == std::end(allColorSets)");
        }
        std::size_t paletteIndex = it - std::begin(allColorSets);

        // Create the GBATile for this tile's key frame
        GBATile keyFrameTile = makeTile(normTile, NormalizedTile::keyFrameIndex(), compiled.palettes[paletteIndex]);

        if (ctx.compilerContext.pairedPrimaryTileset->tileIndexes.contains(keyFrameTile)) {
            if (ctx.compilerContext.pairedPrimaryTileset->tileIndexes.at(keyFrameTile) == 0) {
                /*
                 * Fatal error if the user provided a transparent key frame tile. This is not allowed, since there would
                 * be no way to tell if a transparent user provided tile on the layer sheet referred to the true index 0
                 * transparent tile, or if it was a reference into this particular animation.
                 */
                fatalerror_transparentKeyFrameTile(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY,
                                                   normTile.anim, normTile.tileIndex);
            } else {
                /*
                 * If keyFrameTile was elsewhere present in the primary set, this is a user error because it renders the
                 * animation inoperable, any reference to the repTile in the secondary set will be linked to the primary
                 * tile as opposed to the animation.
                 */
                fatalerror_keyFramePresentInPairedPrimary(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY,
                                                          normTile.anim, normTile.tileIndex);
            }
        }

        // Insert this tile's key frame into the seen tiles map
        auto inserted = tileIndexes.insert({keyFrameTile, compiled.tiles.size()});

        // Insertion happened
        if (inserted.second) {
            // Insert this tile's key frame into the tiles.png
            compiled.tiles.push_back(keyFrameTile);
            compiled.paletteIndexesOfTile.push_back(paletteIndex);
            // Fill out the anim structure
            compiled.anims.at(index.animIndex).frames.at(NormalizedTile::keyFrameIndex()).tiles.push_back(keyFrameTile);
            /*
             * Insert this key frame tile into the 'used' map with 'false'. Will use this later to generate a nice
             * warning if the user doesn't ever use a key frame they specified.
             */
            usedKeyFrameTiles.insert(std::pair{keyFrameTile, false});
        } else if (tileIndexes.contains(keyFrameTile)) {
            fatalerror_duplicateKeyFrameTile(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY, normTile.anim,
                                             normTile.tileIndex);
        } else {
            internalerror("compiler::assignTilesSecondary third key tile insertion branch, should be unreachable");
        }

        // Put the rest of this tile's frames into the anim structure for the emitter
        for (std::size_t frameIndex = 1; frameIndex < normTile.frames.size(); frameIndex++) {
            GBATile frameNTile = makeTile(normTile, frameIndex, compiled.palettes.at(paletteIndex));
            compiled.anims.at(index.animIndex).frames.at(frameIndex).tiles.push_back(frameNTile);
        }
    }

    /*
     * Process regular tiles. The user may have used frame 0 of an animated tile to indicate that a particular metatile
     * has an animated component. Since we already processed animated tiles, we can now link up any animated tile
     * metatileEntries to the animation tile bank at the beginning of tile.png. Regular tiles will be added and linked
     * at this time.
     */
    for (const auto &indexedNormTile : indexedNormTilesWithColorSets) {
        auto index = std::get<0>(indexedNormTile);
        auto &normTile = std::get<1>(indexedNormTile);
        auto &colorSet = std::get<2>(indexedNormTile);

        // Skip animated tiles since we already processed them
        if (index.animated) {
            continue;
        }

        auto it = std::find_if(std::begin(allColorSets), std::end(allColorSets), [&colorSet](const auto &assignedPal) {
            // Find which of the allColorSets palettes this tile belongs to
            return (colorSet.first & ~assignedPal.first).none();
        });
        if (it == std::end(allColorSets)) {
            internalerror("compiler::assignTilesSecondary it == std::end(allColorSets)");
        }
        std::size_t paletteIndex = it - std::begin(allColorSets);
        GBATile gbaTile = makeTile(normTile, NormalizedTile::keyFrameIndex(), compiled.palettes[paletteIndex]);

        if (usedKeyFrameTiles.contains(gbaTile)) {
            // if this gbaTile was present in key frames, mark it as used
            usedKeyFrameTiles.at(gbaTile) = true;
        }

        if (ctx.compilerContext.pairedPrimaryTileset->tileIndexes.contains(gbaTile)) {
            // Tile was in the primary set
            compiled.metatileEntries.at(index.tileIndex) = {
                ctx.compilerContext.pairedPrimaryTileset->tileIndexes.at(gbaTile), paletteIndex, normTile.hFlip,
                normTile.vFlip, normTile.attributes};
        } else {
            // Tile was in the secondary set
            auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
            if (inserted.second) {
                compiled.tiles.push_back(gbaTile);
                compiled.paletteIndexesOfTile.push_back(paletteIndex);
            }
            std::size_t tileIndex = inserted.first->second;
            // Offset the tile index by the secondary tileset VRAM location, which is just the size of the primary tiles
            compiled.metatileEntries.at(index.tileIndex) = {tileIndex + ctx.fieldmapConfig.numTilesInPrimary,
                                                            paletteIndex, normTile.hFlip, normTile.vFlip,
                                                            normTile.attributes};
        }
    }
    compiled.tileIndexes = tileIndexes;

    // Warn user if there are any key frame tiles that did not appear in the metatileEntries
    for (std::size_t animIndex = 0; animIndex < compiled.anims.size(); animIndex++) {
        for (std::size_t tileIndex = 0; tileIndex < compiled.anims.at(animIndex).keyFrame().tiles.size(); tileIndex++) {
            const auto &keyTile = compiled.anims.at(animIndex).keyFrame().tiles.at(tileIndex);
            if (!usedKeyFrameTiles.at(keyTile)) {
                ctx.diag->report(W_KEY_FRAME_NO_MATCHING_TILE, ctx.diag->bold(compiled.anims.at(animIndex).animName),
                                 ctx.diag->bold(tileIndex));
            }
        }
    }

    // error out if there were too many unique tiles
    if (compiled.tiles.size() > ctx.fieldmapConfig.numTilesInSecondary()) {
        fatalerror_tooManyUniqueTiles(ctx.err, ctx.compilerSrcPaths, CompilerMode::SECONDARY, compiled.tiles.size(),
                                      ctx.fieldmapConfig.numTilesInSecondary());
    }

    // exit if there were any other errors
    if (ctx.err.errCount > 0 || ctx.diag->in_flight_count_for_level(diag_level::error) > 0) {
        die_errorCount(ctx.err, ctx.compilerSrcPaths.modeBasedSrcPath(CompilerMode::SECONDARY),
                       "errors generated during secondary tile assignment");
    }
}

static std::vector<ColorSet> assignHardwarePalettes(
    const PorytilesContext &ctx, CompiledTileset &compiled, const CompilerMode mode,
    const std::vector<ColorSet> &assignedPalsSolution, const std::unordered_map<std::size_t, BGR15> &indexToColor,
    const std::unordered_map<std::size_t, std::vector<std::pair<std::size_t, BGR15>>> &palOverridesMap) {
    if (mode != CompilerMode::PRIMARY && mode != CompilerMode::SECONDARY) {
        internalerror("compiler::assignHardwarePalettes invalid compiler mode");
    }

    std::unordered_set<std::size_t> usedHardwarePalettes{};
    std::vector<ColorSet> reorderedAssignedPalsSolution{};
    reorderedAssignedPalsSolution.resize(assignedPalsSolution.size());

    if (mode == CompilerMode::SECONDARY) {
        for (std::size_t i = 0; i < ctx.fieldmapConfig.numPalettesInPrimary; i++) {
            // Copy the primary set's palettes into this tileset so tiles can use them
            for (std::size_t j = 0; j < PAL_SIZE; j++) {
                compiled.palettes.at(i).colors.at(j) =
                    ctx.compilerContext.pairedPrimaryTileset->palettes.at(i).colors.at(j);
            }
        }
    }

    for (const auto &palSolution : assignedPalsSolution) {
        /*
         * Iterate over assignedPalsSolution and first place all solutions tagged with an override into the correct
         * hardware palette.
         */
        if (palSolution.second != SIZE_MAX) {
            auto overrideHardwarePalIndex = palSolution.second;
            const auto &overriddenSlots = palOverridesMap.at(overrideHardwarePalIndex);
            std::unordered_set<std::size_t> usedSlots{};
            std::unordered_set<BGR15> usedBgrs{};
            /*
             * Iterate over overriddenSlots and first place all override colors into the correct slot.
             */
            for (const auto &slot : overriddenSlots) {
                const auto &slotIndex = slot.first;
                const auto &bgr = slot.second;
                compiled.palettes.at(overrideHardwarePalIndex).colors.at(slotIndex) = bgr;
                usedSlots.insert(slotIndex);
                usedBgrs.insert(bgr);
            }

            std::size_t slotIndex = 1;
            for (std::size_t j = 0; j < palSolution.first.size(); j++) {
                // Skip over slotIndexes we already used above
                for (; usedSlots.contains(slotIndex); slotIndex++)
                    ;
                if (palSolution.first.test(j)) {
                    const auto &bgr = indexToColor.at(j);
                    if (usedBgrs.contains(indexToColor.at(j))) {
                        continue;
                    }
                    compiled.palettes.at(overrideHardwarePalIndex).colors.at(slotIndex) = bgr;
                    usedSlots.insert(slotIndex);
                    usedBgrs.insert(bgr);
                    slotIndex++;
                }
            }

            /*
             * Place transparent color, set logical palette size, mark this hardware palette as used, and insert the
             * hardware palette assignment solution into the correct order.
             */
            compiled.palettes.at(overrideHardwarePalIndex).colors.at(0) =
                rgbaToBgr(ctx.compilerConfig.transparencyColor);
            compiled.palettes.at(overrideHardwarePalIndex).size = slotIndex;
            usedHardwarePalettes.insert(overrideHardwarePalIndex);
            const std::size_t modeBasedIndex = mode == CompilerMode::PRIMARY
                                                   ? overrideHardwarePalIndex
                                                   : overrideHardwarePalIndex - ctx.fieldmapConfig.numPalettesInPrimary;
            reorderedAssignedPalsSolution.at(modeBasedIndex) = palSolution;
        }
    }

    std::size_t hardwarePalIndex = mode == CompilerMode::PRIMARY ? 0 : ctx.fieldmapConfig.numPalettesInPrimary;
    for (const auto &palSolution : assignedPalsSolution) {
        /*
         * Iterate over assignedPalsSolution and place all solutions without an override into the first available
         * hardware palette.
         */
        if (palSolution.second == SIZE_MAX) {
            // Increment hardwarePalIndex until it’s no longer found in usedHardwarePalettes
            for (; usedHardwarePalettes.contains(hardwarePalIndex); hardwarePalIndex++)
                ;

            // Place the solution
            std::size_t colorIndex = 1;
            for (std::size_t j = 0; j < palSolution.first.size(); j++) {
                if (palSolution.first.test(j)) {
                    compiled.palettes.at(hardwarePalIndex).colors.at(colorIndex) = indexToColor.at(j);
                    colorIndex++;
                }
            }
            compiled.palettes.at(hardwarePalIndex).colors.at(0) = rgbaToBgr(ctx.compilerConfig.transparencyColor);
            compiled.palettes.at(hardwarePalIndex).size = colorIndex;
            usedHardwarePalettes.insert(hardwarePalIndex);
            const std::size_t modeBasedIndex = mode == CompilerMode::PRIMARY
                                                   ? hardwarePalIndex
                                                   : hardwarePalIndex - ctx.fieldmapConfig.numPalettesInPrimary;
            reorderedAssignedPalsSolution.at(modeBasedIndex) = palSolution;
        }
    }

    return reorderedAssignedPalsSolution;
}

std::unique_ptr<CompiledTileset>
compile(PorytilesContext &ctx, CompilerMode compilerMode, const DecompiledTileset &decompiledTileset,
        const std::vector<RGBATile> &palettePrimers, const std::vector<RGBATile> &paletteOverrides,
        const std::unordered_map<std::size_t, std::vector<std::pair<std::size_t, BGR15>>> &palOverridesMap) {
    /*
     * Sanity check for matching paired primary palette sizes when compiling secondary
     */
    if (compilerMode == CompilerMode::SECONDARY &&
        (ctx.fieldmapConfig.numPalettesInPrimary != ctx.compilerContext.pairedPrimaryTileset->palettes.size())) {
        // FIXME : is this actually an internal error? It seems like a user could force this to happen via bad
        // inputs
        internalerror(fmt::format(
            "compiler::compile config.numPalettesInPrimary did not match primary palette set size ({} != {})",
            ctx.fieldmapConfig.numPalettesInPrimary, ctx.compilerContext.pairedPrimaryTileset->palettes.size()));
    }

    /*
     * Create a unique pointer to our CompiledTileset
     */
    auto compiled = std::make_unique<CompiledTileset>();

    /*
     * Throw an error if there are too many metatiles in the input
     */
    if (compilerMode == CompilerMode::PRIMARY) {
        compiled->palettes.resize(ctx.fieldmapConfig.numPalettesInPrimary);
        std::size_t srcMetatileCount = (decompiledTileset.tiles.size() / ctx.fieldmapConfig.numTilesPerMetatile);
        if (srcMetatileCount > ctx.fieldmapConfig.numMetatilesInPrimary) {
            fatalerror_tooManyMetatiles(ctx.err, ctx.compilerSrcPaths, compilerMode, srcMetatileCount,
                                        ctx.fieldmapConfig.numMetatilesInPrimary);
        }
    } else if (compilerMode == CompilerMode::SECONDARY) {
        compiled->palettes.resize(ctx.fieldmapConfig.numPalettesTotal);
        std::size_t srcMetatileCount = (decompiledTileset.tiles.size() / ctx.fieldmapConfig.numTilesPerMetatile);
        if (srcMetatileCount > ctx.fieldmapConfig.numMetatilesInSecondary()) {
            fatalerror_tooManyMetatiles(ctx.err, ctx.compilerSrcPaths, compilerMode, srcMetatileCount,
                                        ctx.fieldmapConfig.numMetatilesInSecondary());
        }
    } else {
        internalerror_unknownCompilerMode("compiler::compile");
    }
    compiled->metatileEntries.resize(decompiledTileset.tiles.size());

    /*
     * Build indexed normalized tiles, order of this vector matches the decompiled iteration order, with animated
     * tiles at the beginning. It also builds a separate vector of normalized primer tiles.
     */
    auto [indexedNormTiles, normalizedPrimers, normalizedOverrides] =
        normalizeDecompTiles(ctx, compilerMode, decompiledTileset, palettePrimers, paletteOverrides);

    /*
     * Map each unique color to a unique index between 0 and 240 (15 colors per palette * 16 palettes MAX)
     */
    std::unordered_map<BGR15, std::size_t> emptyPrimaryColorIndexMap;
    const std::unordered_map<BGR15, std::size_t> *primaryColorIndexMap = &emptyPrimaryColorIndexMap;
    if (compilerMode == CompilerMode::SECONDARY) {
        primaryColorIndexMap = &(ctx.compilerContext.pairedPrimaryTileset->colorIndexMap);
    }
    auto [colorToIndex, indexToColor] = buildColorIndexMaps(ctx, compilerMode, indexedNormTiles, *primaryColorIndexMap,
                                                            normalizedPrimers, normalizedOverrides);
    compiled->colorIndexMap = colorToIndex;

    /*
     * colorSets is a vector: this enforces a well-defined ordering so tileset compilation results are identical
     * across all compilers and platforms. A ColorSet is just a bitset<240> that marks which colors are present
     * (indexes are based on the colorIndexMaps from above)
     */
    auto [indexedNormTilesWithColorSets, colorSets, primerColorSets, overrideColorSets] =
        matchNormalizedWithColorSets(colorToIndex, indexedNormTiles, normalizedPrimers, normalizedOverrides);

    /*
     * Run palette assignment.
     */
    auto [assignedPalsSolution, primaryPaletteColorSets] =
        runPaletteAssignmentMatrix(ctx, compilerMode, colorSets, primerColorSets, overrideColorSets, colorToIndex);

    /*
     * Copy the assignments into the compiled palettes. In a future version we will support sibling tiles (tile
     * sharing) and so we may need to do something fancier here so that the colors align correctly.
     */
    std::vector<ColorSet> reorderedAssignedPalsSolution =
        assignHardwarePalettes(ctx, *compiled, compilerMode, assignedPalsSolution, indexToColor, palOverridesMap);

    /*
     * Setup the compiled animations
     */
    compiled->anims.reserve(decompiledTileset.anims.size());
    for (std::size_t animIndex = 0; animIndex < decompiledTileset.anims.size(); animIndex++) {
        compiled->anims.emplace_back(decompiledTileset.anims.at(animIndex).animName);
        compiled->anims.at(animIndex).frames.reserve(decompiledTileset.anims.at(animIndex).frames.size());
        for (std::size_t frameIndex = 0; frameIndex < decompiledTileset.anims.at(animIndex).frames.size();
             frameIndex++) {
            compiled->anims.at(animIndex).frames.emplace_back(
                decompiledTileset.anims.at(animIndex).frames.at(frameIndex).frameName);
        }
    }

    /*
     * Build the metatile entries.
     */
    if (compilerMode == CompilerMode::PRIMARY) {
        assignTilesPrimary(ctx, *compiled, indexedNormTilesWithColorSets, reorderedAssignedPalsSolution);
    } else if (compilerMode == CompilerMode::SECONDARY) {
        assignTilesSecondary(ctx, *compiled, indexedNormTilesWithColorSets, primaryPaletteColorSets,
                             reorderedAssignedPalsSolution);
    } else {
        internalerror_unknownCompilerMode("compiler::compile");
    }

    /*
     * Push back transparent tiles to pad out tileset to a non-zero multiple of 16
     */
    compiled->sizeBeforePadding = compiled->tiles.size();
    while (compiled->tiles.size() % 16 != 0 || compiled->tiles.empty()) {
        compiled->tiles.push_back(GBA_TILE_TRANSPARENT);
        compiled->paletteIndexesOfTile.push_back(0);
    }

    return compiled;
}

} // namespace porytiles

#ifndef DOCTEST_CONFIG_DISABLE
TEST_CASE("insertRGBA should add new colors in order and return the correct index for a given color") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    porytiles::NormalizedPalette palette1{};
    palette1.size = 1;
    palette1.colors = {};

    porytiles::RGBATile dummy{};
    dummy.type = porytiles::TileType::LAYERED;
    dummy.metatileIndex = 0;
    dummy.subtile = porytiles::Subtile::NORTHEAST;

    // Transparent should return 0
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA_MAGENTA, 0, 0, true) == 0);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}, 0, 0, true) == 0);

    // insert colors
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 1);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{8, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 2);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{16, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 3);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{24, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 4);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{32, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 5);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{40, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 6);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{48, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 7);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{56, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 8);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{64, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 9);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 10);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{80, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 11);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{88, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 12);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{96, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 13);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{104, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 14);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 15);

    // repeat colors should return their indexes
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 10);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 15);

    // Transparent should still return 0
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA_MAGENTA, 0, 0, true) == 0);
    CHECK(insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
                     porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}, 0, 0, true) == 0);

    // Should generate an error, palette full
    insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
               porytiles::RGBA_CYAN, 0, 0, true);
    CHECK(ctx.err.errCount == 1);

    // invalid alpha value, must be opaque or transparent, generates another error
    insertRGBA(ctx, porytiles::CompilerMode::PRIMARY, dummy, ctx.compilerConfig.transparencyColor, palette1,
               porytiles::RGBA32{0, 0, 0, 12}, 0, 0, true);
    CHECK(ctx.err.errCount == 2);
}

TEST_CASE("candidate should return the NormalizedTile with requested flips") {
    porytiles::PorytilesContext ctx{};

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/corners.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/corners.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
    porytiles::RGBATile tile = tiles.tiles[0];

    SUBCASE("case: no flips") {
        std::vector<porytiles::RGBATile> singleFrameTile = {tile};
        porytiles::NormalizedTile candidate =
            porytiles::candidate(ctx, porytiles::CompilerMode::PRIMARY, ctx.compilerConfig.transparencyColor,
                                 singleFrameTile, false, false, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.keyFrame().colorIndexes[0] == 1);
        CHECK(candidate.keyFrame().colorIndexes[7] == 2);
        CHECK(candidate.keyFrame().colorIndexes[9] == 3);
        CHECK(candidate.keyFrame().colorIndexes[14] == 4);
        CHECK(candidate.keyFrame().colorIndexes[18] == 2);
        CHECK(candidate.keyFrame().colorIndexes[21] == 5);
        CHECK(candidate.keyFrame().colorIndexes[42] == 3);
        CHECK(candidate.keyFrame().colorIndexes[45] == 1);
        CHECK(candidate.keyFrame().colorIndexes[49] == 6);
        CHECK(candidate.keyFrame().colorIndexes[54] == 7);
        CHECK(candidate.keyFrame().colorIndexes[56] == 8);
        CHECK(candidate.keyFrame().colorIndexes[63] == 5);
    }

    SUBCASE("case: hFlip") {
        std::vector<porytiles::RGBATile> singleFrameTile = {tile};
        porytiles::NormalizedTile candidate =
            porytiles::candidate(ctx, porytiles::CompilerMode::PRIMARY, ctx.compilerConfig.transparencyColor,
                                 singleFrameTile, true, false, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.keyFrame().colorIndexes[0] == 1);
        CHECK(candidate.keyFrame().colorIndexes[7] == 2);
        CHECK(candidate.keyFrame().colorIndexes[9] == 3);
        CHECK(candidate.keyFrame().colorIndexes[14] == 4);
        CHECK(candidate.keyFrame().colorIndexes[18] == 5);
        CHECK(candidate.keyFrame().colorIndexes[21] == 1);
        CHECK(candidate.keyFrame().colorIndexes[42] == 2);
        CHECK(candidate.keyFrame().colorIndexes[45] == 4);
        CHECK(candidate.keyFrame().colorIndexes[49] == 6);
        CHECK(candidate.keyFrame().colorIndexes[54] == 7);
        CHECK(candidate.keyFrame().colorIndexes[56] == 5);
        CHECK(candidate.keyFrame().colorIndexes[63] == 8);
    }

    SUBCASE("case: vFlip") {
        std::vector<porytiles::RGBATile> singleFrameTile = {tile};
        porytiles::NormalizedTile candidate =
            porytiles::candidate(ctx, porytiles::CompilerMode::PRIMARY, ctx.compilerConfig.transparencyColor,
                                 singleFrameTile, false, true, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.keyFrame().colorIndexes[0] == 1);
        CHECK(candidate.keyFrame().colorIndexes[7] == 2);
        CHECK(candidate.keyFrame().colorIndexes[9] == 3);
        CHECK(candidate.keyFrame().colorIndexes[14] == 4);
        CHECK(candidate.keyFrame().colorIndexes[18] == 5);
        CHECK(candidate.keyFrame().colorIndexes[21] == 6);
        CHECK(candidate.keyFrame().colorIndexes[42] == 7);
        CHECK(candidate.keyFrame().colorIndexes[45] == 2);
        CHECK(candidate.keyFrame().colorIndexes[49] == 5);
        CHECK(candidate.keyFrame().colorIndexes[54] == 8);
        CHECK(candidate.keyFrame().colorIndexes[56] == 6);
        CHECK(candidate.keyFrame().colorIndexes[63] == 7);
    }

    SUBCASE("case: hFlip and vFlip") {
        std::vector<porytiles::RGBATile> singleFrameTile = {tile};
        porytiles::NormalizedTile candidate =
            porytiles::candidate(ctx, porytiles::CompilerMode::PRIMARY, ctx.compilerConfig.transparencyColor,
                                 singleFrameTile, true, true, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.keyFrame().colorIndexes[0] == 1);
        CHECK(candidate.keyFrame().colorIndexes[7] == 2);
        CHECK(candidate.keyFrame().colorIndexes[9] == 3);
        CHECK(candidate.keyFrame().colorIndexes[14] == 4);
        CHECK(candidate.keyFrame().colorIndexes[18] == 5);
        CHECK(candidate.keyFrame().colorIndexes[21] == 6);
        CHECK(candidate.keyFrame().colorIndexes[42] == 1);
        CHECK(candidate.keyFrame().colorIndexes[45] == 7);
        CHECK(candidate.keyFrame().colorIndexes[49] == 8);
        CHECK(candidate.keyFrame().colorIndexes[54] == 6);
        CHECK(candidate.keyFrame().colorIndexes[56] == 7);
        CHECK(candidate.keyFrame().colorIndexes[63] == 5);
    }
}

TEST_CASE("normalize should return the normal form of the given tile") {
    porytiles::PorytilesContext ctx{};

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/corners.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/corners.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
    porytiles::RGBATile tile = tiles.tiles[0];

    std::vector<porytiles::RGBATile> singleFrameTile = {tile};
    porytiles::NormalizedTile normalizedTile =
        porytiles::normalize(ctx, porytiles::CompilerMode::PRIMARY, singleFrameTile);
    CHECK(normalizedTile.palette.size == 9);
    CHECK_FALSE(normalizedTile.hFlip);
    CHECK_FALSE(normalizedTile.vFlip);
    CHECK(normalizedTile.keyFrame().colorIndexes[0] == 1);
    CHECK(normalizedTile.keyFrame().colorIndexes[7] == 2);
    CHECK(normalizedTile.keyFrame().colorIndexes[9] == 3);
    CHECK(normalizedTile.keyFrame().colorIndexes[14] == 4);
    CHECK(normalizedTile.keyFrame().colorIndexes[18] == 2);
    CHECK(normalizedTile.keyFrame().colorIndexes[21] == 5);
    CHECK(normalizedTile.keyFrame().colorIndexes[42] == 3);
    CHECK(normalizedTile.keyFrame().colorIndexes[45] == 1);
    CHECK(normalizedTile.keyFrame().colorIndexes[49] == 6);
    CHECK(normalizedTile.keyFrame().colorIndexes[54] == 7);
    CHECK(normalizedTile.keyFrame().colorIndexes[56] == 8);
    CHECK(normalizedTile.keyFrame().colorIndexes[63] == 5);
}

TEST_CASE("normalizeDecompTiles should correctly normalize all tiles in the decomp tileset") {
    porytiles::PorytilesContext ctx{};

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);

    auto [indexedNormTiles, _1, _2] = normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});

    CHECK(indexedNormTiles.size() == 4);

    // First tile normal form is vFlipped, palette should have 2 colors
    CHECK(indexedNormTiles[0].second.keyFrame().colorIndexes[0] == 0);
    CHECK(indexedNormTiles[0].second.keyFrame().colorIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(indexedNormTiles[0].second.keyFrame().colorIndexes[i] == 1);
    }
    CHECK(indexedNormTiles[0].second.palette.size == 2);
    CHECK(indexedNormTiles[0].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[0].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK_FALSE(indexedNormTiles[0].second.hFlip);
    CHECK(indexedNormTiles[0].second.vFlip);
    CHECK(indexedNormTiles[0].first.tileIndex == 0);

    // Second tile already in normal form, palette should have 3 colors
    CHECK(indexedNormTiles[1].second.keyFrame().colorIndexes[0] == 0);
    CHECK(indexedNormTiles[1].second.keyFrame().colorIndexes[54] == 1);
    CHECK(indexedNormTiles[1].second.keyFrame().colorIndexes[55] == 1);
    CHECK(indexedNormTiles[1].second.keyFrame().colorIndexes[62] == 1);
    CHECK(indexedNormTiles[1].second.keyFrame().colorIndexes[63] == 2);
    CHECK(indexedNormTiles[1].second.palette.size == 3);
    CHECK(indexedNormTiles[1].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[1].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(indexedNormTiles[1].second.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK_FALSE(indexedNormTiles[1].second.hFlip);
    CHECK_FALSE(indexedNormTiles[1].second.vFlip);
    CHECK(indexedNormTiles[1].first.tileIndex == 1);

    // Third tile normal form is hFlipped, palette should have 3 colors
    CHECK(indexedNormTiles[2].second.keyFrame().colorIndexes[0] == 0);
    CHECK(indexedNormTiles[2].second.keyFrame().colorIndexes[7] == 1);
    CHECK(indexedNormTiles[2].second.keyFrame().colorIndexes[56] == 1);
    CHECK(indexedNormTiles[2].second.keyFrame().colorIndexes[63] == 2);
    CHECK(indexedNormTiles[2].second.palette.size == 3);
    CHECK(indexedNormTiles[2].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[2].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
    CHECK(indexedNormTiles[2].second.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK_FALSE(indexedNormTiles[2].second.vFlip);
    CHECK(indexedNormTiles[2].second.hFlip);
    CHECK(indexedNormTiles[2].first.tileIndex == 2);

    // Fourth tile normal form is hFlipped and vFlipped, palette should have 2 colors
    CHECK(indexedNormTiles[3].second.keyFrame().colorIndexes[0] == 0);
    CHECK(indexedNormTiles[3].second.keyFrame().colorIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(indexedNormTiles[3].second.keyFrame().colorIndexes[i] == 1);
    }
    CHECK(indexedNormTiles[3].second.palette.size == 2);
    CHECK(indexedNormTiles[3].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(indexedNormTiles[3].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(indexedNormTiles[3].second.hFlip);
    CHECK(indexedNormTiles[3].second.vFlip);
    CHECK(indexedNormTiles[3].first.tileIndex == 3);
}

TEST_CASE("normalizeDecompTiles should correctly normalize multi-frame animated tiles") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
    png::image<png::rgba_pixel> tilesPng{"Resources/Tests/2x2_pattern_2.png"};

    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, tilesPng);

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_flower_white"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_flower_yellow"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_water_1"}));

    porytiles::AnimationPng<png::rgba_pixel> white00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_flower_white/00.png"}, "anim_flower_white", "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> white01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_flower_white/01.png"}, "anim_flower_white", "01.png"};
    porytiles::AnimationPng<png::rgba_pixel> white02{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_flower_white/02.png"}, "anim_flower_white", "02.png"};

    porytiles::AnimationPng<png::rgba_pixel> yellow00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_flower_yellow/00.png"}, "anim_flower_yellow", "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> yellow01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_flower_yellow/01.png"}, "anim_flower_yellow", "01.png"};
    porytiles::AnimationPng<png::rgba_pixel> yellow02{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_flower_yellow/02.png"}, "anim_flower_yellow", "02.png"};

    porytiles::AnimationPng<png::rgba_pixel> water00{png::image<png::rgba_pixel>{"Resources/Tests/anim_water_1/00.png"},
                                                     "anim_water_1", "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> water01{png::image<png::rgba_pixel>{"Resources/Tests/anim_water_1/01.png"},
                                                     "anim_water_1", "01.png"};

    std::vector<porytiles::AnimationPng<png::rgba_pixel>> whiteAnim{};
    std::vector<porytiles::AnimationPng<png::rgba_pixel>> yellowAnim{};
    std::vector<porytiles::AnimationPng<png::rgba_pixel>> waterAnim{};

    whiteAnim.push_back(white00);
    whiteAnim.push_back(white01);
    whiteAnim.push_back(white02);

    yellowAnim.push_back(yellow00);
    yellowAnim.push_back(yellow01);
    yellowAnim.push_back(yellow02);

    waterAnim.push_back(water00);
    waterAnim.push_back(water01);

    std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
    anims.push_back(whiteAnim);
    anims.push_back(yellowAnim);
    anims.push_back(waterAnim);

    porytiles::importAnimTiles(ctx, porytiles::CompilerMode::PRIMARY, anims, tiles);

    auto [indexedNormTiles, _1, _2] = normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});

    CHECK(indexedNormTiles.size() == 13);

    // white flower multiframe tiles
    CHECK(indexedNormTiles.at(0).first.animated);
    CHECK(indexedNormTiles.at(0).first.animIndex == 0);
    CHECK(indexedNormTiles.at(0).first.tileIndex == 0);

    CHECK(indexedNormTiles.at(1).first.animated);
    CHECK(indexedNormTiles.at(1).first.animIndex == 0);
    CHECK(indexedNormTiles.at(1).first.tileIndex == 1);

    CHECK(indexedNormTiles.at(2).first.animated);
    CHECK(indexedNormTiles.at(2).first.animIndex == 0);
    CHECK(indexedNormTiles.at(2).first.tileIndex == 2);

    CHECK(indexedNormTiles.at(3).first.animated);
    CHECK(indexedNormTiles.at(3).first.animIndex == 0);
    CHECK(indexedNormTiles.at(3).first.tileIndex == 3);

    // yellow flower multiframe tiles
    CHECK(indexedNormTiles.at(4).first.animated);
    CHECK(indexedNormTiles.at(4).first.animIndex == 1);
    CHECK(indexedNormTiles.at(4).first.tileIndex == 0);

    CHECK(indexedNormTiles.at(5).first.animated);
    CHECK(indexedNormTiles.at(5).first.animIndex == 1);
    CHECK(indexedNormTiles.at(5).first.tileIndex == 1);

    CHECK(indexedNormTiles.at(6).first.animated);
    CHECK(indexedNormTiles.at(6).first.animIndex == 1);
    CHECK(indexedNormTiles.at(6).first.tileIndex == 2);

    CHECK(indexedNormTiles.at(7).first.animated);
    CHECK(indexedNormTiles.at(7).first.animIndex == 1);
    CHECK(indexedNormTiles.at(7).first.tileIndex == 3);

    // water multiframe tile
    CHECK(indexedNormTiles.at(8).first.animated);
    CHECK(indexedNormTiles.at(8).first.animIndex == 2);
    CHECK(indexedNormTiles.at(8).first.tileIndex == 0);
    CHECK(indexedNormTiles.at(8).second.palette.size == 8);
    CHECK_FALSE(indexedNormTiles.at(8).second.hFlip);
    CHECK(indexedNormTiles.at(8).second.vFlip);

    // regular tiles
    CHECK_FALSE(indexedNormTiles.at(9).first.animated);
    CHECK(indexedNormTiles.at(9).first.animIndex == 0);
    CHECK(indexedNormTiles.at(9).first.tileIndex == 0);

    CHECK_FALSE(indexedNormTiles.at(10).first.animated);
    CHECK(indexedNormTiles.at(10).first.animIndex == 0);
    CHECK(indexedNormTiles.at(10).first.tileIndex == 1);

    CHECK_FALSE(indexedNormTiles.at(11).first.animated);
    CHECK(indexedNormTiles.at(11).first.animIndex == 0);
    CHECK(indexedNormTiles.at(11).first.tileIndex == 2);

    CHECK_FALSE(indexedNormTiles.at(12).first.animated);
    CHECK(indexedNormTiles.at(12).first.animIndex == 0);
    CHECK(indexedNormTiles.at(12).first.tileIndex == 3);
}

TEST_CASE("buildColorIndexMaps should build a map of all unique colors in the decomp tileset") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
    auto [indexedNormTiles, _1, _2] =
        porytiles::normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});

    auto [colorToIndex, indexToColor] =
        porytiles::buildColorIndexMaps(ctx, porytiles::CompilerMode::PRIMARY, indexedNormTiles, {}, {}, {});

    CHECK(colorToIndex.size() == 4);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);
}

TEST_CASE("toColorSet should return the correct bitset based on the supplied palette") {
    std::unordered_map<porytiles::BGR15, std::size_t> colorIndexMap = {
        {porytiles::rgbaToBgr(porytiles::RGBA_BLUE), 0},   {porytiles::rgbaToBgr(porytiles::RGBA_RED), 1},
        {porytiles::rgbaToBgr(porytiles::RGBA_GREEN), 2},  {porytiles::rgbaToBgr(porytiles::RGBA_CYAN), 3},
        {porytiles::rgbaToBgr(porytiles::RGBA_YELLOW), 4},
    };

    SUBCASE("palette 1") {
        porytiles::NormalizedTile tile{porytiles::RGBA_MAGENTA};
        tile.palette.size = 2;
        tile.palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
        tile.palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_RED);

        ColorSet colorSet = porytiles::toColorSet(colorIndexMap, tile);
        CHECK(colorSet.first.count() == 1);
        CHECK(colorSet.first.test(1));
    }

    SUBCASE("palette 2") {
        porytiles::NormalizedTile tile{porytiles::RGBA_MAGENTA};
        tile.palette.size = 4;
        tile.palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
        tile.palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_YELLOW);
        tile.palette.colors[2] = porytiles::rgbaToBgr(porytiles::RGBA_GREEN);
        tile.palette.colors[3] = porytiles::rgbaToBgr(porytiles::RGBA_CYAN);

        ColorSet colorSet = porytiles::toColorSet(colorIndexMap, tile);
        CHECK(colorSet.first.count() == 3);
        CHECK(colorSet.first.test(4));
        CHECK(colorSet.first.test(2));
        CHECK(colorSet.first.test(3));
    }
}

TEST_CASE("matchNormalizedWithColorSets should return the expected data structures") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
    auto [indexedNormTiles, _1, _2] =
        porytiles::normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});
    auto [colorToIndex, indexToColor] =
        porytiles::buildColorIndexMaps(ctx, porytiles::CompilerMode::PRIMARY, indexedNormTiles, {}, {}, {});

    CHECK(colorToIndex.size() == 4);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
    CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);

    auto [indexedNormTilesWithColorSets, colorSets, _3, _4] =
        porytiles::matchNormalizedWithColorSets(colorToIndex, indexedNormTiles, {}, {});

    CHECK(indexedNormTilesWithColorSets.size() == 4);
    // colorSets size is 3 because first and fourth tiles have the same palette
    CHECK(colorSets.size() == 3);

    // First tile has 1 non-transparent color, color should be BLUE
    CHECK(std::get<0>(indexedNormTilesWithColorSets[0]).tileIndex == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).keyFrame().colorIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).keyFrame().colorIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).keyFrame().colorIndexes[i] == 1);
    }
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.size == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[0]).hFlip);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).vFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[0]).first.count() == 1);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[0]).first.test(0));
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[0])) !=
          colorSets.end());

    // Second tile has two non-transparent colors, RED and GREEN
    CHECK(std::get<0>(indexedNormTilesWithColorSets[1]).tileIndex == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).keyFrame().colorIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).keyFrame().colorIndexes[54] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).keyFrame().colorIndexes[55] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).keyFrame().colorIndexes[62] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).keyFrame().colorIndexes[63] == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.size == 3);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[1]).hFlip);
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[1]).vFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).first.count() == 2);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).first.test(1));
    CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).first.test(2));
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[1])) !=
          colorSets.end());

    // Third tile has two non-transparent colors, CYAN and GREEN
    CHECK(std::get<0>(indexedNormTilesWithColorSets[2]).tileIndex == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).keyFrame().colorIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).keyFrame().colorIndexes[7] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).keyFrame().colorIndexes[56] == 1);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).keyFrame().colorIndexes[63] == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.size == 3);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[2] ==
          porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[2]).vFlip);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).hFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).first.count() == 2);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).first.test(1));
    CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).first.test(3));
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[2])) !=
          colorSets.end());

    // Fourth tile has 1 non-transparent color, color should be BLUE
    CHECK(std::get<0>(indexedNormTilesWithColorSets[3]).tileIndex == 3);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).keyFrame().colorIndexes[0] == 0);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).keyFrame().colorIndexes[7] == 1);
    for (int i = 56; i <= 63; i++) {
        CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).keyFrame().colorIndexes[i] == 1);
    }
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.size == 2);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.colors[0] ==
          porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.colors[1] ==
          porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).hFlip);
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).vFlip);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[3]).first.count() == 1);
    CHECK(std::get<2>(indexedNormTilesWithColorSets[3]).first.test(0));
    CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[3])) !=
          colorSets.end());
}

TEST_CASE("assign should correctly assign all normalized palettes or fail if impossible") {
    SUBCASE("It should successfully allocate a simple 2x2 tileset png") {
        constexpr int SOLUTION_SIZE = 2;
        porytiles::PorytilesContext ctx{};
        ctx.fieldmapConfig.numPalettesInPrimary = SOLUTION_SIZE;
        ctx.compilerConfig.primaryExploredNodeCutoff = 20;

        REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
        png::image<png::rgba_pixel> png1{"Resources/Tests/2x2_pattern_2.png"};
        porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
        auto [indexedNormTiles, _1, _2] =
            porytiles::normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});
        auto [colorToIndex, indexToColor] =
            porytiles::buildColorIndexMaps(ctx, porytiles::CompilerMode::PRIMARY, indexedNormTiles, {}, {}, {});
        auto [indexedNormTilesWithColorSets, colorSets, _3, _4] =
            porytiles::matchNormalizedWithColorSets(colorToIndex, indexedNormTiles, {}, {});

        // Set up the state struct
        std::vector<ColorSet> solution;
        solution.reserve(SOLUTION_SIZE);
        std::vector<ColorSet> hardwarePalettes;
        hardwarePalettes.resize(SOLUTION_SIZE);
        std::vector<ColorSet> unassigned;
        std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
        std::stable_sort(std::begin(unassigned), std::end(unassigned),
                         [](const auto &cs1, const auto &cs2) { return cs1.first.count() < cs2.first.count(); });
        porytiles::AssignState state = {hardwarePalettes, unassigned.size(), 0};

        CHECK(porytiles::assignDepthFirst(ctx, porytiles::CompilerMode::PRIMARY, state, solution, {}, unassigned, {}) ==
              porytiles::AssignResult::SUCCESS);
        CHECK(solution.size() == SOLUTION_SIZE);
        CHECK(solution.at(0).first.count() == 1);
        CHECK(solution.at(1).first.count() == 3);
        CHECK(solution.at(0).first.test(0));
        CHECK(solution.at(1).first.test(1));
        CHECK(solution.at(1).first.test(2));
        CHECK(solution.at(1).first.test(3));
    }

    SUBCASE("It should successfully allocate a large, complex PNG") {
        constexpr int SOLUTION_SIZE = 5;
        porytiles::PorytilesContext ctx{};

        REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/compile_raw_set_1/set.png"}));
        png::image<png::rgba_pixel> png1{"Resources/Tests/compile_raw_set_1/set.png"};
        porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
        auto [indexedNormTiles, _1, _2] =
            porytiles::normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});
        auto [colorToIndex, indexToColor] =
            porytiles::buildColorIndexMaps(ctx, porytiles::CompilerMode::PRIMARY, indexedNormTiles, {}, {}, {});
        auto [indexedNormTilesWithColorSets, colorSets, _3, _4] =
            porytiles::matchNormalizedWithColorSets(colorToIndex, indexedNormTiles, {}, {});

        // Set up the state struct
        std::vector<ColorSet> solution;
        solution.reserve(SOLUTION_SIZE);
        std::vector<ColorSet> hardwarePalettes;
        hardwarePalettes.resize(SOLUTION_SIZE);
        std::vector<ColorSet> unassigned;
        std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
        std::stable_sort(std::begin(unassigned), std::end(unassigned),
                         [](const auto &cs1, const auto &cs2) { return cs1.first.count() < cs2.first.count(); });
        porytiles::AssignState state = {hardwarePalettes, unassigned.size(), 0};

        CHECK(porytiles::assignDepthFirst(ctx, porytiles::CompilerMode::PRIMARY, state, solution, {}, unassigned, {}) ==
              porytiles::AssignResult::SUCCESS);
        CHECK(solution.size() == SOLUTION_SIZE);
        CHECK(solution.at(0).first.count() == 11);
        CHECK(solution.at(1).first.count() == 12);
        CHECK(solution.at(2).first.count() == 14);
        CHECK(solution.at(3).first.count() == 14);
        CHECK(solution.at(4).first.count() == 15);
    }
}

TEST_CASE("makeTile should create the expected GBATile from the given NormalizedTile and GBAPalette") {
    porytiles::PorytilesContext ctx{};
    ctx.compilerConfig.transparencyColor = porytiles::RGBA_MAGENTA;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numTilesInPrimary = 4;
    ctx.compilerConfig.primaryExploredNodeCutoff = 5;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
    auto [indexedNormTiles, _1, _2] = normalizeDecompTiles(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {});
    auto compiledTiles = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {}, {});

    porytiles::GBATile tile0 = porytiles::makeTile(
        indexedNormTiles[0].second, porytiles::NormalizedTile::keyFrameIndex(), compiledTiles->palettes[0]);
    CHECK_FALSE(indexedNormTiles[0].second.hFlip);
    CHECK(indexedNormTiles[0].second.vFlip);
    CHECK(tile0.colorIndexes[0] == 0);
    CHECK(tile0.colorIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile0.colorIndexes[i] == 1);
    }

    porytiles::GBATile tile1 = porytiles::makeTile(
        indexedNormTiles[1].second, porytiles::NormalizedTile::keyFrameIndex(), compiledTiles->palettes[1]);
    CHECK_FALSE(indexedNormTiles[1].second.hFlip);
    CHECK_FALSE(indexedNormTiles[1].second.vFlip);
    CHECK(tile1.colorIndexes[0] == 0);
    CHECK(tile1.colorIndexes[54] == 1);
    CHECK(tile1.colorIndexes[55] == 1);
    CHECK(tile1.colorIndexes[62] == 1);
    CHECK(tile1.colorIndexes[63] == 2);

    porytiles::GBATile tile2 = porytiles::makeTile(
        indexedNormTiles[2].second, porytiles::NormalizedTile::keyFrameIndex(), compiledTiles->palettes[1]);
    CHECK(indexedNormTiles[2].second.hFlip);
    CHECK_FALSE(indexedNormTiles[2].second.vFlip);
    CHECK(tile2.colorIndexes[0] == 0);
    CHECK(tile2.colorIndexes[7] == 3);
    CHECK(tile2.colorIndexes[56] == 3);
    CHECK(tile2.colorIndexes[63] == 1);

    porytiles::GBATile tile3 = porytiles::makeTile(
        indexedNormTiles[3].second, porytiles::NormalizedTile::keyFrameIndex(), compiledTiles->palettes[0]);
    CHECK(indexedNormTiles[3].second.hFlip);
    CHECK(indexedNormTiles[3].second.vFlip);
    CHECK(tile3.colorIndexes[0] == 0);
    CHECK(tile3.colorIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile3.colorIndexes[i] == 1);
    }
}

TEST_CASE("compile simple example should perform as expected") {
    porytiles::PorytilesContext ctx{};
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numTilesInPrimary = 4;
    ctx.compilerConfig.primaryExploredNodeCutoff = 5;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/2x2_pattern_2.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, porytiles::CompilerMode::PRIMARY, png1);
    auto compiledTiles = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, tiles, {}, {}, {});

    // Check that compiled palettes are as expected
    CHECK(compiledTiles->palettes.at(0).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledTiles->palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledTiles->palettes.at(1).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledTiles->palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledTiles->palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledTiles->palettes.at(1).colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));

    /*
     * Check that compiled GBATiles have expected index values, there are only 3 in final tileset (ignoring the
     * transparent tile at the start) since two of the original tiles are flips of each other.
     */
    porytiles::GBATile &tile0 = compiledTiles->tiles[0];
    for (size_t i = 0; i < 64; i++) {
        CHECK(tile0.colorIndexes[i] == 0);
    }

    porytiles::GBATile &tile1 = compiledTiles->tiles[1];
    CHECK(tile1.colorIndexes[0] == 0);
    CHECK(tile1.colorIndexes[7] == 1);
    for (size_t i = 56; i < 64; i++) {
        CHECK(tile1.colorIndexes[i] == 1);
    }

    porytiles::GBATile tile2 = compiledTiles->tiles[2];
    CHECK(tile2.colorIndexes[0] == 0);
    CHECK(tile2.colorIndexes[54] == 1);
    CHECK(tile2.colorIndexes[55] == 1);
    CHECK(tile2.colorIndexes[62] == 1);
    CHECK(tile2.colorIndexes[63] == 2);

    porytiles::GBATile tile3 = compiledTiles->tiles[3];
    CHECK(tile3.colorIndexes[0] == 0);
    CHECK(tile3.colorIndexes[7] == 3);
    CHECK(tile3.colorIndexes[56] == 3);
    CHECK(tile3.colorIndexes[63] == 1);

    /*
     * Check that all the metatileEntries are correct.
     */
    CHECK(compiledTiles->metatileEntries[0].tileIndex == 1);
    CHECK(compiledTiles->metatileEntries[0].paletteIndex == 0);
    CHECK_FALSE(compiledTiles->metatileEntries[0].hFlip);
    CHECK(compiledTiles->metatileEntries[0].vFlip);

    CHECK(compiledTiles->metatileEntries[1].tileIndex == 2);
    CHECK(compiledTiles->metatileEntries[1].paletteIndex == 1);
    CHECK_FALSE(compiledTiles->metatileEntries[1].hFlip);
    CHECK_FALSE(compiledTiles->metatileEntries[1].vFlip);

    CHECK(compiledTiles->metatileEntries[2].tileIndex == 3);
    CHECK(compiledTiles->metatileEntries[2].paletteIndex == 1);
    CHECK(compiledTiles->metatileEntries[2].hFlip);
    CHECK_FALSE(compiledTiles->metatileEntries[2].vFlip);

    CHECK(compiledTiles->metatileEntries[3].tileIndex == 1);
    CHECK(compiledTiles->metatileEntries[3].paletteIndex == 0);
    CHECK(compiledTiles->metatileEntries[3].hFlip);
    CHECK(compiledTiles->metatileEntries[3].vFlip);
}

TEST_CASE("compile function should fill out primary CompiledTileset struct with expected values") {
    porytiles::PorytilesContext ctx{};
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/simple_metatiles_3/primary/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/simple_metatiles_3/primary/middle.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/simple_metatiles_3/primary/top.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    auto compiledPrimary = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {}, {}, {});

    // Check that tiles are as expected
    CHECK(compiledPrimary->tiles.size() == 16);
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/expected_tiles.png"}));
    png::image<png::index_pixel> expectedPng{"Resources/Tests/simple_metatiles_3/primary/expected_tiles.png"};
    for (std::size_t tileIndex = 0; tileIndex < compiledPrimary->tiles.size(); tileIndex++) {
        for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH_PIX; row++) {
            for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH_PIX; col++) {
                CHECK(compiledPrimary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH_PIX)] ==
                      expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH_PIX)]);
            }
        }
    }

    // Check that paletteIndexesOfTile are correct
    CHECK(compiledPrimary->paletteIndexesOfTile.size() == 16);
    CHECK(compiledPrimary->paletteIndexesOfTile[0] == 0);
    CHECK(compiledPrimary->paletteIndexesOfTile[1] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[2] == 1);
    CHECK(compiledPrimary->paletteIndexesOfTile[3] == 1);
    CHECK(compiledPrimary->paletteIndexesOfTile[4] == 0);

    // Check that compiled palettes are as expected
    CHECK(compiledPrimary->palettes.size() == ctx.fieldmapConfig.numPalettesInPrimary);
    CHECK(compiledPrimary->palettes.at(0).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledPrimary->palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
    CHECK(compiledPrimary->palettes.at(1).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledPrimary->palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledPrimary->palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledPrimary->palettes.at(2).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledPrimary->palettes.at(2).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledPrimary->palettes.at(2).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));

    // Check that all metatile entries are correct
    CHECK(compiledPrimary->metatileEntries.size() ==
          porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

    CHECK(compiledPrimary->metatileEntries[0].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[0].vFlip);
    CHECK(compiledPrimary->metatileEntries[0].tileIndex == 1);
    CHECK(compiledPrimary->metatileEntries[0].paletteIndex == 2);

    CHECK_FALSE(compiledPrimary->metatileEntries[1].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[1].vFlip);
    CHECK(compiledPrimary->metatileEntries[1].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[1].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[2].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[2].vFlip);
    CHECK(compiledPrimary->metatileEntries[2].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[2].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[3].hFlip);
    CHECK(compiledPrimary->metatileEntries[3].vFlip);
    CHECK(compiledPrimary->metatileEntries[3].tileIndex == 2);
    CHECK(compiledPrimary->metatileEntries[3].paletteIndex == 1);

    CHECK_FALSE(compiledPrimary->metatileEntries[4].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[4].vFlip);
    CHECK(compiledPrimary->metatileEntries[4].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[4].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[5].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[5].vFlip);
    CHECK(compiledPrimary->metatileEntries[5].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[5].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[6].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[6].vFlip);
    CHECK(compiledPrimary->metatileEntries[6].tileIndex == 3);
    CHECK(compiledPrimary->metatileEntries[6].paletteIndex == 1);

    CHECK_FALSE(compiledPrimary->metatileEntries[7].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[7].vFlip);
    CHECK(compiledPrimary->metatileEntries[7].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[7].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[8].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[8].vFlip);
    CHECK(compiledPrimary->metatileEntries[8].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[8].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[9].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[9].vFlip);
    CHECK(compiledPrimary->metatileEntries[9].tileIndex == 4);
    CHECK(compiledPrimary->metatileEntries[9].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[10].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[10].vFlip);
    CHECK(compiledPrimary->metatileEntries[10].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[10].paletteIndex == 0);

    CHECK_FALSE(compiledPrimary->metatileEntries[11].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[11].vFlip);
    CHECK(compiledPrimary->metatileEntries[11].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[11].paletteIndex == 0);

    for (std::size_t index = ctx.fieldmapConfig.numTilesPerMetatile;
         index < porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile; index++) {
        CHECK_FALSE(compiledPrimary->metatileEntries[index].hFlip);
        CHECK_FALSE(compiledPrimary->metatileEntries[index].vFlip);
        CHECK(compiledPrimary->metatileEntries[index].tileIndex == 0);
        CHECK(compiledPrimary->metatileEntries[index].paletteIndex == 0);
    }

    // Check that colorIndexMap is correct
    CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 0);
    CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_YELLOW)] == 1);
    CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 2);
    CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 3);
    CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_WHITE)] == 4);

    // Check that tileIndexes is correct
    CHECK(compiledPrimary->tileIndexes.size() == 5);
    CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[0]] == 0);
    CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[1]] == 1);
    CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[2]] == 2);
    CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[3]] == 3);
    CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[4]] == 4);
}

TEST_CASE("compile function should fill out secondary CompiledTileset struct with expected values") {
    porytiles::PorytilesContext ctx{};
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/primary/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/simple_metatiles_3/primary/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/simple_metatiles_3/primary/middle.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/simple_metatiles_3/primary/top.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    ctx.compilerContext.pairedPrimaryTileset =
        porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {}, {}, {});

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/secondary/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/secondary/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/simple_metatiles_3/secondary/top.png"}));
    png::image<png::rgba_pixel> bottomSecondary{"Resources/Tests/simple_metatiles_3/secondary/bottom.png"};
    png::image<png::rgba_pixel> middleSecondary{"Resources/Tests/simple_metatiles_3/secondary/middle.png"};
    png::image<png::rgba_pixel> topSecondary{"Resources/Tests/simple_metatiles_3/secondary/top.png"};
    porytiles::DecompiledTileset decompiledSecondary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::SECONDARY, std::unordered_map<std::size_t, porytiles::Attributes>{},
        bottomSecondary, middleSecondary, topSecondary);
    auto compiledSecondary =
        porytiles::compile(ctx, porytiles::CompilerMode::SECONDARY, decompiledSecondary, {}, {}, {});

    // Check that tiles are as expected
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Tests/simple_metatiles_3/secondary/expected_tiles.png"}));
    png::image<png::index_pixel> expectedPng{"Resources/Tests/simple_metatiles_3/secondary/expected_tiles.png"};
    for (std::size_t tileIndex = 0; tileIndex < compiledSecondary->tiles.size(); tileIndex++) {
        for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH_PIX; row++) {
            for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH_PIX; col++) {
                CHECK(compiledSecondary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH_PIX)] ==
                      expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH_PIX)]);
            }
        }
    }

    // Check that paletteIndexesOfTile are correct
    CHECK(compiledSecondary->paletteIndexesOfTile[0] == 2);
    CHECK(compiledSecondary->paletteIndexesOfTile[1] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[2] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[3] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[4] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[5] == 5);

    // Check that compiled palettes are as expected
    CHECK(compiledSecondary->palettes.at(0).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledSecondary->palettes.at(0).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
    CHECK(compiledSecondary->palettes.at(1).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledSecondary->palettes.at(1).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
    CHECK(compiledSecondary->palettes.at(1).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledSecondary->palettes.at(2).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledSecondary->palettes.at(2).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
    CHECK(compiledSecondary->palettes.at(2).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
    CHECK(compiledSecondary->palettes.at(3).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledSecondary->palettes.at(3).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
    CHECK(compiledSecondary->palettes.at(3).colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
    CHECK(compiledSecondary->palettes.at(3).colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_PURPLE));
    CHECK(compiledSecondary->palettes.at(3).colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_LIME));
    CHECK(compiledSecondary->palettes.at(4).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledSecondary->palettes.at(5).colors[0] == porytiles::rgbaToBgr(ctx.compilerConfig.transparencyColor));
    CHECK(compiledSecondary->palettes.at(5).colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));

    // Check that all metatile entries are correct
    CHECK(compiledSecondary->metatileEntries.size() ==
          porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

    CHECK_FALSE(compiledSecondary->metatileEntries[0].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[0].vFlip);
    CHECK(compiledSecondary->metatileEntries[0].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[0].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary->metatileEntries[1].hFlip);
    CHECK(compiledSecondary->metatileEntries[1].vFlip);
    CHECK(compiledSecondary->metatileEntries[1].tileIndex == 0 + ctx.fieldmapConfig.numTilesInPrimary);
    CHECK(compiledSecondary->metatileEntries[1].paletteIndex == 2);

    CHECK_FALSE(compiledSecondary->metatileEntries[2].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[2].vFlip);
    CHECK(compiledSecondary->metatileEntries[2].tileIndex == 1 + ctx.fieldmapConfig.numTilesInPrimary);
    CHECK(compiledSecondary->metatileEntries[2].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary->metatileEntries[3].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[3].vFlip);
    CHECK(compiledSecondary->metatileEntries[3].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[3].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary->metatileEntries[4].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[4].vFlip);
    CHECK(compiledSecondary->metatileEntries[4].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[4].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary->metatileEntries[5].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[5].vFlip);
    CHECK(compiledSecondary->metatileEntries[5].tileIndex == 2 + ctx.fieldmapConfig.numTilesInPrimary);
    CHECK(compiledSecondary->metatileEntries[5].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary->metatileEntries[6].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[6].vFlip);
    CHECK(compiledSecondary->metatileEntries[6].tileIndex == 3 + ctx.fieldmapConfig.numTilesInPrimary);
    CHECK(compiledSecondary->metatileEntries[6].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary->metatileEntries[7].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[7].vFlip);
    CHECK(compiledSecondary->metatileEntries[7].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[7].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary->metatileEntries[8].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[8].vFlip);
    CHECK(compiledSecondary->metatileEntries[8].tileIndex == 4 + ctx.fieldmapConfig.numTilesInPrimary);
    CHECK(compiledSecondary->metatileEntries[8].paletteIndex == 3);

    CHECK_FALSE(compiledSecondary->metatileEntries[9].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[9].vFlip);
    CHECK(compiledSecondary->metatileEntries[9].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[9].paletteIndex == 0);

    CHECK_FALSE(compiledSecondary->metatileEntries[10].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[10].vFlip);
    CHECK(compiledSecondary->metatileEntries[10].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[10].paletteIndex == 0);

    CHECK(compiledSecondary->metatileEntries[11].hFlip);
    CHECK(compiledSecondary->metatileEntries[11].vFlip);
    CHECK(compiledSecondary->metatileEntries[11].tileIndex == 5 + ctx.fieldmapConfig.numTilesInPrimary);
    CHECK(compiledSecondary->metatileEntries[11].paletteIndex == 5);

    for (std::size_t index = ctx.fieldmapConfig.numTilesPerMetatile;
         index < porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile; index++) {
        CHECK_FALSE(compiledSecondary->metatileEntries[index].hFlip);
        CHECK_FALSE(compiledSecondary->metatileEntries[index].vFlip);
        CHECK(compiledSecondary->metatileEntries[index].tileIndex == 0);
        CHECK(compiledSecondary->metatileEntries[index].paletteIndex == 0);
    }

    // Check that colorIndexMap is correct
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 0);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_YELLOW)] == 1);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 2);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 3);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_WHITE)] == 4);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 5);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_PURPLE)] == 6);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_LIME)] == 7);
    CHECK(compiledSecondary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREY)] == 8);

    // Check that tileIndexes is correct
    CHECK(compiledSecondary->tileIndexes.size() == 6);
    CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[0]] == 0);
    CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[1]] == 1);
    CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[2]] == 2);
    CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[3]] == 3);
    CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[4]] == 4);
    CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[5]] == 5);
}

TEST_CASE("compile function should correctly compile primary set with animated tiles") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/anim_metatiles_1/primary/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/anim_metatiles_1/primary/middle.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/anim_metatiles_1/primary/top.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/anim/water"}));

    porytiles::AnimationPng<png::rgba_pixel> flowerWhiteKey{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/key.png"},
        "flower_white", "key.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerWhite00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/0.png"}, "flower_white",
        "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerWhite01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/1.png"}, "flower_white",
        "01.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerWhite02{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/2.png"}, "flower_white",
        "02.png"};
    porytiles::AnimationPng<png::rgba_pixel> waterKey{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/water/key.png"}, "water", "key.png"};
    porytiles::AnimationPng<png::rgba_pixel> water00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/water/0.png"}, "water", "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> water01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/water/1.png"}, "water", "01.png"};

    std::vector<porytiles::AnimationPng<png::rgba_pixel>> flowerWhiteAnim{};
    std::vector<porytiles::AnimationPng<png::rgba_pixel>> waterAnim{};

    flowerWhiteAnim.push_back(flowerWhiteKey);
    flowerWhiteAnim.push_back(flowerWhite00);
    flowerWhiteAnim.push_back(flowerWhite01);
    flowerWhiteAnim.push_back(flowerWhite02);

    waterAnim.push_back(waterKey);
    waterAnim.push_back(water00);
    waterAnim.push_back(water01);

    std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
    anims.push_back(flowerWhiteAnim);
    anims.push_back(waterAnim);

    porytiles::importAnimTiles(ctx, porytiles::CompilerMode::PRIMARY, anims, decompiledPrimary);

    auto compiledPrimary = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {}, {}, {});

    CHECK(compiledPrimary->tiles.size() == 16);

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/expected_tiles.png"}));
    png::image<png::index_pixel> expectedPng{"Resources/Tests/anim_metatiles_1/primary/expected_tiles.png"};
    for (std::size_t tileIndex = 0; tileIndex < compiledPrimary->tiles.size(); tileIndex++) {
        for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH_PIX; row++) {
            for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH_PIX; col++) {
                CHECK(compiledPrimary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH_PIX)] ==
                      expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH_PIX)]);
            }
        }
    }

    // Check that paletteIndexesOfTile is correct
    CHECK(compiledPrimary->paletteIndexesOfTile.size() == 16);
    CHECK(compiledPrimary->paletteIndexesOfTile[0] == 0);
    CHECK(compiledPrimary->paletteIndexesOfTile[1] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[2] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[3] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[4] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[5] == 1);
    CHECK(compiledPrimary->paletteIndexesOfTile[6] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[7] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[8] == 2);
    CHECK(compiledPrimary->paletteIndexesOfTile[9] == 2);

    // Check that all metatile entries are correct
    CHECK(compiledPrimary->metatileEntries.size() ==
          porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

    // Metatile 0 bottom
    CHECK_FALSE(compiledPrimary->metatileEntries[0].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[0].vFlip);
    CHECK(compiledPrimary->metatileEntries[0].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[0].paletteIndex == 0);
    CHECK_FALSE(compiledPrimary->metatileEntries[1].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[1].vFlip);
    CHECK(compiledPrimary->metatileEntries[1].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[1].paletteIndex == 0);
    CHECK_FALSE(compiledPrimary->metatileEntries[2].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[2].vFlip);
    CHECK(compiledPrimary->metatileEntries[2].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[2].paletteIndex == 0);
    CHECK_FALSE(compiledPrimary->metatileEntries[3].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[3].vFlip);
    CHECK(compiledPrimary->metatileEntries[3].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[3].paletteIndex == 0);
    // Metatile 0 middle
    CHECK(compiledPrimary->metatileEntries[4].hFlip);
    CHECK(compiledPrimary->metatileEntries[4].vFlip);
    CHECK(compiledPrimary->metatileEntries[4].tileIndex == 6);
    CHECK(compiledPrimary->metatileEntries[4].paletteIndex == 2);
    CHECK(compiledPrimary->metatileEntries[5].hFlip);
    CHECK(compiledPrimary->metatileEntries[5].vFlip);
    CHECK(compiledPrimary->metatileEntries[5].tileIndex == 7);
    CHECK(compiledPrimary->metatileEntries[5].paletteIndex == 2);
    CHECK_FALSE(compiledPrimary->metatileEntries[6].hFlip);
    CHECK(compiledPrimary->metatileEntries[6].vFlip);
    CHECK(compiledPrimary->metatileEntries[6].tileIndex == 8);
    CHECK(compiledPrimary->metatileEntries[6].paletteIndex == 2);
    CHECK(compiledPrimary->metatileEntries[7].hFlip);
    CHECK(compiledPrimary->metatileEntries[7].vFlip);
    CHECK(compiledPrimary->metatileEntries[7].tileIndex == 9);
    CHECK(compiledPrimary->metatileEntries[7].paletteIndex == 2);
    // Metatile 0 top
    CHECK_FALSE(compiledPrimary->metatileEntries[8].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[8].vFlip);
    CHECK(compiledPrimary->metatileEntries[8].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[8].paletteIndex == 0);
    CHECK_FALSE(compiledPrimary->metatileEntries[9].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[9].vFlip);
    CHECK(compiledPrimary->metatileEntries[9].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[9].paletteIndex == 0);
    CHECK_FALSE(compiledPrimary->metatileEntries[10].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[10].vFlip);
    CHECK(compiledPrimary->metatileEntries[10].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[10].paletteIndex == 0);
    CHECK_FALSE(compiledPrimary->metatileEntries[11].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[11].vFlip);
    CHECK(compiledPrimary->metatileEntries[11].tileIndex == 0);
    CHECK(compiledPrimary->metatileEntries[11].paletteIndex == 0);

    // Metatile 1 bottom
    CHECK(compiledPrimary->metatileEntries[12].hFlip);
    CHECK(compiledPrimary->metatileEntries[12].vFlip);
    CHECK(compiledPrimary->metatileEntries[12].tileIndex == 6);
    CHECK(compiledPrimary->metatileEntries[12].paletteIndex == 2);
    CHECK(compiledPrimary->metatileEntries[13].hFlip);
    CHECK(compiledPrimary->metatileEntries[13].vFlip);
    CHECK(compiledPrimary->metatileEntries[13].tileIndex == 7);
    CHECK(compiledPrimary->metatileEntries[13].paletteIndex == 2);
    CHECK_FALSE(compiledPrimary->metatileEntries[14].hFlip);
    CHECK(compiledPrimary->metatileEntries[14].vFlip);
    CHECK(compiledPrimary->metatileEntries[14].tileIndex == 8);
    CHECK(compiledPrimary->metatileEntries[14].paletteIndex == 2);
    CHECK(compiledPrimary->metatileEntries[15].hFlip);
    CHECK(compiledPrimary->metatileEntries[15].vFlip);
    CHECK(compiledPrimary->metatileEntries[15].tileIndex == 9);
    CHECK(compiledPrimary->metatileEntries[15].paletteIndex == 2);
    // Metatile 1 middle
    CHECK_FALSE(compiledPrimary->metatileEntries[16].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[16].vFlip);
    CHECK(compiledPrimary->metatileEntries[16].tileIndex == 1);
    CHECK(compiledPrimary->metatileEntries[16].paletteIndex == 2);
    CHECK_FALSE(compiledPrimary->metatileEntries[17].hFlip);
    CHECK_FALSE(compiledPrimary->metatileEntries[17].vFlip);
    CHECK(compiledPrimary->metatileEntries[17].tileIndex == 2);
    CHECK(compiledPrimary->metatileEntries[17].paletteIndex == 2);
    CHECK_FALSE(compiledPrimary->metatileEntries[18].hFlip);
    CHECK(compiledPrimary->metatileEntries[18].vFlip);
    CHECK(compiledPrimary->metatileEntries[18].tileIndex == 3);
    CHECK(compiledPrimary->metatileEntries[18].paletteIndex == 2);
    CHECK(compiledPrimary->metatileEntries[19].hFlip);
    CHECK(compiledPrimary->metatileEntries[19].vFlip);
    CHECK(compiledPrimary->metatileEntries[19].tileIndex == 4);
    CHECK(compiledPrimary->metatileEntries[19].paletteIndex == 2);
    // Metatile 1 top is blank, don't bother testing

    // Metatile 2 bottom is blank, don't bother testing
    // Metatile 2 middle
    CHECK_FALSE(compiledPrimary->metatileEntries[28].hFlip);
    CHECK(compiledPrimary->metatileEntries[28].vFlip);
    CHECK(compiledPrimary->metatileEntries[28].tileIndex == 5);
    CHECK(compiledPrimary->metatileEntries[28].paletteIndex == 1);
    CHECK_FALSE(compiledPrimary->metatileEntries[29].hFlip);
    CHECK(compiledPrimary->metatileEntries[29].vFlip);
    CHECK(compiledPrimary->metatileEntries[29].tileIndex == 5);
    CHECK(compiledPrimary->metatileEntries[29].paletteIndex == 1);
    CHECK_FALSE(compiledPrimary->metatileEntries[30].hFlip);
    CHECK(compiledPrimary->metatileEntries[30].vFlip);
    CHECK(compiledPrimary->metatileEntries[30].tileIndex == 5);
    CHECK(compiledPrimary->metatileEntries[30].paletteIndex == 1);
    CHECK_FALSE(compiledPrimary->metatileEntries[31].hFlip);
    CHECK(compiledPrimary->metatileEntries[31].vFlip);
    CHECK(compiledPrimary->metatileEntries[31].tileIndex == 5);
    CHECK(compiledPrimary->metatileEntries[31].paletteIndex == 1);
    // Metatile 2 top is blank, don't bother testing

    // Verify integrity of anims structure
    CHECK(compiledPrimary->anims.size() == 2);

    CHECK(compiledPrimary->anims.at(0).frames.size() == 4);
    CHECK(compiledPrimary->anims.at(0).frames.at(0).tiles.size() == 4);
    CHECK(compiledPrimary->anims.at(0).frames.at(1).tiles.size() == 4);
    CHECK(compiledPrimary->anims.at(0).frames.at(2).tiles.size() == 4);
    CHECK(compiledPrimary->anims.at(0).frames.at(3).tiles.size() == 4);

    CHECK(compiledPrimary->anims.at(1).frames.size() == 3);
    CHECK(compiledPrimary->anims.at(1).frames.at(0).tiles.size() == 1);
    CHECK(compiledPrimary->anims.at(1).frames.at(1).tiles.size() == 1);
    CHECK(compiledPrimary->anims.at(1).frames.at(2).tiles.size() == 1);
}

TEST_CASE("compile function should correctly compile secondary set with animated tiles") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/anim_metatiles_1/primary/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/anim_metatiles_1/primary/middle.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/anim_metatiles_1/primary/top.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/primary/anim/water"}));

    porytiles::AnimationPng<png::rgba_pixel> flowerWhiteKey{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/key.png"},
        "flower_white", "key.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerWhite00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/0.png"}, "flower_white",
        "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerWhite01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/1.png"}, "flower_white",
        "01.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerWhite02{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/flower_white/2.png"}, "flower_white",
        "02.png"};
    porytiles::AnimationPng<png::rgba_pixel> waterKey{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/water/key.png"}, "water", "key.png"};
    porytiles::AnimationPng<png::rgba_pixel> water00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/water/0.png"}, "water", "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> water01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/primary/anim/water/1.png"}, "water", "01.png"};

    std::vector<porytiles::AnimationPng<png::rgba_pixel>> flowerWhiteAnim{};
    std::vector<porytiles::AnimationPng<png::rgba_pixel>> waterAnim{};

    flowerWhiteAnim.push_back(flowerWhiteKey);
    flowerWhiteAnim.push_back(flowerWhite00);
    flowerWhiteAnim.push_back(flowerWhite01);
    flowerWhiteAnim.push_back(flowerWhite02);

    waterAnim.push_back(waterKey);
    waterAnim.push_back(water00);
    waterAnim.push_back(water01);

    std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
    anims.push_back(flowerWhiteAnim);
    anims.push_back(waterAnim);

    porytiles::importAnimTiles(ctx, porytiles::CompilerMode::PRIMARY, anims, decompiledPrimary);

    ctx.compilerContext.pairedPrimaryTileset =
        porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {}, {}, {});

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/secondary/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/secondary/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/secondary/top.png"}));
    png::image<png::rgba_pixel> bottomSecondary{"Resources/Tests/anim_metatiles_1/secondary/bottom.png"};
    png::image<png::rgba_pixel> middleSecondary{"Resources/Tests/anim_metatiles_1/secondary/middle.png"};
    png::image<png::rgba_pixel> topSecondary{"Resources/Tests/anim_metatiles_1/secondary/top.png"};
    porytiles::DecompiledTileset decompiledSecondary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::SECONDARY, std::unordered_map<std::size_t, porytiles::Attributes>{},
        bottomSecondary, middleSecondary, topSecondary);

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/anim_metatiles_1/secondary/anim/flower_red"}));

    porytiles::AnimationPng<png::rgba_pixel> flowerRedKey{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/secondary/anim/flower_red/key.png"},
        "flower_white", "key.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerRed00{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/secondary/anim/flower_red/0.png"}, "flower_white",
        "00.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerRed01{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/secondary/anim/flower_red/1.png"}, "flower_white",
        "01.png"};
    porytiles::AnimationPng<png::rgba_pixel> flowerRed02{
        png::image<png::rgba_pixel>{"Resources/Tests/anim_metatiles_1/secondary/anim/flower_red/2.png"}, "flower_white",
        "02.png"};

    std::vector<porytiles::AnimationPng<png::rgba_pixel>> flowerRedAnim{};

    flowerRedAnim.push_back(flowerRedKey);
    flowerRedAnim.push_back(flowerRed00);
    flowerRedAnim.push_back(flowerRed01);
    flowerRedAnim.push_back(flowerRed02);

    std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> animsSecondary{};
    animsSecondary.push_back(flowerRedAnim);

    porytiles::importAnimTiles(ctx, porytiles::CompilerMode::SECONDARY, animsSecondary, decompiledSecondary);

    auto compiledSecondary =
        porytiles::compile(ctx, porytiles::CompilerMode::SECONDARY, decompiledSecondary, {}, {}, {});

    CHECK(compiledSecondary->tiles.size() == 16);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Tests/anim_metatiles_1/secondary/expected_tiles.png"}));
    png::image<png::index_pixel> expectedPng{"Resources/Tests/anim_metatiles_1/secondary/expected_tiles.png"};
    for (std::size_t tileIndex = 0; tileIndex < compiledSecondary->tiles.size(); tileIndex++) {
        for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH_PIX; row++) {
            for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH_PIX; col++) {
                CHECK(compiledSecondary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH_PIX)] ==
                      expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH_PIX)]);
            }
        }
    }

    // Check that paletteIndexesOfTile is correct
    CHECK(compiledSecondary->paletteIndexesOfTile.size() == 16);
    CHECK(compiledSecondary->paletteIndexesOfTile[0] == 5);
    CHECK(compiledSecondary->paletteIndexesOfTile[1] == 5);
    CHECK(compiledSecondary->paletteIndexesOfTile[2] == 5);
    CHECK(compiledSecondary->paletteIndexesOfTile[3] == 5);
    CHECK(compiledSecondary->paletteIndexesOfTile[4] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[5] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[6] == 3);
    CHECK(compiledSecondary->paletteIndexesOfTile[7] == 3);

    // Check that all metatile entries are correct
    CHECK(compiledSecondary->metatileEntries.size() ==
          porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

    // Metatile 0 bottom
    CHECK_FALSE(compiledSecondary->metatileEntries[0].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[0].vFlip);
    CHECK(compiledSecondary->metatileEntries[0].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[0].paletteIndex == 0);
    CHECK_FALSE(compiledSecondary->metatileEntries[1].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[1].vFlip);
    CHECK(compiledSecondary->metatileEntries[1].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[1].paletteIndex == 0);
    CHECK_FALSE(compiledSecondary->metatileEntries[2].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[2].vFlip);
    CHECK(compiledSecondary->metatileEntries[2].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[2].paletteIndex == 0);
    CHECK_FALSE(compiledSecondary->metatileEntries[3].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[3].vFlip);
    CHECK(compiledSecondary->metatileEntries[3].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[3].paletteIndex == 0);
    // Metatile 0 middle
    CHECK_FALSE(compiledSecondary->metatileEntries[4].hFlip);
    CHECK(compiledSecondary->metatileEntries[4].vFlip);
    CHECK(compiledSecondary->metatileEntries[4].tileIndex == 5);
    CHECK(compiledSecondary->metatileEntries[4].paletteIndex == 1);
    CHECK_FALSE(compiledSecondary->metatileEntries[5].hFlip);
    CHECK(compiledSecondary->metatileEntries[5].vFlip);
    CHECK(compiledSecondary->metatileEntries[5].tileIndex == 5);
    CHECK(compiledSecondary->metatileEntries[5].paletteIndex == 1);
    CHECK_FALSE(compiledSecondary->metatileEntries[6].hFlip);
    CHECK(compiledSecondary->metatileEntries[6].vFlip);
    CHECK(compiledSecondary->metatileEntries[6].tileIndex == 5);
    CHECK(compiledSecondary->metatileEntries[6].paletteIndex == 1);
    CHECK_FALSE(compiledSecondary->metatileEntries[7].hFlip);
    CHECK(compiledSecondary->metatileEntries[7].vFlip);
    CHECK(compiledSecondary->metatileEntries[7].tileIndex == 5);
    CHECK(compiledSecondary->metatileEntries[7].paletteIndex == 1);
    // Metatile 0 top
    CHECK_FALSE(compiledSecondary->metatileEntries[8].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[8].vFlip);
    CHECK(compiledSecondary->metatileEntries[8].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[8].paletteIndex == 0);
    CHECK_FALSE(compiledSecondary->metatileEntries[9].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[9].vFlip);
    CHECK(compiledSecondary->metatileEntries[9].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[9].paletteIndex == 0);
    CHECK_FALSE(compiledSecondary->metatileEntries[10].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[10].vFlip);
    CHECK(compiledSecondary->metatileEntries[10].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[10].paletteIndex == 0);
    CHECK_FALSE(compiledSecondary->metatileEntries[11].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[11].vFlip);
    CHECK(compiledSecondary->metatileEntries[11].tileIndex == 0);
    CHECK(compiledSecondary->metatileEntries[11].paletteIndex == 0);

    // Metatile 1 bottom
    CHECK(compiledSecondary->metatileEntries[12].hFlip);
    CHECK(compiledSecondary->metatileEntries[12].vFlip);
    CHECK(compiledSecondary->metatileEntries[12].tileIndex == 6);
    CHECK(compiledSecondary->metatileEntries[12].paletteIndex == 2);
    CHECK(compiledSecondary->metatileEntries[13].hFlip);
    CHECK(compiledSecondary->metatileEntries[13].vFlip);
    CHECK(compiledSecondary->metatileEntries[13].tileIndex == 7);
    CHECK(compiledSecondary->metatileEntries[13].paletteIndex == 2);
    CHECK_FALSE(compiledSecondary->metatileEntries[14].hFlip);
    CHECK(compiledSecondary->metatileEntries[14].vFlip);
    CHECK(compiledSecondary->metatileEntries[14].tileIndex == 8);
    CHECK(compiledSecondary->metatileEntries[14].paletteIndex == 2);
    CHECK(compiledSecondary->metatileEntries[15].hFlip);
    CHECK(compiledSecondary->metatileEntries[15].vFlip);
    CHECK(compiledSecondary->metatileEntries[15].tileIndex == 9);
    CHECK(compiledSecondary->metatileEntries[15].paletteIndex == 2);
    // Metatile 1 middle
    CHECK_FALSE(compiledSecondary->metatileEntries[16].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[16].vFlip);
    CHECK(compiledSecondary->metatileEntries[16].tileIndex == 512);
    CHECK(compiledSecondary->metatileEntries[16].paletteIndex == 5);
    CHECK(compiledSecondary->metatileEntries[17].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[17].vFlip);
    CHECK(compiledSecondary->metatileEntries[17].tileIndex == 513);
    CHECK(compiledSecondary->metatileEntries[17].paletteIndex == 5);
    CHECK_FALSE(compiledSecondary->metatileEntries[18].hFlip);
    CHECK(compiledSecondary->metatileEntries[18].vFlip);
    CHECK(compiledSecondary->metatileEntries[18].tileIndex == 514);
    CHECK(compiledSecondary->metatileEntries[18].paletteIndex == 5);
    CHECK(compiledSecondary->metatileEntries[19].hFlip);
    CHECK(compiledSecondary->metatileEntries[19].vFlip);
    CHECK(compiledSecondary->metatileEntries[19].tileIndex == 515);
    CHECK(compiledSecondary->metatileEntries[19].paletteIndex == 5);
    // Metatile 1 top is blank, don't bother testing

    // Metatile 2 bottom is blank, don't bother testing
    // Metatile 2 middle
    CHECK_FALSE(compiledSecondary->metatileEntries[28].hFlip);
    CHECK(compiledSecondary->metatileEntries[28].vFlip);
    CHECK(compiledSecondary->metatileEntries[28].tileIndex == 516);
    CHECK(compiledSecondary->metatileEntries[28].paletteIndex == 3);
    CHECK_FALSE(compiledSecondary->metatileEntries[29].hFlip);
    CHECK(compiledSecondary->metatileEntries[29].vFlip);
    CHECK(compiledSecondary->metatileEntries[29].tileIndex == 517);
    CHECK(compiledSecondary->metatileEntries[29].paletteIndex == 3);
    CHECK_FALSE(compiledSecondary->metatileEntries[30].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[30].vFlip);
    CHECK(compiledSecondary->metatileEntries[30].tileIndex == 518);
    CHECK(compiledSecondary->metatileEntries[30].paletteIndex == 3);
    CHECK_FALSE(compiledSecondary->metatileEntries[31].hFlip);
    CHECK_FALSE(compiledSecondary->metatileEntries[31].vFlip);
    CHECK(compiledSecondary->metatileEntries[31].tileIndex == 519);
    CHECK(compiledSecondary->metatileEntries[31].paletteIndex == 3);
    // Metatile 2 top is blank, don't bother testing

    // Verify integrity of anims structure
    CHECK(compiledSecondary->anims.size() == 1);

    CHECK(compiledSecondary->anims.at(0).frames.size() == 4);
    CHECK(compiledSecondary->anims.at(0).frames.at(0).tiles.size() == 4);
    CHECK(compiledSecondary->anims.at(0).frames.at(1).tiles.size() == 4);
    CHECK(compiledSecondary->anims.at(0).frames.at(2).tiles.size() == 4);
    CHECK(compiledSecondary->anims.at(0).frames.at(3).tiles.size() == 4);
}

TEST_CASE("primer tiles should change output of primary compile function") {
    porytiles::PorytilesContext ctx{};
    ctx.fieldmapConfig.numPalettesInPrimary = 4;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.primarySmartPrune = true;
    ctx.compilerConfig.cacheAssign = false;

    // Import decompiled tiles
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_primer_1/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_primer_1/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_primer_1/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/palette_primer_1/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/palette_primer_1/bottom.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/palette_primer_1/bottom.png"};
    porytiles::DecompiledTileset decompiled = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    // Import palette primer
    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_primer_1/palette-primers/primer.pal"}));
    std::ifstream primerIfstream{std::filesystem::path{"Resources/Tests/palette_primer_1/palette-primers/primer.pal"}};
    porytiles::RGBATile primerTile =
        porytiles::importPalettePrimer(ctx, porytiles::CompilerMode::PRIMARY, primerIfstream,
                                       "Resources/Tests/palette_primer_1/palette-primers/primer.pal");
    primerTile.primerFilename = "Resources/Tests/palette_primer_1/palette-primers/primer.pal";
    std::vector<porytiles::RGBATile> palettePrimers{};
    palettePrimers.push_back(primerTile);
    primerIfstream.close();

    // Compile with no primer
    auto compiledNoPrimer = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiled, {}, {}, {});

    // Confirm compiled no primer is as expected
    CHECK(compiledNoPrimer->palettes.at(0).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoPrimer->palettes.at(0).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 255, 0}));
    CHECK(compiledNoPrimer->palettes.at(0).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 0}));
    CHECK(compiledNoPrimer->palettes.at(0).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledNoPrimer->palettes.at(1).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoPrimer->palettes.at(1).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 255, 255}));
    CHECK(compiledNoPrimer->palettes.at(1).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 255, 0}));
    CHECK(compiledNoPrimer->palettes.at(1).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledNoPrimer->palettes.at(2).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoPrimer->palettes.at(2).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledNoPrimer->palettes.at(2).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 255}));
    CHECK(compiledNoPrimer->palettes.at(2).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledNoPrimer->palettes.at(3).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoPrimer->palettes.at(3).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{128, 128, 128}));
    CHECK(compiledNoPrimer->palettes.at(3).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 255, 255}));
    CHECK(compiledNoPrimer->palettes.at(3).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));

    // Compile with primer
    auto compiledPrimer = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiled, palettePrimers, {}, {});

    // Confirm compiled with primer is as expected
    for (std::size_t i = 0; i < 3; i++) {
        CHECK(compiledPrimer->palettes.at(i).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
        for (std::size_t j = 1; j < porytiles::PAL_SIZE; j++) {
            CHECK(compiledPrimer->palettes.at(i).colors.at(j) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
        }
    }
    CHECK(compiledPrimer->palettes.at(3).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 255, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 255, 255}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(4) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 255, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(5) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(6) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 255}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(7) == porytiles::rgbaToBgr(porytiles::RGBA32{128, 128, 128}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(8) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 255, 255}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(9) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(10) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(11) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(12) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(13) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(14) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
    CHECK(compiledPrimer->palettes.at(3).colors.at(15) == porytiles::rgbaToBgr(porytiles::RGBA32{0, 0, 0}));
}

TEST_CASE("overrides should change output of primary compile function") {
    porytiles::PorytilesContext ctx{};
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.primaryExploredNodeCutoff = 1'000'000;
    ctx.compilerConfig.primarySmartPrune = true;
    ctx.compilerConfig.cacheAssign = false;

    // Import decompiled tiles
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/palette_override_1/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/palette_override_1/middle.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/palette_override_1/top.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    // Import palette overrides
    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/00.pal"}));
    std::ifstream override00IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/00.pal"}};

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/01.pal"}));
    std::ifstream override01IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/01.pal"}};

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/02.pal"}));
    std::ifstream override02IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/02.pal"}};

    auto [overrideTile00, overriddenPalSlots00] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::PRIMARY, override00IfStream,
                                         "Resources/Tests/palette_override_1/palette-overrides/00.pal");
    overrideTile00.overrideFilename = "Resources/Tests/palette_override_1/palette-overrides/00.pal";
    overrideTile00.overridePaletteIndex = 0;

    auto [overrideTile01, overriddenPalSlots01] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::PRIMARY, override01IfStream,
                                         "Resources/Tests/palette_override_1/palette-overrides/01.pal");
    overrideTile01.overrideFilename = "Resources/Tests/palette_override_1/palette-overrides/01.pal";
    overrideTile01.overridePaletteIndex = 1;

    auto [overrideTile02, overriddenPalSlots02] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::PRIMARY, override02IfStream,
                                         "Resources/Tests/palette_override_1/palette-overrides/02.pal");
    overrideTile02.overrideFilename = "Resources/Tests/palette_override_1/palette-overrides/02.pal";
    overrideTile02.overridePaletteIndex = 2;

    std::vector<porytiles::RGBATile> paletteOverrides{};
    paletteOverrides.push_back(overrideTile00);
    paletteOverrides.push_back(overrideTile01);
    paletteOverrides.push_back(overrideTile02);
    override00IfStream.close();
    override01IfStream.close();
    override02IfStream.close();
    std::unordered_map<size_t, std::vector<std::pair<size_t, porytiles::BGR15>>> palOverridesMap{};
    palOverridesMap.insert({0, overriddenPalSlots00});
    palOverridesMap.insert({1, overriddenPalSlots01});
    palOverridesMap.insert({2, overriddenPalSlots02});

    // Compile with no overrides
    auto compiledNoOverrides = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {}, {}, {});

    // Without overrides, there should be 53 actual tiles
    CHECK(compiledNoOverrides->sizeBeforePadding == 53);

    // Confirm compiled no override is as expected
    // Without overrides, the grass greens end up in pal 0
    CHECK(compiledNoOverrides->palettes.at(0).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoOverrides->palettes.at(0).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{112, 192, 160}));
    CHECK(compiledNoOverrides->palettes.at(0).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{64, 176, 128}));
    CHECK(compiledNoOverrides->palettes.at(0).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{160, 208, 192}));

    // Pals 1 and 2 contain a smattering of the mart/center colors
    CHECK(compiledNoOverrides->palettes.at(1).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoOverrides->palettes.at(1).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{200, 64, 80}));
    CHECK(compiledNoOverrides->palettes.at(1).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{248, 184, 128}));
    CHECK(compiledNoOverrides->palettes.at(1).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{232, 144, 112}));

    CHECK(compiledNoOverrides->palettes.at(2).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledNoOverrides->palettes.at(2).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{112, 192, 160}));
    CHECK(compiledNoOverrides->palettes.at(2).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{200, 64, 80}));
    CHECK(compiledNoOverrides->palettes.at(2).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{232, 144, 112}));

    // Compile with overrides
    auto compiledOverrides = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {},
                                                paletteOverrides, palOverridesMap);

    // With overrides, there should be 47 actual tiles
    CHECK(compiledOverrides->sizeBeforePadding == 47);

    // Confirm compiled with overrides is as expected
    CHECK(compiledOverrides->palettes.at(0).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledOverrides->palettes.at(0).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{96, 96, 120}));
    CHECK(compiledOverrides->palettes.at(0).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{248, 184, 128}));
    CHECK(compiledOverrides->palettes.at(0).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{200, 64, 80}));
    CHECK(compiledOverrides->palettes.at(0).colors.at(4) == porytiles::rgbaToBgr(porytiles::RGBA32{232, 144, 112}));

    CHECK(compiledOverrides->palettes.at(1).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledOverrides->palettes.at(1).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{96, 96, 120}));
    CHECK(compiledOverrides->palettes.at(1).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{112, 184, 240}));
    CHECK(compiledOverrides->palettes.at(1).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{72, 112, 168}));
    CHECK(compiledOverrides->palettes.at(1).colors.at(4) == porytiles::rgbaToBgr(porytiles::RGBA32{96, 160, 216}));

    /*
     * With overrides, the grass greens are now relegated to pal 2. Also, we set the tree greens to the end of pal 2
     via
     * an override
     */
    CHECK(compiledOverrides->palettes.at(2).colors.at(0) == porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(1) == porytiles::rgbaToBgr(porytiles::RGBA32{112, 192, 160}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(2) == porytiles::rgbaToBgr(porytiles::RGBA32{64, 176, 128}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(3) == porytiles::rgbaToBgr(porytiles::RGBA32{160, 208, 192}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(12) == porytiles::rgbaToBgr(porytiles::RGBA32{56, 136, 48}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(13) == porytiles::rgbaToBgr(porytiles::RGBA32{176, 248, 128}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(14) == porytiles::rgbaToBgr(porytiles::RGBA32{56, 80, 0}));
    CHECK(compiledOverrides->palettes.at(2).colors.at(15) == porytiles::rgbaToBgr(porytiles::RGBA32{128, 192, 96}));
}

TEST_CASE("overrides should change output of secondary compile function") {
    porytiles::PorytilesContext ctx{};
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.primaryExploredNodeCutoff = 1'000'000;
    ctx.compilerConfig.primarySmartPrune = true;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryExploredNodeCutoff = 1'000'000;
    ctx.compilerConfig.secondarySmartPrune = true;
    ctx.compilerConfig.cacheAssign = false;

    // Set up compilation for paired primary
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/top.png"}));
    png::image<png::rgba_pixel> bottomPrimary{"Resources/Tests/palette_override_1/bottom.png"};
    png::image<png::rgba_pixel> middlePrimary{"Resources/Tests/palette_override_1/middle.png"};
    png::image<png::rgba_pixel> topPrimary{"Resources/Tests/palette_override_1/top.png"};
    porytiles::DecompiledTileset decompiledPrimary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles::Attributes>{}, bottomPrimary,
        middlePrimary, topPrimary);

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/00.pal"}));
    std::ifstream override00IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/00.pal"}};

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/01.pal"}));
    std::ifstream override01IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/01.pal"}};

    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/02.pal"}));
    std::ifstream override02IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_1/palette-overrides/02.pal"}};

    auto [overrideTile00, overriddenPalSlots00] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::PRIMARY, override00IfStream,
                                         "Resources/Tests/palette_override_1/palette-overrides/00.pal");
    overrideTile00.overrideFilename = "Resources/Tests/palette_override_1/palette-overrides/00.pal";
    overrideTile00.overridePaletteIndex = 0;

    auto [overrideTile01, overriddenPalSlots01] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::PRIMARY, override01IfStream,
                                         "Resources/Tests/palette_override_1/palette-overrides/01.pal");
    overrideTile01.overrideFilename = "Resources/Tests/palette_override_1/palette-overrides/01.pal";
    overrideTile01.overridePaletteIndex = 1;

    auto [overrideTile02, overriddenPalSlots02] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::PRIMARY, override02IfStream,
                                         "Resources/Tests/palette_override_1/palette-overrides/02.pal");
    overrideTile02.overrideFilename = "Resources/Tests/palette_override_1/palette-overrides/02.pal";
    overrideTile02.overridePaletteIndex = 2;

    std::vector<porytiles::RGBATile> paletteOverridesPrimary{};
    paletteOverridesPrimary.push_back(overrideTile00);
    paletteOverridesPrimary.push_back(overrideTile01);
    paletteOverridesPrimary.push_back(overrideTile02);
    override00IfStream.close();
    override01IfStream.close();
    override02IfStream.close();
    std::unordered_map<size_t, std::vector<std::pair<size_t, porytiles::BGR15>>> palOverridesMapPrimary{};
    palOverridesMapPrimary.insert({0, overriddenPalSlots00});
    palOverridesMapPrimary.insert({1, overriddenPalSlots01});
    palOverridesMapPrimary.insert({2, overriddenPalSlots02});

    // Compile primary with overrides
    auto compiledPrimary = porytiles::compile(ctx, porytiles::CompilerMode::PRIMARY, decompiledPrimary, {},
                                              paletteOverridesPrimary, palOverridesMapPrimary);
    ctx.compilerContext.pairedPrimaryTileset = std::move(compiledPrimary);

    // Import secondary resources
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_2/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_2/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_2/top.png"}));
    png::image<png::rgba_pixel> bottomSecondary{"Resources/Tests/palette_override_2/bottom.png"};
    png::image<png::rgba_pixel> middleSecondary{"Resources/Tests/palette_override_2/middle.png"};
    png::image<png::rgba_pixel> topSecondary{"Resources/Tests/palette_override_2/top.png"};
    porytiles::DecompiledTileset decompiledSecondary = porytiles::importLayeredTilesFromPngs(
        ctx, porytiles::CompilerMode::SECONDARY, std::unordered_map<std::size_t, porytiles::Attributes>{},
        bottomSecondary, middleSecondary, topSecondary);
    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_2/palette-overrides/03.pal"}));
    std::ifstream override03IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_2/palette-overrides/03.pal"}};
    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_2/palette-overrides/04.pal"}));
    std::ifstream override04IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_2/palette-overrides/04.pal"}};
    REQUIRE(
        std::filesystem::exists(std::filesystem::path{"Resources/Tests/palette_override_2/palette-overrides/05.pal"}));
    std::ifstream override05IfStream{
        std::filesystem::path{"Resources/Tests/palette_override_2/palette-overrides/05.pal"}};
    auto [overrideTile03, overriddenPalSlots03] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::SECONDARY, override03IfStream,
                                         "Resources/Tests/palette_override_2/palette-overrides/03.pal");
    overrideTile03.overrideFilename = "Resources/Tests/palette_override_2/palette-overrides/03.pal";
    overrideTile03.overridePaletteIndex = 3;
    auto [overrideTile04, overriddenPalSlots04] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::SECONDARY, override04IfStream,
                                         "Resources/Tests/palette_override_2/palette-overrides/04.pal");
    overrideTile04.overrideFilename = "Resources/Tests/palette_override_2/palette-overrides/04.pal";
    overrideTile04.overridePaletteIndex = 4;
    auto [overrideTile05, overriddenPalSlots05] =
        porytiles::importPaletteOverride(ctx, porytiles::CompilerMode::SECONDARY, override05IfStream,
                                         "Resources/Tests/palette_override_2/palette-overrides/05.pal");
    overrideTile05.overrideFilename = "Resources/Tests/palette_override_2/palette-overrides/05.pal";
    overrideTile05.overridePaletteIndex = 5;
    std::vector<porytiles::RGBATile> paletteOverridesSecondary{};
    paletteOverridesSecondary.push_back(overrideTile03);
    paletteOverridesSecondary.push_back(overrideTile04);
    paletteOverridesSecondary.push_back(overrideTile05);
    override03IfStream.close();
    override04IfStream.close();
    override05IfStream.close();
    std::unordered_map<size_t, std::vector<std::pair<size_t, porytiles::BGR15>>> palOverridesMapSecondary{};
    palOverridesMapSecondary.insert({3, overriddenPalSlots03});
    palOverridesMapSecondary.insert({4, overriddenPalSlots04});
    palOverridesMapSecondary.insert({5, overriddenPalSlots05});

    // Compile with no overrides
    auto compiledSecondaryNoOverrides =
        porytiles::compile(ctx, porytiles::CompilerMode::SECONDARY, decompiledSecondary, {}, {}, {});

    // Without overrides, there should be 12 actual tiles
    CHECK(compiledSecondaryNoOverrides->sizeBeforePadding == 12);

    // Without overrides, the flower colors end up in pal 3
    CHECK(compiledSecondaryNoOverrides->palettes.at(3).colors.at(0) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(3).colors.at(1) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{64, 96, 64}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(3).colors.at(2) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{184, 96, 112}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(3).colors.at(3) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{248, 144, 112}));

    // Pal 4 contains the tree colors
    CHECK(compiledSecondaryNoOverrides->palettes.at(4).colors.at(0) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(4).colors.at(1) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{32, 96, 24}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(4).colors.at(2) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{176, 248, 144}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(4).colors.at(3) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{56, 152, 88}));

    // Pal 5 contains the log colors
    CHECK(compiledSecondaryNoOverrides->palettes.at(5).colors.at(0) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(5).colors.at(1) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{112, 64, 64}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(5).colors.at(2) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{216, 184, 152}));
    CHECK(compiledSecondaryNoOverrides->palettes.at(5).colors.at(3) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{176, 136, 128}));

    // Compile with overrides
    auto compiledSecondaryOverrides = porytiles::compile(ctx, porytiles::CompilerMode::SECONDARY, decompiledSecondary,
                                                         {}, paletteOverridesSecondary, palOverridesMapSecondary);

    // With overrides, there should be 6 actual tiles
    CHECK(compiledSecondaryOverrides->sizeBeforePadding == 6);

    // With overrides, pal 3 has log colors and tree colors
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(0) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(1) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{112, 64, 64}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(2) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{216, 184, 152}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(3) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{176, 136, 128}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(4) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{80, 48, 56}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(12) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{32, 96, 24}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(13) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{176, 248, 144}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(14) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{56, 152, 88}));
    CHECK(compiledSecondaryOverrides->palettes.at(3).colors.at(15) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{128, 208, 40}));

    // Pal 4 is the orange flower
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(0) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(4) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{64, 96, 64}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(5) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{64, 136, 80}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(6) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{128, 224, 64}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(7) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{96, 192, 80}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(8) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{56, 168, 128}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(9) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{48, 136, 40}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(10) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{232, 224, 160}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(11) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{184, 96, 112}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(12) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{248, 144, 112}));
    CHECK(compiledSecondaryOverrides->palettes.at(4).colors.at(13) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{144, 112, 64}));

    // Pal 5 is the purple flower
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(0) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{255, 0, 255}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(4) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{64, 96, 64}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(5) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{64, 136, 80}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(6) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{128, 224, 64}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(7) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{96, 192, 80}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(8) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{56, 168, 128}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(9) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{48, 136, 40}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(10) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{168, 184, 248}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(11) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{96, 48, 168}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(12) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{104, 128, 224}));
    CHECK(compiledSecondaryOverrides->palettes.at(5).colors.at(13) ==
          porytiles::rgbaToBgr(porytiles::RGBA32{168, 104, 152}));
}
#endif // TEST_PALETTE_OVERRIDES
