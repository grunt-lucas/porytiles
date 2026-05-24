#include "legacy/importer.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include <algorithm>
#include <bitset>
#include <csv.h>
#include <filesystem>
#include <fmt/color.h>
#include <fstream>
#include <iostream>
#include <png.hpp>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "legacy/cli_options.h"
#include "legacy/driver.h"
#include "legacy/logger.h"
#include "legacy/porytiles_context.h"
#include "legacy/types.h"
#include "legacy/utilities.h"
#include "panic/panic.hpp"

namespace porytiles_legacy {

DecompiledTileset importTilesFromPng(PorytilesContext &ctx, CompilerMode compilerMode,
                                     const png::image<png::rgba_pixel> &png) {
    if (png.get_height() % TILE_SIDE_LENGTH_PIX != 0) {
        ctx.diag->Report(ErrGeneric, fmt::format("source tiles PNG height '{}' was not divisible by 8",
                                                 ctx.diag->Bold(png.get_height())));
    }
    if (png.get_width() % TILE_SIDE_LENGTH_PIX != 0) {
        ctx.diag->Report(ErrGeneric, fmt::format("source tiles PNG width '{}' was not divisible by 8",
                                                 ctx.diag->Bold(png.get_width())));
    }

    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       "freestanding source dimension not divisible by 8");
    }

    DecompiledTileset decompiledTiles;

    std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH_PIX;
    std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH_PIX;

    for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / pngWidthInTiles;
        std::size_t tileCol = tileIndex % pngWidthInTiles;
        RGBATile tile{};
        tile.type = TileType::FREESTANDING;
        tile.tileIndex = tileIndex;
        for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
            tile.pixels[pixelIndex].red = png[pixelRow][pixelCol].red;
            tile.pixels[pixelIndex].green = png[pixelRow][pixelCol].green;
            tile.pixels[pixelIndex].blue = png[pixelRow][pixelCol].blue;
            tile.pixels[pixelIndex].alpha = png[pixelRow][pixelCol].alpha;
        }
        decompiledTiles.tiles.push_back(tile);
    }
    return decompiledTiles;
}

static std::bitset<3> getLayerBitset(const RGBA32 &transparentColor, const RGBATile &bottomTile,
                                     const RGBATile &middleTile, const RGBATile &topTile) {
    std::bitset<3> layers{};
    if (!bottomTile.transparent(transparentColor)) {
        layers.set(0);
    }
    if (!middleTile.transparent(transparentColor)) {
        layers.set(1);
    }
    if (!topTile.transparent(transparentColor)) {
        layers.set(2);
    }
    return layers;
}

static LayerType layerBitsetToLayerType(PorytilesContext &ctx, std::bitset<3> layerBitset, std::size_t metatileIndex) {
    bool bottomHasContent = layerBitset.test(0);
    bool middleHasContent = layerBitset.test(1);
    bool topHasContent = layerBitset.test(2);

    if (bottomHasContent && middleHasContent && topHasContent) {
        ctx.diag->Report(
            ErrGeneric,
            fmt::format("dual-layer inference failed for metatile '{}', all three layers had non-transparent content",
                        ctx.diag->Bold(metatileIndex)));
        return LayerType::TRIPLE;
    }
    if (!bottomHasContent && !middleHasContent && !topHasContent) {
        // transparent tile case
        return LayerType::NORMAL;
    }
    if (bottomHasContent && !middleHasContent && !topHasContent) {
        return LayerType::COVERED;
    }
    if (!bottomHasContent && middleHasContent && !topHasContent) {
        return LayerType::NORMAL;
    }
    if (!bottomHasContent && !middleHasContent && topHasContent) {
        return LayerType::NORMAL;
    }
    if (!bottomHasContent && middleHasContent && topHasContent) {
        return LayerType::NORMAL;
    }
    if (bottomHasContent && middleHasContent && !topHasContent) {
        return LayerType::COVERED;
    }

    // bottomHasContent && !middleHasContent && topHasContent
    return LayerType::SPLIT;
}

DecompiledTileset importLayeredTilesFromPngs(PorytilesContext &ctx, CompilerMode compilerMode,
                                             const std::unordered_map<std::size_t, Attributes> &attributesMap,
                                             const png::image<png::rgba_pixel> &bottom,
                                             const png::image<png::rgba_pixel> &middle,
                                             const png::image<png::rgba_pixel> &top) {
    if (bottom.get_height() % METATILE_SIDE_LENGTH != 0) {
        ctx.diag->Report(ErrGeneric, fmt::format("{} layer source PNG height '{}' was not divisible by 16",
                                                 layerString(TileLayer::BOTTOM), ctx.diag->Bold(bottom.get_height())));
    }
    if (middle.get_height() % METATILE_SIDE_LENGTH != 0) {
        ctx.diag->Report(ErrGeneric, fmt::format("{} layer source PNG height '{}' was not divisible by 16",
                                                 layerString(TileLayer::MIDDLE), ctx.diag->Bold(middle.get_height())));
    }
    if (top.get_height() % METATILE_SIDE_LENGTH != 0) {
        ctx.diag->Report(ErrGeneric, fmt::format("{} layer source PNG height '{}' was not divisible by 16",
                                                 layerString(TileLayer::TOP), ctx.diag->Bold(top.get_height())));
    }

    if (bottom.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
        ctx.diag->Report(ErrGeneric,
                         fmt::format("{} layer source PNG width '{}' was not {}", layerString(TileLayer::BOTTOM),
                                     fmt::styled(bottom.get_width(), fmt::emphasis::bold), METATILE_SHEET_WIDTH));
    }
    if (middle.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
        ctx.diag->Report(ErrGeneric,
                         fmt::format("{} layer source PNG width '{}' was not {}", layerString(TileLayer::MIDDLE),
                                     fmt::styled(middle.get_width(), fmt::emphasis::bold), METATILE_SHEET_WIDTH));
    }
    if (top.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
        ctx.diag->Report(ErrGeneric,
                         fmt::format("{} layer source PNG width '{}' was not {}", layerString(TileLayer::TOP),
                                     fmt::styled(top.get_width(), fmt::emphasis::bold), METATILE_SHEET_WIDTH));
    }
    if ((bottom.get_height() != middle.get_height()) || (bottom.get_height() != top.get_height())) {
        ctx.diag->Report(ErrGeneric,
                         fmt::format("bottom, middle, top layer source PNG heights '{}, {}, {}' were not equivalent",
                                     ctx.diag->Bold(bottom.get_height()), ctx.diag->Bold(middle.get_height()),
                                     ctx.diag->Bold(top.get_height())));
    }

    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), "source layer png dimensions invalid");
    }

    DecompiledTileset decompiledTiles{};

    // Since all widths and heights are the same, we can just read the bottom layer's width and height
    std::size_t widthInMetatiles = bottom.get_width() / METATILE_SIDE_LENGTH;
    std::size_t heightInMetatiles = bottom.get_height() / METATILE_SIDE_LENGTH;

    for (size_t metatileIndex = 0; metatileIndex < widthInMetatiles * heightInMetatiles; metatileIndex++) {
        size_t metatileRow = metatileIndex / widthInMetatiles;
        size_t metatileCol = metatileIndex % widthInMetatiles;
        std::vector<RGBATile> bottomTiles{};
        std::vector<RGBATile> middleTiles{};
        std::vector<RGBATile> topTiles{};

        // Grab the supplied default behavior, encounter/terrain types
        // FIXME : default behavior/encounter/terrain parsing code is duped
        std::uint16_t defaultBehavior;
        EncounterType defaultEncounterType;
        TerrainType defaultTerrainType;
        try {
            defaultBehavior = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
        } catch (const std::exception &) {
            defaultBehavior = 0;
            const auto msg = fmt::format("supplied default behavior '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultBehavior));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
        }
        try {
            std::uint8_t encounterValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
            defaultEncounterType = encounterTypeFromInt(encounterValue);
        } catch (const std::exception &) {
            defaultEncounterType = EncounterType::NONE;
            const auto msg = fmt::format("supplied default EncounterType '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultEncounterType));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
        }
        try {
            std::uint8_t terrainValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
            defaultTerrainType = terrainTypeFromInt(terrainValue);
        } catch (const std::exception &) {
            defaultTerrainType = TerrainType::NORMAL;
            const auto msg = fmt::format("supplied default TerrainType '{}' was not valid",
                                         ctx.diag->Bold(ctx.compilerConfig.defaultTerrainType));
            ctx.diag->Report(FatalGeneric, msg);
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
        }

        // Attributes are per-metatile so we can compute them once here
        Attributes metatileAttributes{};
        metatileAttributes.baseGame = ctx.targetBaseGame;
        metatileAttributes.metatileBehavior = defaultBehavior;
        metatileAttributes.encounterType = defaultEncounterType;
        metatileAttributes.terrainType = defaultTerrainType;
        if (attributesMap.contains(metatileIndex)) {
            const Attributes &fromMap = attributesMap.at(metatileIndex);
            metatileAttributes.metatileBehavior = fromMap.metatileBehavior;
            metatileAttributes.encounterType = fromMap.encounterType;
            metatileAttributes.terrainType = fromMap.terrainType;
        }

        // Bottom layer
        for (std::size_t bottomTileIndex = 0;
             bottomTileIndex < METATILE_TILE_SIDE_LENGTH_TILES * METATILE_TILE_SIDE_LENGTH_TILES; bottomTileIndex++) {
            std::size_t tileRow = bottomTileIndex / METATILE_TILE_SIDE_LENGTH_TILES;
            std::size_t tileCol = bottomTileIndex % METATILE_TILE_SIDE_LENGTH_TILES;
            RGBATile bottomTile{};
            bottomTile.type = TileType::LAYERED;
            bottomTile.layer = TileLayer::BOTTOM;
            bottomTile.metatileIndex = metatileIndex;
            bottomTile.subtile = static_cast<Subtile>(bottomTileIndex);
            bottomTile.attributes = metatileAttributes;
            for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                std::size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH_PIX) +
                                       (pixelIndex / TILE_SIDE_LENGTH_PIX);
                std::size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH_PIX) +
                                       (pixelIndex % TILE_SIDE_LENGTH_PIX);
                bottomTile.pixels[pixelIndex].red = bottom[pixelRow][pixelCol].red;
                bottomTile.pixels[pixelIndex].green = bottom[pixelRow][pixelCol].green;
                bottomTile.pixels[pixelIndex].blue = bottom[pixelRow][pixelCol].blue;
                bottomTile.pixels[pixelIndex].alpha = bottom[pixelRow][pixelCol].alpha;
            }
            bottomTiles.push_back(bottomTile);
        }

        // Middle layer
        for (std::size_t middleTileIndex = 0;
             middleTileIndex < METATILE_TILE_SIDE_LENGTH_TILES * METATILE_TILE_SIDE_LENGTH_TILES; middleTileIndex++) {
            std::size_t tileRow = middleTileIndex / METATILE_TILE_SIDE_LENGTH_TILES;
            std::size_t tileCol = middleTileIndex % METATILE_TILE_SIDE_LENGTH_TILES;
            RGBATile middleTile{};
            middleTile.type = TileType::LAYERED;
            middleTile.layer = TileLayer::MIDDLE;
            middleTile.metatileIndex = metatileIndex;
            middleTile.subtile = static_cast<Subtile>(middleTileIndex);
            middleTile.attributes = metatileAttributes;
            for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                std::size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH_PIX) +
                                       (pixelIndex / TILE_SIDE_LENGTH_PIX);
                std::size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH_PIX) +
                                       (pixelIndex % TILE_SIDE_LENGTH_PIX);
                middleTile.pixels[pixelIndex].red = middle[pixelRow][pixelCol].red;
                middleTile.pixels[pixelIndex].green = middle[pixelRow][pixelCol].green;
                middleTile.pixels[pixelIndex].blue = middle[pixelRow][pixelCol].blue;
                middleTile.pixels[pixelIndex].alpha = middle[pixelRow][pixelCol].alpha;
            }
            middleTiles.push_back(middleTile);
        }

        // Top layer
        for (std::size_t topTileIndex = 0;
             topTileIndex < METATILE_TILE_SIDE_LENGTH_TILES * METATILE_TILE_SIDE_LENGTH_TILES; topTileIndex++) {
            std::size_t tileRow = topTileIndex / METATILE_TILE_SIDE_LENGTH_TILES;
            std::size_t tileCol = topTileIndex % METATILE_TILE_SIDE_LENGTH_TILES;
            RGBATile topTile{};
            topTile.type = TileType::LAYERED;
            topTile.layer = TileLayer::TOP;
            topTile.metatileIndex = metatileIndex;
            topTile.subtile = static_cast<Subtile>(topTileIndex);
            topTile.attributes = metatileAttributes;
            topTile.attributes = metatileAttributes;
            for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                std::size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH_PIX) +
                                       (pixelIndex / TILE_SIDE_LENGTH_PIX);
                std::size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH_PIX) +
                                       (pixelIndex % TILE_SIDE_LENGTH_PIX);
                topTile.pixels[pixelIndex].red = top[pixelRow][pixelCol].red;
                topTile.pixels[pixelIndex].green = top[pixelRow][pixelCol].green;
                topTile.pixels[pixelIndex].blue = top[pixelRow][pixelCol].blue;
                topTile.pixels[pixelIndex].alpha = top[pixelRow][pixelCol].alpha;
            }
            topTiles.push_back(topTile);
        }

        if (bottomTiles.size() != middleTiles.size() || middleTiles.size() != topTiles.size()) {
            Panic("importer::importLayeredTilesFromPng bottomTiles, middleTiles, topTiles sizes were not equivalent");
        }

        if (ctx.compilerConfig.tripleLayer) {
            // Triple layer case: set all three layers to LayerType::TRIPLE
            for (std::size_t i = 0; i < bottomTiles.size(); i++) {
                bottomTiles.at(i).attributes.layerType = LayerType::TRIPLE;
                middleTiles.at(i).attributes.layerType = LayerType::TRIPLE;
                topTiles.at(i).attributes.layerType = LayerType::TRIPLE;
            }
        } else {
            /*
             * Determine layer type by assigning each logical layer to a bit in a bitset. Then we compute the "layer
             * bitset" for each subtile in the metatile. Finally, we OR all these bitsets together. If the final bitset
             * size is 2 or less, then we know we have a valid dual-layer tile. We can then read out the bits to
             * determine which layer type best describes this tile.
             */
            std::bitset<3> layers = getLayerBitset(ctx.compilerConfig.transparencyColor, bottomTiles.at(0),
                                                   middleTiles.at(0), topTiles.at(0));
            for (std::size_t i = 1; i < bottomTiles.size(); i++) {
                std::bitset<3> newLayers = getLayerBitset(ctx.compilerConfig.transparencyColor, bottomTiles.at(i),
                                                          middleTiles.at(i), topTiles.at(i));
                layers |= newLayers;
            }
            LayerType type = layerBitsetToLayerType(ctx, layers, metatileIndex);
            for (std::size_t i = 0; i < bottomTiles.size(); i++) {
                bottomTiles.at(i).attributes.layerType = type;
                middleTiles.at(i).attributes.layerType = type;
                topTiles.at(i).attributes.layerType = type;
            }
        }

        // Copy the tiles into the decompiled buffer, accounting for the LayerType we just computed
        switch (bottomTiles.at(0).attributes.layerType) {
        case LayerType::TRIPLE:
            for (const auto &bottomTile : bottomTiles) {
                decompiledTiles.tiles.push_back(bottomTile);
            }
            for (const auto &middleTile : middleTiles) {
                decompiledTiles.tiles.push_back(middleTile);
            }
            for (const auto &topTile : topTiles) {
                decompiledTiles.tiles.push_back(topTile);
            }
            break;
        case LayerType::NORMAL:
            for (const auto &middleTile : middleTiles) {
                decompiledTiles.tiles.push_back(middleTile);
            }
            for (const auto &topTile : topTiles) {
                decompiledTiles.tiles.push_back(topTile);
            }
            break;
        case LayerType::COVERED:
            for (const auto &bottomTile : bottomTiles) {
                decompiledTiles.tiles.push_back(bottomTile);
            }
            for (const auto &middleTile : middleTiles) {
                decompiledTiles.tiles.push_back(middleTile);
            }
            break;
        case LayerType::SPLIT:
            for (const auto &bottomTile : bottomTiles) {
                decompiledTiles.tiles.push_back(bottomTile);
            }
            for (const auto &topTile : topTiles) {
                decompiledTiles.tiles.push_back(topTile);
            }
            break;
        default:
            Panic("importer::importLayeredTilesFromPng unknown LayerType");
        }
    }

    std::size_t metatileCount = decompiledTiles.tiles.size() / (ctx.compilerConfig.tripleLayer ? 12 : 8);
    for (const auto &metatileId : attributesMap | std::views::keys) {
        if (metatileId > metatileCount - 1) {
            ctx.diag->Report(WarnUnusedAttribute, ctx.diag->Bold(metatileId));
            ctx.diag->ReportPartner(WarnUnusedAttribute, 0, metatileCount,
                                    ctx.diag->Bold(ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode).string()));
        }
    }

    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       "errors generated during layered tile import");
    }

    return decompiledTiles;
}

static void validateAnimFormat(const PorytilesContext &ctx, const DecompiledAnimation &anim) {
    if (anim.frames.size() < 2) {
        Panic("importer::validateAnimFormat bad anim format, found frames.size() < 2");
    }
    std::vector<std::unordered_set<RGBA32>> keyFrameColors{};
    std::vector<std::unordered_set<RGBA32>> regularFrameColors{};
    std::vector<std::unordered_set<RGBA32>> keyFrameMissingColors{};

    for (const auto &tile : anim.keyFrame().tiles) {
        std::unordered_set<RGBA32> tileColors{};
        for (const auto &color : tile.pixels) {
            tileColors.insert(color);
        }
        keyFrameColors.push_back(tileColors);
    }

    for (std::size_t i = 0; i < keyFrameColors.size(); i++) {
        regularFrameColors.emplace_back();
        keyFrameMissingColors.emplace_back();
    }

    for (std::size_t i = 1; i < anim.size(); i++) {
        const auto &frame = anim.frames.at(i);
        int tileIndex = 0;
        for (const auto &tile : frame.tiles) {
            for (const auto &color : tile.pixels) {
                regularFrameColors.at(tileIndex).insert(color);
            }
            tileIndex++;
        }
    }

    for (std::size_t i = 0; i < keyFrameColors.size(); i++) {
        const auto &keyFrameColorsSet = keyFrameColors.at(i);
        const auto &regularFrameColorsSet = regularFrameColors.at(i);
        auto &keyFrameMissingColorsSet = keyFrameMissingColors.at(i);
        for (const auto &color : regularFrameColorsSet) {
            if (!keyFrameColorsSet.contains(color)) {
                keyFrameMissingColorsSet.insert(color);
            }
        }
    }

    for (std::size_t i = 0; i < keyFrameMissingColors.size(); i++) {
        if (!keyFrameMissingColors.at(i).empty()) {
            ctx.diag->Report(WarnKeyFrameMissingColors, anim.animName, i);
            std::vector<RGBA32> v{keyFrameMissingColors.at(i).begin(), keyFrameMissingColors.at(i).end()};
            ctx.diag->ReportPartner(WarnKeyFrameMissingColors, 0, std::move(v));
        }
    }
}

void importAnimTiles(PorytilesContext &ctx, CompilerMode compilerMode,
                     const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims,
                     DecompiledTileset &tiles) {
    std::vector<DecompiledAnimation> anims{};

    for (const auto &rawAnim : rawAnims) {
        if (rawAnim.empty()) {
            Panic("importer::importAnimTiles rawAnim was empty");
        }

        std::set<png::uint_32> frameWidths{};
        std::set<png::uint_32> frameHeights{};
        std::string animName = rawAnim.at(0).animName;
        DecompiledAnimation anim{animName};
        for (const auto &rawFrame : rawAnim) {
            DecompiledAnimFrame animFrame{rawFrame.frameName};

            if (rawFrame.png.get_height() % TILE_SIDE_LENGTH_PIX != 0) {
                ctx.diag->Report(ErrGeneric, fmt::format("anim {} frame {} PNG height '{}' was not divisible by 8",
                                                         rawFrame.animName, rawFrame.frameName,
                                                         ctx.diag->Bold(rawFrame.png.get_height())));
            }
            if (rawFrame.png.get_width() % TILE_SIDE_LENGTH_PIX != 0) {
                ctx.diag->Report(ErrGeneric, fmt::format("anim {} frame {} PNG width '{}' was not divisible by 8",
                                                         rawFrame.animName, rawFrame.frameName,
                                                         ctx.diag->Bold(rawFrame.png.get_width())));
            }

            if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
                die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                               "anim frame source dimension not divisible by 8");
            }

            frameWidths.insert(rawFrame.png.get_width());
            frameHeights.insert(rawFrame.png.get_height());
            if (frameWidths.size() != 1) {
                const auto dimensionName = "width";
                const auto msg = fmt::format("animation '{}' frame '{}' {} '{}' did not match previous frame {}s",
                                             ctx.diag->Bold(rawFrame.animName), ctx.diag->Bold(rawFrame.frameName),
                                             dimensionName, ctx.diag->Bold(rawFrame.png.get_width()), dimensionName);
                ctx.diag->Report(FatalGeneric, msg);
                die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                          fmt::format("anim {} frame {} dimension {} mismatch", rawFrame.animName,
                                                      rawFrame.frameName, dimensionName));
            }
            if (frameHeights.size() != 1) {
                const auto dimensionName = "height";
                const auto msg = fmt::format("animation '{}' frame '{}' {} '{}' did not match previous frame {}s",
                                             ctx.diag->Bold(rawFrame.animName), ctx.diag->Bold(rawFrame.frameName),
                                             dimensionName, ctx.diag->Bold(rawFrame.png.get_height()), dimensionName);
                ctx.diag->Report(FatalGeneric, msg);
                die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                          fmt::format("anim {} frame {} dimension {} mismatch", rawFrame.animName,
                                                      rawFrame.frameName, dimensionName));
            }

            std::size_t pngWidthInTiles = rawFrame.png.get_width() / TILE_SIDE_LENGTH_PIX;
            std::size_t pngHeightInTiles = rawFrame.png.get_height() / TILE_SIDE_LENGTH_PIX;
            for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
                std::size_t tileRow = tileIndex / pngWidthInTiles;
                std::size_t tileCol = tileIndex % pngWidthInTiles;
                RGBATile tile{};
                tile.type = TileType::ANIM;
                tile.anim = rawFrame.animName;
                tile.frame = rawFrame.frameName;
                tile.tileIndex = tileIndex;

                for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                    std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
                    std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
                    tile.pixels[pixelIndex].red = rawFrame.png[pixelRow][pixelCol].red;
                    tile.pixels[pixelIndex].green = rawFrame.png[pixelRow][pixelCol].green;
                    tile.pixels[pixelIndex].blue = rawFrame.png[pixelRow][pixelCol].blue;
                    tile.pixels[pixelIndex].alpha = rawFrame.png[pixelRow][pixelCol].alpha;
                }
                animFrame.tiles.push_back(tile);
            }
            anim.frames.push_back(animFrame);
        }
        validateAnimFormat(ctx, anim);
        anims.push_back(anim);
    }

    if (ctx.diag->EnabledAt(WarnKeyFrameMissingColors) == DiagLevel::Error &&
        ctx.diag->InFlightCountFor(WarnKeyFrameMissingColors) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       "some key frame subtiles were missing essential colors");
    }
    tiles.anims = anims;
}

static std::uint8_t parseMacroFormatLine(PorytilesContext &ctx, std::ifstream &behaviorFile, CompilerMode *compilerMode,
                                         DecompilerMode *decompilerMode, const std::string &behaviorName,
                                         const std::string &behaviorValueString, std::size_t processedUpToLine) {
    std::uint8_t behaviorVal{};
    try {
        std::size_t pos;
        behaviorVal = std::stoi(behaviorValueString, &pos, 0);
        if (std::string{behaviorValueString}.size() != pos) {
            behaviorFile.close();
            // throw here so it catches below and prints an error message
            throw std::runtime_error{""};
        }
    } catch (const std::exception &e) {
        behaviorFile.close();
        if (compilerMode != nullptr) {
            const auto msg =
                fmt::format("invalid value '{}' for behavior '{}' defined at line {}",
                            ctx.diag->Bold(behaviorValueString), ctx.diag->Bold(behaviorName), processedUpToLine);
            ctx.diag->Report(FatalGeneric, msg);
            ctx.diag->Report(
                NoteGeneric,
                "behavior must be an integral value (both decimal and hexadecimal notations are permitted)",
                ctx.diag->Bold("id"));
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(*compilerMode),
                                      fmt::format("invalid behavior value {}", behaviorValueString));
        }
        if (decompilerMode != nullptr) {
            const auto msg =
                fmt::format("invalid value '{}' for behavior '{}' defined at line {}",
                            ctx.diag->Bold(behaviorValueString), ctx.diag->Bold(behaviorName), processedUpToLine);
            ctx.diag->Report(FatalGeneric, msg);
            ctx.diag->Report(
                NoteGeneric,
                "behavior must be an integral value (both decimal and hexadecimal notations are permitted)",
                ctx.diag->Bold("id"));
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(*decompilerMode),
                                        fmt::format("invalid behavior value {}", behaviorValueString));
        }
        Panic("importer::importMetatileBehaviorHeader both compilerMode and decompilerMode were null");
    }
    return behaviorVal;
}

static std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeaderHelper(PorytilesContext &ctx, CompilerMode *compilerMode, DecompilerMode *decompilerMode,
                                   std::ifstream &behaviorFile) {
    std::unordered_map<std::string, std::uint8_t> behaviorMap{};
    std::unordered_map<std::uint8_t, std::string> behaviorReverseMap{};

    std::string line;
    std::size_t processedUpToLine = 1;
    std::uint8_t enumBehaviorCounter = 0;
    while (std::getline(behaviorFile, line)) {
        std::string buffer;
        std::stringstream stringStream(line);
        std::vector<std::string> tokens{};
        while (stringStream >> buffer) {
            tokens.push_back(buffer);
        }
        // Parse the macro format of the header, e.g.
        // #define MB_DEEP_WATER 0x12
        if (tokens.size() >= 3 && tokens.at(1).starts_with("MB_") && tokens.at(1) != "MB_INVALID") {
            const std::string &behaviorName = tokens.at(1);
            const std::string &behaviorValueString = tokens.at(2);
            const std::uint8_t behaviorVal = parseMacroFormatLine(ctx, behaviorFile, compilerMode, decompilerMode,
                                                                  behaviorName, behaviorValueString, processedUpToLine);
            if (behaviorVal != 0xFF) {
                behaviorMap.insert(std::pair{behaviorName, behaviorVal});
                behaviorReverseMap.insert(std::pair{behaviorVal, behaviorName});
            }
        }
        // Parse the enum format of the header, e.g.
        //  MB_DEEP_WATER,
        // or
        //  MB_INTERIOR_DEEP_WATER, // Used by interior maps; functionally the same as MB_DEEP_WATER
        else if (!tokens.empty() && tokens.at(0).starts_with("MB_") && tokens.at(0).back() == ',' &&
                 tokens.at(0) != "MB_INVALID") {
            std::string &behaviorName = tokens.at(0);
            if (!behaviorName.empty() && behaviorName.back() == ',') {
                behaviorName.pop_back();
            }
            behaviorMap.insert(std::pair{behaviorName, enumBehaviorCounter});
            behaviorReverseMap.insert(std::pair{enumBehaviorCounter, behaviorName});
            enumBehaviorCounter++;
        }
        processedUpToLine++;
    }
    behaviorFile.close();

    return std::pair{behaviorMap, behaviorReverseMap};
}

std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeader(PorytilesContext &ctx, CompilerMode compilerMode, std::ifstream &behaviorFile) {
    return importMetatileBehaviorHeaderHelper(ctx, &compilerMode, nullptr, behaviorFile);
}

std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeader(PorytilesContext &ctx, DecompilerMode decompilerMode, std::ifstream &behaviorFile) {
    return importMetatileBehaviorHeaderHelper(ctx, nullptr, &decompilerMode, behaviorFile);
}

std::unordered_map<std::size_t, Attributes>
importAttributesFromCsv(PorytilesContext &ctx, CompilerMode compilerMode,
                        const std::unordered_map<std::string, std::uint8_t> &behaviorMap, const std::string &filePath) {
    std::unordered_map<std::size_t, Attributes> attributeMap{};
    std::unordered_map<std::size_t, std::size_t> lineFirstSeen{};
    io::CSVReader<4> in{filePath};
    try {
        in.read_header(io::ignore_missing_column, "id", "behavior", "terrainType", "encounterType");
    } catch (const std::exception &) {
        const auto msg = fmt::format("{}: incorrect header row format", filePath);
        ctx.diag->Report(FatalGeneric, msg);
        ctx.diag->Report(NoteGeneric, fmt::format("valid headers are '{}' or '{}'", ctx.diag->Bold("id,behavior"),
                                                  ctx.diag->Bold("id,behavior,terrainType,encounterType")));
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                  fmt::format("{}: incorrect header row format", filePath));
    }

    std::string id;
    bool hasId = in.has_column("id");

    std::string behavior;
    bool hasBehavior = in.has_column("behavior");

    std::string terrainType;
    bool hasTerrainType = in.has_column("terrainType");

    std::string encounterType;
    bool hasEncounterType = in.has_column("encounterType");

    if (!hasId || !hasBehavior || (hasTerrainType && !hasEncounterType) || (!hasTerrainType && hasEncounterType)) {
        const auto msg = fmt::format("{}: incorrect header row format", filePath);
        ctx.diag->Report(FatalGeneric, msg);
        ctx.diag->Report(NoteGeneric, fmt::format("valid headers are '{}' or '{}'", ctx.diag->Bold("id,behavior"),
                                                  ctx.diag->Bold("id,behavior,terrainType,encounterType")));
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                  fmt::format("{}: incorrect header row format", filePath));
    }

    if (ctx.targetBaseGame == TargetBaseGame::FIRERED && (!hasTerrainType || !hasEncounterType)) {
        ctx.diag->Report(WarnAttributeFormatMismatch, ctx.diag->Bold(filePath), "few",
                         ctx.diag->Bold(targetBaseGameString(ctx.targetBaseGame)));
        ctx.diag->ReportPartner(WarnAttributeFormatMismatch, 0);
    }
    if ((ctx.targetBaseGame == TargetBaseGame::EMERALD || ctx.targetBaseGame == TargetBaseGame::RUBY) &&
        (hasTerrainType || hasEncounterType)) {
        ctx.diag->Report(WarnAttributeFormatMismatch, ctx.diag->Bold(filePath), "many",
                         ctx.diag->Bold(targetBaseGameString(ctx.targetBaseGame)));
    }

    // Grab the supplied default behavior, encounter/terrain types
    // FIXME : default behavior/encounter/terrain parsing code is duped
    std::uint16_t defaultBehavior;
    EncounterType defaultEncounterType;
    TerrainType defaultTerrainType;
    try {
        defaultBehavior = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultBehavior.c_str());
    } catch (const std::exception &e) {
        defaultBehavior = 0;
        const auto msg = fmt::format("supplied default behavior '{}' was not valid",
                                     ctx.diag->Bold(ctx.compilerConfig.defaultBehavior));
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    try {
        std::uint8_t encounterValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultEncounterType.c_str());
        defaultEncounterType = encounterTypeFromInt(encounterValue);
    } catch (const std::exception &e) {
        defaultEncounterType = EncounterType::NONE;
        const auto msg = fmt::format("supplied default EncounterType '{}' was not valid",
                                     ctx.diag->Bold(ctx.compilerConfig.defaultEncounterType));
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    try {
        std::uint8_t terrainValue = parseInteger<std::uint16_t>(ctx.compilerConfig.defaultTerrainType.c_str());
        defaultTerrainType = terrainTypeFromInt(terrainValue);
    } catch (const std::exception &e) {
        defaultTerrainType = TerrainType::NORMAL;
        const auto msg = fmt::format("supplied default TerrainType '{}' was not valid",
                                     ctx.diag->Bold(ctx.compilerConfig.defaultTerrainType));
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }

    // processedUpToLine starts at 1 since we processed the header already, which was on line 1
    std::size_t processedUpToLine = 1;
    while (true) {
        bool readRow = false;
        try {
            readRow = in.read_row(id, behavior, terrainType, encounterType);
            processedUpToLine++;
        } catch (const std::exception &e) {
            // increment processedUpToLine here, since we threw before we could increment in the try
            processedUpToLine++;
            ctx.diag->Report(ErrGeneric,
                             fmt::format("{} provided columns did not match header",
                                         ctx.diag->Bold(filePath + ":" + std::to_string(processedUpToLine) + ":")));
            continue;
        }
        if (!readRow) {
            break;
        }

        Attributes attribute{};
        attribute.baseGame = ctx.targetBaseGame;
        attribute.metatileBehavior = defaultBehavior;
        attribute.encounterType = defaultEncounterType;
        attribute.terrainType = defaultTerrainType;
        if (behaviorMap.contains(behavior)) {
            attribute.metatileBehavior = behaviorMap.at(behavior);
        } else {
            ctx.diag->Report(ErrGeneric,
                             fmt::format("{} unknown metatile behavior '{}'",
                                         ctx.diag->Bold(filePath + ":" + std::to_string(processedUpToLine) + ":"),
                                         ctx.diag->Bold(behavior)));
        }

        if (hasTerrainType) {
            try {
                attribute.terrainType = stringToTerrainType(terrainType);
            } catch (std::invalid_argument &) {
                ctx.diag->Report(ErrGeneric,
                                 fmt::format("{} invalid TerrainType `{}'",
                                             ctx.diag->Bold(filePath + ":" + std::to_string(processedUpToLine) + ":"),
                                             ctx.diag->Bold(terrainType)));
            }
        }
        if (hasEncounterType) {
            try {
                attribute.encounterType = stringToEncounterType(encounterType);
            } catch (std::invalid_argument &) {
                ctx.diag->Report(ErrGeneric,
                                 fmt::format("{} invalid EncounterType `{}'",
                                             ctx.diag->Bold(filePath + ":" + std::to_string(processedUpToLine) + ":"),
                                             ctx.diag->Bold(encounterType)));
            }
        }

        std::size_t idVal{};
        try {
            std::size_t pos;
            idVal = std::stoi(id, &pos, 0);
            if (std::string{id}.size() != pos) {
                // throw here so it catches below and prints an error
                throw std::runtime_error{""};
            }
        } catch (std::exception &) {
            const auto msg = fmt::format("{}: invalid value '{}' for column '{}' at line {}", filePath,
                                         ctx.diag->Bold(id), ctx.diag->Bold("id"), processedUpToLine);
            ctx.diag->Report(FatalGeneric, msg);
            ctx.diag->Report(
                NoteGeneric,
                "column '{}' must contain an integral value (both decimal and hexadecimal notations are permitted)",
                ctx.diag->Bold("id"));
            die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                                      fmt::format("{}: invalid id {}", filePath, id));
        }

        auto inserted = attributeMap.insert(std::pair{idVal, attribute});
        if (!inserted.second) {
            ctx.diag->Report(ErrGeneric,
                             fmt::format("{} duplicate entry for metatile '{}', first definition on line {}",
                                         ctx.diag->Bold(filePath + ":" + std::to_string(processedUpToLine) + ":"),
                                         ctx.diag->Bold(id), lineFirstSeen.at(idVal)));
        }
        if (!lineFirstSeen.contains(idVal)) {
            lineFirstSeen.insert(std::pair{idVal, processedUpToLine});
        }
    }

    if (ctx.diag->InFlightCountForLevel(DiagLevel::Error) > 0) {
        die_errorCount(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode),
                       "errors generated during attributes CSV parsing");
    }

    return attributeMap;
}

static std::vector<GBAPalette> importCompiledPalettes(PorytilesContext &ctx, DecompilerMode decompilerMode,
                                                      const std::vector<std::unique_ptr<std::ifstream>> &paletteFiles,
                                                      const std::vector<std::string> &fileNames) {
    std::vector<GBAPalette> palettes{};

    int index = 0;
    for (const std::unique_ptr<std::ifstream> &stream : paletteFiles) {
        std::string line;
        /*
         * TODO : should fatal errors here have better messages? Users shouldn't ever really see these errors, since
         * compiled palettes will always presumably have correct formatting unless a user has manually messed with one
         */
        /*
         * FIXME : this function assumes the pal file is DOS format, need to fix this
         * Pret PR here recently addressed the gbagfx DOS line ending issue:
         * https://github.com/pret/pokeemerald/pull/2004
         */
        std::getline(*stream, line);
        if (line.size() == 0) {
            const auto msg = "invalid blank line in pal file";
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
        }
        line.pop_back();
        if (line != "JASC-PAL") {
            const auto msg = fmt::format("expected `JASC-PAL' in pal file, saw '{}'", line);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
        }

        std::getline(*stream, line);
        if (line.size() == 0) {
            const auto msg = "invalid blank line in pal file";
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
        }
        line.pop_back();
        if (line != "0100") {
            const auto msg = fmt::format("expected `0100' in pal file, saw '{}'", line);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
        }

        std::getline(*stream, line);
        if (line.size() == 0) {
            const auto msg = "invalid blank line in pal file";
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
        }
        line.pop_back();
        if (line != "16") {
            const auto msg = fmt::format("expected `16' in pal file, saw '{}'", line);
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(decompilerMode), msg);
        }

        GBAPalette palette{};
        /*
         * Set palette size to PAL_SIZE (16). There is really no way to truly tell a compiled palette's size, since 0
         * could have been an intentional black, or it could mean the color was unset. For decompilation, we don't
         * really care anyway.
         */
        palette.size = PAL_SIZE;
        std::size_t colorIndex = 0;
        while (std::getline(*stream, line)) {
            BGR15 bgr = rgbaToBgr(parseJascLineDecompiler(ctx, decompilerMode, line, fileNames.at(index)));
            palette.colors.at(colorIndex) = bgr;
            colorIndex++;
        }
        palettes.push_back(palette);
        index++;
    }

    return palettes;
}

static std::vector<GBATile> importCompiledTiles(PorytilesContext &ctx, const png::image<png::index_pixel> &tiles) {
    std::vector<GBATile> gbaTiles{};

    std::size_t widthInTiles = tiles.get_width() / TILE_SIDE_LENGTH_PIX;
    std::size_t heightInTiles = tiles.get_height() / TILE_SIDE_LENGTH_PIX;

    for (std::size_t tileIndex = 0; tileIndex < widthInTiles * heightInTiles; tileIndex++) {
        std::size_t tileRow = tileIndex / widthInTiles;
        std::size_t tileCol = tileIndex % widthInTiles;
        GBATile tile{};
        for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
            std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
            std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
            tile.colorIndexes.at(pixelIndex) = tiles[pixelRow][pixelCol];
        }
        gbaTiles.push_back(tile);
    }

    return gbaTiles;
}

static std::vector<MetatileEntry>
importCompiledMetatiles(PorytilesContext &ctx, DecompilerMode mode, std::ifstream &metatilesBin,
                        std::unordered_map<std::size_t, Attributes> &attributesMap,
                        const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap) {
    std::vector<MetatileEntry> metatileEntries{};

    std::vector<unsigned char> metatileDataBuf{std::istreambuf_iterator<char>(metatilesBin), {}};

    /*
     * Each subtile is 2 bytes (u16), so our byte total should be either a multiple of 16 or 24. 16 for dual-layer,
     * since there are 8 subtiles per metatile. 24 for triple layer, since there are 12 subtiles per metatile.
     */
    if (metatileDataBuf.size() % (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_DUAL) != 0 &&
        metatileDataBuf.size() % (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_TRIPLE) != 0) {
        const auto msg = "decompiler input 'metatiles.bin' corrupted, not valid uint16 data";
        ctx.diag->Report(FatalGeneric, msg);
        die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
    }

    bool tripleLayer =
        (metatileDataBuf.size() / (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_TRIPLE) == attributesMap.size());

    std::size_t metatileIndex = 0;
    for (std::size_t metatileBinByteIndex = 0; metatileBinByteIndex < metatileDataBuf.size();
         metatileBinByteIndex += 2) {
        MetatileEntry metatileEntry{};

        // Compute the actual metatileIndex
        metatileIndex = tripleLayer ? metatileBinByteIndex / (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_TRIPLE)
                                    : metatileBinByteIndex / (BYTES_PER_METATILE_ENTRY * TILES_PER_METATILE_DUAL);

        std::uint16_t lowerByte = metatileDataBuf.at(metatileBinByteIndex);
        std::uint16_t upperByte = metatileDataBuf.at(metatileBinByteIndex + 1);
        std::uint16_t entryBits = (upperByte << 8) | lowerByte;
        metatileEntry.tileIndex = entryBits & 0x03FF;
        metatileEntry.hFlip = (entryBits >> 10) & 0x0001;
        metatileEntry.vFlip = (entryBits >> 11) & 0x0001;
        metatileEntry.paletteIndex = (entryBits >> 12) & 0x000F;

        metatileEntry.attributes.baseGame = ctx.targetBaseGame;
        if (tripleLayer) {
            metatileEntry.attributes.layerType = LayerType::TRIPLE;
        } else {
            metatileEntry.attributes.layerType = attributesMap.at(metatileIndex).layerType;
        }
        metatileEntry.attributes.metatileBehavior = attributesMap.at(metatileIndex).metatileBehavior;
        metatileEntry.attributes.encounterType = attributesMap.at(metatileIndex).encounterType;
        metatileEntry.attributes.terrainType = attributesMap.at(metatileIndex).terrainType;

        std::string behaviorString = std::to_string(metatileEntry.attributes.metatileBehavior);
        if (behaviorReverseMap.contains(metatileEntry.attributes.metatileBehavior)) {
            behaviorString = behaviorReverseMap.at(metatileEntry.attributes.metatileBehavior);
        }

        if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
            pt_logln(
                ctx, stderr,
                "found MetatileEntry[tile: {}, hFlip: {}, vFlip: {}, palette: {}, attr:[behavior: {}, layerType: {}, "
                "terrainType: {}, encounterType: {}]]",
                metatileEntry.tileIndex, metatileEntry.hFlip, metatileEntry.vFlip, metatileEntry.paletteIndex,
                behaviorString, layerTypeString(metatileEntry.attributes.layerType),
                terrainTypeString(metatileEntry.attributes.terrainType),
                encounterTypeString(metatileEntry.attributes.encounterType));
        } else {
            pt_logln(
                ctx, stderr,
                "found MetatileEntry[tile: {}, hFlip: {}, vFlip: {}, palette: {}, attr:[behavior: {}, layerType: {}]]",
                metatileEntry.tileIndex, metatileEntry.hFlip, metatileEntry.vFlip, metatileEntry.paletteIndex,
                behaviorString, layerTypeString(metatileEntry.attributes.layerType));
        }

        metatileEntries.push_back(metatileEntry);
    }

    return metatileEntries;
}

static std::unordered_map<std::size_t, Attributes>
importCompiledMetatileAttributes(PorytilesContext &ctx, DecompilerMode mode, std::ifstream &metatileAttributesBin) {
    std::vector<unsigned char> attributesDataBuf{std::istreambuf_iterator<char>(metatileAttributesBin), {}};

    std::unordered_map<std::size_t, Attributes> attributesMap{};

    std::size_t metatileCount;
    if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
        if (attributesDataBuf.size() % BYTES_PER_ATTRIBUTE_FIRERED != 0) {
            const auto msg = "decompiler input 'metatile_attributes.bin' corrupted, not valid uint32 data";
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
        }
        metatileCount = attributesDataBuf.size() / BYTES_PER_ATTRIBUTE_FIRERED;
    } else {
        if (attributesDataBuf.size() % BYTES_PER_ATTRIBUTE_EMERALD != 0) {
            const auto msg = "decompiler input 'metatile_attributes.bin' corrupted, not valid uint16 data";
            ctx.diag->Report(FatalGeneric, msg);
            die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
        }
        metatileCount = attributesDataBuf.size() / BYTES_PER_ATTRIBUTE_EMERALD;
    }

    for (std::size_t metatileIndex = 0; metatileIndex < metatileCount; metatileIndex++) {
        Attributes attributes{};
        if (ctx.targetBaseGame == TargetBaseGame::FIRERED) {
            std::uint32_t byte0 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED));
            std::uint32_t byte1 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED) + 1);
            std::uint32_t byte2 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED) + 2);
            std::uint32_t byte3 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_FIRERED) + 3);
            std::uint32_t attribute = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
            attributes.metatileBehavior = attribute & 0x000001FF;

            // Parse terrain type and emit warning if out of range
            std::uint8_t terrainTypeInt = (attribute >> 9) & 0x0000001F;
            auto maybe_terrain_type = terrainTypeFromIntNoPanic(terrainTypeInt);
            if (!maybe_terrain_type.has_value()) {
                ctx.diag->Report(WarnAttributeOutOfRange, ctx.diag->Bold(decompilerModeString(mode)),
                                 ctx.diag->Bold(metatileIndex), ctx.diag->Bold(terrainTypeInt),
                                 ctx.diag->Bold("TerrainType"));
                attributes.terrainType = TerrainType::NORMAL;
            } else {
                attributes.terrainType = maybe_terrain_type.value();
            }

            // Parse encounter type and emit warning if out of range
            std::uint8_t encounterTypeInt = (attribute >> 24) & 0x00000007;
            auto maybe_encounter_type = encounterTypeFromIntNoPanic(encounterTypeInt);
            if (!maybe_encounter_type.has_value()) {
                ctx.diag->Report(WarnAttributeOutOfRange, ctx.diag->Bold(decompilerModeString(mode)),
                                 ctx.diag->Bold(metatileIndex), ctx.diag->Bold(encounterTypeInt),
                                 ctx.diag->Bold("EncounterType"));
                attributes.encounterType = EncounterType::NONE;
            } else {
                attributes.encounterType = maybe_encounter_type.value();
            }

            // Parse layer type and emit warning if out of range
            std::uint8_t layerTypeInt = (attribute >> 29) & 0x00000003;
            auto maybe_layer_type = layerTypeFromIntNoPanic(layerTypeInt);
            if (!maybe_layer_type.has_value()) {
                ctx.diag->Report(WarnAttributeOutOfRange, ctx.diag->Bold(decompilerModeString(mode)),
                                 ctx.diag->Bold(metatileIndex), ctx.diag->Bold(layerTypeInt),
                                 ctx.diag->Bold("LayerType"));
                attributes.layerType = LayerType::NORMAL;
            } else {
                attributes.layerType = maybe_layer_type.value();
            }
        } else {
            std::uint16_t byte0 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_EMERALD));
            std::uint16_t byte1 = attributesDataBuf.at((metatileIndex * BYTES_PER_ATTRIBUTE_EMERALD) + 1);
            std::uint16_t attribute = (byte1 << 8) | byte0;
            attributes.metatileBehavior = attribute & 0x00FF;

            // Parse layer type and emit warning if out of range
            std::uint8_t layerTypeVal = (attribute >> 12) & 0x000F;
            auto maybe_layer_type = layerTypeFromIntNoPanic(layerTypeVal);
            if (!maybe_layer_type.has_value()) {
                ctx.diag->Report(WarnAttributeOutOfRange, ctx.diag->Bold(decompilerModeString(mode)),
                                 ctx.diag->Bold(metatileIndex), ctx.diag->Bold(layerTypeVal),
                                 ctx.diag->Bold("LayerType"));
                attributes.layerType = LayerType::NORMAL;
            } else {
                attributes.layerType = maybe_layer_type.value();
            }
        }
        attributesMap.insert(std::pair{metatileIndex, attributes});
    }
    return attributesMap;
}

static std::vector<CompiledAnimation>
importCompiledAnimations(PorytilesContext &ctx, DecompilerMode mode,
                         const std::vector<std::vector<AnimationPng<png::index_pixel>>> &rawAnims) {
    std::vector<CompiledAnimation> anims{};
    for (const auto &rawAnim : rawAnims) {
        std::set<png::uint_32> frameWidths{};
        std::set<png::uint_32> frameHeights{};
        if (rawAnim.empty()) {
            Panic("importer::importCompiledAnimations frames.size() was 0");
        }
        CompiledAnimation compiledAnim{rawAnim.at(0).animName};
        for (const auto &animPng : rawAnim) {
            CompiledAnimFrame animFrame{animPng.frameName};

            if (animPng.png.get_width() % TILE_SIDE_LENGTH_PIX != 0) {
                const auto msg = fmt::format("anim '{}' frame '{}' width '{}' was not divisible by 8",
                                             ctx.diag->Bold(compiledAnim.animName), ctx.diag->Bold(animFrame.frameName),
                                             ctx.diag->Bold(animPng.png.get_width()));
                ctx.diag->Report(FatalGeneric, msg);
                die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
            }
            if (animPng.png.get_height() % TILE_SIDE_LENGTH_PIX != 0) {
                const auto msg = fmt::format("anim '{}' frame '{}' height '{}' was not divisible by 8",
                                             ctx.diag->Bold(compiledAnim.animName), ctx.diag->Bold(animFrame.frameName),
                                             ctx.diag->Bold(animPng.png.get_height()));
                ctx.diag->Report(FatalGeneric, msg);
                die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
            }

            frameWidths.insert(animPng.png.get_width());
            frameHeights.insert(animPng.png.get_height());
            if (frameWidths.size() != 1) {
                const auto msg = fmt::format("anim '{}' frame '{}' width '{}' differed from previous frame width",
                                             ctx.diag->Bold(compiledAnim.animName), ctx.diag->Bold(animFrame.frameName),
                                             ctx.diag->Bold(animPng.png.get_width()));
                ctx.diag->Report(FatalGeneric, msg);
                die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
            }
            if (frameHeights.size() != 1) {
                const auto msg = fmt::format("anim '{}' frame '{}' height '{}' differed from previous frame height",
                                             ctx.diag->Bold(compiledAnim.animName), ctx.diag->Bold(animFrame.frameName),
                                             ctx.diag->Bold(animPng.png.get_height()));
                ctx.diag->Report(FatalGeneric, msg);
                die_decompilationTerminated(ctx, ctx.decompilerSrcPaths.modeBasedSrcPath(mode), msg);
            }

            std::size_t pngWidthInTiles = animPng.png.get_width() / TILE_SIDE_LENGTH_PIX;
            std::size_t pngHeightInTiles = animPng.png.get_height() / TILE_SIDE_LENGTH_PIX;
            for (std::size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
                std::size_t tileRow = tileIndex / pngWidthInTiles;
                std::size_t tileCol = tileIndex % pngWidthInTiles;
                GBATile tile{};
                for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                    std::size_t pixelRow = (tileRow * TILE_SIDE_LENGTH_PIX) + (pixelIndex / TILE_SIDE_LENGTH_PIX);
                    std::size_t pixelCol = (tileCol * TILE_SIDE_LENGTH_PIX) + (pixelIndex % TILE_SIDE_LENGTH_PIX);
                    tile.colorIndexes[pixelIndex] = animPng.png[pixelRow][pixelCol];
                }
                animFrame.tiles.push_back(tile);
            }
            compiledAnim.frames.push_back(animFrame);
        }
        anims.push_back(compiledAnim);
    }

    return anims;
}

std::pair<CompiledTileset, std::unordered_map<std::size_t, Attributes>>
importCompiledTileset(PorytilesContext &ctx, DecompilerMode mode, std::ifstream &metatiles, std::ifstream &attributes,
                      const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap,
                      const png::image<png::index_pixel> &tilesheetPng,
                      const std::vector<std::unique_ptr<std::ifstream>> &paletteFiles,
                      const std::vector<std::string> &paletteFileNames,
                      const std::vector<std::vector<AnimationPng<png::index_pixel>>> &compiledAnims) {
    CompiledTileset tileset{};

    tileset.tiles = importCompiledTiles(ctx, tilesheetPng);
    tileset.palettes = importCompiledPalettes(ctx, mode, paletteFiles, paletteFileNames);
    auto attributesMap = importCompiledMetatileAttributes(ctx, mode, attributes);
    tileset.metatileEntries = importCompiledMetatiles(ctx, mode, metatiles, attributesMap, behaviorReverseMap);
    tileset.anims = importCompiledAnimations(ctx, mode, compiledAnims);

    /*
     * TODO : perform key frame inference here. We have to determine the key frame in order to
     * determine which palette each anim is actually using. If key frame inference fails, skip
     * decompilation of this anim?
     */

    return {tileset, attributesMap};
}

static std::uint8_t consumeJascHeader(const PorytilesContext &ctx, CompilerMode compilerMode,
                                      std::ifstream &paletteFile, const std::string &fileName) {
    /*
     * FIXME : this function assumes the pal file is DOS format, need to fix this
     * Pret PR here recently addressed the gbagfx DOS line ending issue: https://github.com/pret/pokeemerald/pull/2004
     */

    std::string line{};
    std::getline(paletteFile, line);
    if (line.empty()) {
        const auto msg = fmt::format("invalid blank line in pal file: {}", fileName);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (line.at(line.size() - 1) == '\r') {
        line.pop_back();
    }
    if (line != "JASC-PAL") {
        const auto msg = fmt::format("expected 'JASC-PAL' as first line in pal file: {}", fileName);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    std::getline(paletteFile, line);
    if (line.empty()) {
        const auto msg = fmt::format("invalid blank line in pal file: {}", fileName);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (line.at(line.size() - 1) == '\r') {
        line.pop_back();
    }
    if (line != "0100") {
        const auto msg = fmt::format("expected '0100' as second line in pal file: {}", fileName);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    std::getline(paletteFile, line);
    if (line.empty()) {
        const auto msg = fmt::format("invalid blank line in pal file: {}", fileName);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    if (line.at(line.size() - 1) == '\r') {
        line.pop_back();
    }

    std::uint8_t paletteSize{};
    try {
        paletteSize = parseInteger<std::uint8_t>(line.c_str());
    } catch (const std::exception &e) {
        paletteSize = 0;
        const auto msg = fmt::format("invalid pal size in pal file: {}", fileName);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    return paletteSize;
}

RGBATile importPalettePrimer(PorytilesContext &ctx, const CompilerMode compilerMode, std::ifstream &paletteFile,
                             const std::string &fileName) {
    std::unordered_map<BGR15, std::pair<RGBA32, std::size_t>> bgrToRgba{};
    RGBATile primerTile{};
    primerTile.type = TileType::PRIMER;

    std::string line{};
    const std::uint8_t declaredPaletteSize = consumeJascHeader(ctx, compilerMode, paletteFile, fileName);
    if (declaredPaletteSize == 0 || declaredPaletteSize > PAL_SIZE - 1) {
        ctx.diag->Report(ErrGeneric, fmt::format("{} invalid declared size '{}', must be 1 <= size <= 15",
                                                 ctx.diag->Bold(fileName + ":"), ctx.diag->Bold(declaredPaletteSize)));
    }

    std::uint8_t lineCount = 4;
    std::uint8_t usedPaletteCount = 0;
    while (std::getline(paletteFile, line)) {
        const RGBA32 rgba = parseJascLineCompiler(ctx, compilerMode, line, fileName);

        if (const BGR15 bgr = rgbaToBgr(rgba); !bgrToRgba.contains(bgr)) {
            bgrToRgba.insert(std::pair{bgr, std::pair{rgba, lineCount}});
        } else {
            ctx.diag->Report(ErrGeneric,
                             fmt::format("{} illegal BGR-equivalent color '{}', previously saw '{}' on line {}",
                                         ctx.diag->Bold(fileName + ":"), ctx.diag->Bold(rgba.jasc()),
                                         ctx.diag->Bold(bgrToRgba.at(bgr).first.jasc()), bgrToRgba.at(bgr).second));
        }

        if (rgbaToBgr(rgba) == rgbaToBgr(ctx.compilerConfig.transparencyColor)) {
            ctx.diag->Report(ErrGeneric, fmt::format("{} '{}' was transparent or collapsed to transparent",
                                                     ctx.diag->Bold(fileName + ":"), ctx.diag->Bold(rgba.jasc())));
        }

        primerTile.pixels.at(usedPaletteCount) = rgba;

        lineCount++;
        usedPaletteCount++;
        if (usedPaletteCount >= PAL_SIZE - 1) {
            break;
        }
    }

    if (usedPaletteCount != declaredPaletteSize) {
        ctx.diag->Report(ErrGeneric, fmt::format("{} used pal size ({}) did not match declared size '{}'",
                                                 ctx.diag->Bold(fileName + ":"), usedPaletteCount,
                                                 ctx.diag->Bold(declaredPaletteSize)));
    }
    primerTile.primerSize = usedPaletteCount;

    return primerTile;
}

std::pair<RGBATile, OverridenPaletteSlots> importPaletteOverride(PorytilesContext &ctx, const CompilerMode compilerMode,
                                                                 std::ifstream &paletteFile,
                                                                 const std::string &fileName) {
    std::unordered_map<BGR15, std::pair<RGBA32, std::size_t>> bgrToRgba{};
    RGBATile overrideTile{};
    OverridenPaletteSlots overridePaletteSlots{};
    overrideTile.type = TileType::OVERRIDE;

    std::string line{};
    const std::uint8_t declaredPaletteSize = consumeJascHeader(ctx, compilerMode, paletteFile, fileName);
    if (declaredPaletteSize != PAL_SIZE) {
        ctx.diag->Report(ErrGeneric,
                         fmt::format("{} invalid declared size '{}', must be exactly {}",
                                     ctx.diag->Bold(fileName + ":"), ctx.diag->Bold(declaredPaletteSize), PAL_SIZE));
    }

    std::uint8_t lineCount = 4;
    std::getline(paletteFile, line);
    if (line.at(line.size() - 1) == '\r') {
        line.pop_back();
    }
    if (line != "-") {
        const auto msg = fmt::format("{}: 0th override slot must be '-' but saw '{}'", fileName, line);
        ctx.diag->Report(FatalGeneric, msg);
        die_compilationTerminated(ctx, ctx.compilerSrcPaths.modeBasedSrcPath(compilerMode), msg);
    }
    lineCount++;

    // usedPaletteCount starts at 1 since we've already "used" a slot for transparent
    std::uint8_t usedPaletteCount = 1;
    std::uint8_t overriddenSlotCount = 0;
    bool sawAtLeastOneOverriddenColor = false;
    while (std::getline(paletteFile, line)) {
        if (line.at(line.size() - 1) == '\r') {
            line.pop_back();
        }
        if (line != "-") {
            const RGBA32 rgba = parseJascLineCompiler(ctx, compilerMode, line, fileName);
            sawAtLeastOneOverriddenColor = true;
            if (const BGR15 bgr = rgbaToBgr(rgba); !bgrToRgba.contains(bgr)) {
                bgrToRgba.insert(std::pair{bgr, std::pair{rgba, lineCount}});
            } else {
                ctx.diag->Report(ErrGeneric,
                                 fmt::format("{} illegal BGR-equivalent color '{}', previously saw '{}' on line {}",
                                             ctx.diag->Bold(fileName + ":"), ctx.diag->Bold(rgba.jasc()),
                                             ctx.diag->Bold(bgrToRgba.at(bgr).first.jasc()), bgrToRgba.at(bgr).second));
            }

            if (rgbaToBgr(rgba) == rgbaToBgr(ctx.compilerConfig.transparencyColor)) {
                ctx.diag->Report(ErrGeneric, fmt::format("{}: '{}' was transparent or collapsed to transparent",
                                                         ctx.diag->Bold(fileName + ":"), ctx.diag->Bold(rgba.jasc())));
            }
            overrideTile.pixels.at(overriddenSlotCount) = rgba;
            overriddenSlotCount++;
            overridePaletteSlots.emplace_back(usedPaletteCount, rgbaToBgr(rgba));
        }
        lineCount++;
        usedPaletteCount++;
        if (usedPaletteCount >= PAL_SIZE) {
            break;
        }
    }

    if (!sawAtLeastOneOverriddenColor) {
        ctx.diag->Report(ErrGeneric,
                         fmt::format("{} no overridden pal slots were present", ctx.diag->Bold(fileName + ":")));
        ctx.diag->Report(NoteGeneric,
                         fmt::format("this is illegal, a completely empty override is effectively a no-op"));
    }

    if (usedPaletteCount != declaredPaletteSize) {
        ctx.diag->Report(ErrGeneric, fmt::format("{} used palette size ({}) did not match declared size '{}'",
                                                 ctx.diag->Bold(fileName + ":"), usedPaletteCount,
                                                 ctx.diag->Bold(declaredPaletteSize)));
    }

    // minus 1 for transparent
    overrideTile.overrideSize = overriddenSlotCount;

    return {overrideTile, overridePaletteSlots};
}

} // namespace porytiles_legacy

#ifndef DOCTEST_CONFIG_DISABLE
TEST_CASE("importTilesFromPng should read an RGBA PNG into a DecompiledTileset in tile-wise left-to-right, "
          "top-to-bottom order") {
    porytiles_legacy::PorytilesContext ctx{};
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/2x2_pattern_1.png"}));
    png::image<png::rgba_pixel> png1{"Resources/Doctests/2x2_pattern_1.png"};

    porytiles_legacy::DecompiledTileset tiles = porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, png1);

    // Tile 0 should have blue stripe from top left to bottom right
    CHECK(tiles.tiles[0].pixels[0] == porytiles_legacy::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[9] == porytiles_legacy::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[54] == porytiles_legacy::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[63] == porytiles_legacy::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[1] == porytiles_legacy::RGBA_MAGENTA);
    CHECK(tiles.tiles[0].type == porytiles_legacy::TileType::FREESTANDING);
    CHECK(tiles.tiles[0].tileIndex == 0);

    // Tile 1 should have red stripe from top right to bottom left
    CHECK(tiles.tiles[1].pixels[7] == porytiles_legacy::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[14] == porytiles_legacy::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[49] == porytiles_legacy::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[56] == porytiles_legacy::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[0] == porytiles_legacy::RGBA_MAGENTA);
    CHECK(tiles.tiles[1].type == porytiles_legacy::TileType::FREESTANDING);
    CHECK(tiles.tiles[1].tileIndex == 1);

    // Tile 2 should have green stripe from top right to bottom left
    CHECK(tiles.tiles[2].pixels[7] == porytiles_legacy::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[14] == porytiles_legacy::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[49] == porytiles_legacy::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[56] == porytiles_legacy::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[0] == porytiles_legacy::RGBA_MAGENTA);
    CHECK(tiles.tiles[2].type == porytiles_legacy::TileType::FREESTANDING);
    CHECK(tiles.tiles[2].tileIndex == 2);

    // Tile 3 should have yellow stripe from top left to bottom right
    CHECK(tiles.tiles[3].pixels[0] == porytiles_legacy::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[9] == porytiles_legacy::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[54] == porytiles_legacy::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[63] == porytiles_legacy::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[1] == porytiles_legacy::RGBA_MAGENTA);
    CHECK(tiles.tiles[3].type == porytiles_legacy::TileType::FREESTANDING);
    CHECK(tiles.tiles[3].tileIndex == 3);
}

TEST_CASE("importLayeredTilesFromPngs should read the RGBA PNGs into a DecompiledTileset in correct metatile order") {
    porytiles_legacy::PorytilesContext ctx{};

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/simple_metatiles_1/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/simple_metatiles_1/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/simple_metatiles_1/top.png"}));

    png::image<png::rgba_pixel> bottom{"Resources/Doctests/simple_metatiles_1/bottom.png"};
    png::image<png::rgba_pixel> middle{"Resources/Doctests/simple_metatiles_1/middle.png"};
    png::image<png::rgba_pixel> top{"Resources/Doctests/simple_metatiles_1/top.png"};

    porytiles_legacy::DecompiledTileset tiles = porytiles_legacy::importLayeredTilesFromPngs(
        ctx, porytiles_legacy::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles_legacy::Attributes>{}, bottom,
        middle, top);

    // Metatile 0 bottom layer
    CHECK(tiles.tiles[0] == porytiles_legacy::RGBA_TILE_RED);
    CHECK(tiles.tiles[0].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[0].layer == porytiles_legacy::TileLayer::BOTTOM);
    CHECK(tiles.tiles[0].metatileIndex == 0);
    CHECK(tiles.tiles[0].subtile == porytiles_legacy::Subtile::NORTHWEST);
    CHECK(tiles.tiles[1] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[1].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[1].layer == porytiles_legacy::TileLayer::BOTTOM);
    CHECK(tiles.tiles[1].metatileIndex == 0);
    CHECK(tiles.tiles[1].subtile == porytiles_legacy::Subtile::NORTHEAST);
    CHECK(tiles.tiles[2] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[2].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[2].layer == porytiles_legacy::TileLayer::BOTTOM);
    CHECK(tiles.tiles[2].metatileIndex == 0);
    CHECK(tiles.tiles[2].subtile == porytiles_legacy::Subtile::SOUTHWEST);
    CHECK(tiles.tiles[3] == porytiles_legacy::RGBA_TILE_YELLOW);
    CHECK(tiles.tiles[3].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[3].layer == porytiles_legacy::TileLayer::BOTTOM);
    CHECK(tiles.tiles[3].metatileIndex == 0);
    CHECK(tiles.tiles[3].subtile == porytiles_legacy::Subtile::SOUTHEAST);

    // Metatile 0 middle layer
    CHECK(tiles.tiles[4] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[4].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[4].layer == porytiles_legacy::TileLayer::MIDDLE);
    CHECK(tiles.tiles[4].metatileIndex == 0);
    CHECK(tiles.tiles[4].subtile == porytiles_legacy::Subtile::NORTHWEST);
    CHECK(tiles.tiles[5] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[5].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[5].layer == porytiles_legacy::TileLayer::MIDDLE);
    CHECK(tiles.tiles[5].metatileIndex == 0);
    CHECK(tiles.tiles[5].subtile == porytiles_legacy::Subtile::NORTHEAST);
    CHECK(tiles.tiles[6] == porytiles_legacy::RGBA_TILE_GREEN);
    CHECK(tiles.tiles[6].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[6].layer == porytiles_legacy::TileLayer::MIDDLE);
    CHECK(tiles.tiles[6].metatileIndex == 0);
    CHECK(tiles.tiles[6].subtile == porytiles_legacy::Subtile::SOUTHWEST);
    CHECK(tiles.tiles[7] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[7].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[7].layer == porytiles_legacy::TileLayer::MIDDLE);
    CHECK(tiles.tiles[7].metatileIndex == 0);
    CHECK(tiles.tiles[7].subtile == porytiles_legacy::Subtile::SOUTHEAST);

    // Metatile 0 top layer
    CHECK(tiles.tiles[8] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[8].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[8].layer == porytiles_legacy::TileLayer::TOP);
    CHECK(tiles.tiles[8].metatileIndex == 0);
    CHECK(tiles.tiles[8].subtile == porytiles_legacy::Subtile::NORTHWEST);
    CHECK(tiles.tiles[9] == porytiles_legacy::RGBA_TILE_BLUE);
    CHECK(tiles.tiles[9].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[9].layer == porytiles_legacy::TileLayer::TOP);
    CHECK(tiles.tiles[9].metatileIndex == 0);
    CHECK(tiles.tiles[9].subtile == porytiles_legacy::Subtile::NORTHEAST);
    CHECK(tiles.tiles[10] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[10].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[10].layer == porytiles_legacy::TileLayer::TOP);
    CHECK(tiles.tiles[10].metatileIndex == 0);
    CHECK(tiles.tiles[10].subtile == porytiles_legacy::Subtile::SOUTHWEST);
    CHECK(tiles.tiles[11] == porytiles_legacy::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[11].type == porytiles_legacy::TileType::LAYERED);
    CHECK(tiles.tiles[11].layer == porytiles_legacy::TileLayer::TOP);
    CHECK(tiles.tiles[11].metatileIndex == 0);
    CHECK(tiles.tiles[11].subtile == porytiles_legacy::Subtile::SOUTHEAST);
}

TEST_CASE("importAnimTiles should read each animation and correctly populate the DecompiledTileset anims field") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/anim_flower_white"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/anim_flower_yellow"}));

    porytiles_legacy::AnimationPng<png::rgba_pixel> white00{
        png::image<png::rgba_pixel>{"Resources/Doctests/anim_flower_white/00.png"}, "anim_flower_white", "00.png"};
    porytiles_legacy::AnimationPng<png::rgba_pixel> white01{
        png::image<png::rgba_pixel>{"Resources/Doctests/anim_flower_white/01.png"}, "anim_flower_white", "01.png"};
    porytiles_legacy::AnimationPng<png::rgba_pixel> white02{
        png::image<png::rgba_pixel>{"Resources/Doctests/anim_flower_white/02.png"}, "anim_flower_white", "02.png"};

    porytiles_legacy::AnimationPng<png::rgba_pixel> yellow00{
        png::image<png::rgba_pixel>{"Resources/Doctests/anim_flower_yellow/00.png"}, "anim_flower_yellow", "00.png"};
    porytiles_legacy::AnimationPng<png::rgba_pixel> yellow01{
        png::image<png::rgba_pixel>{"Resources/Doctests/anim_flower_yellow/01.png"}, "anim_flower_yellow", "01.png"};
    porytiles_legacy::AnimationPng<png::rgba_pixel> yellow02{
        png::image<png::rgba_pixel>{"Resources/Doctests/anim_flower_yellow/02.png"}, "anim_flower_yellow", "02.png"};

    std::vector<porytiles_legacy::AnimationPng<png::rgba_pixel>> whiteAnim{};
    std::vector<porytiles_legacy::AnimationPng<png::rgba_pixel>> yellowAnim{};

    whiteAnim.push_back(white00);
    whiteAnim.push_back(white01);
    whiteAnim.push_back(white02);

    yellowAnim.push_back(yellow00);
    yellowAnim.push_back(yellow01);
    yellowAnim.push_back(yellow02);

    std::vector<std::vector<porytiles_legacy::AnimationPng<png::rgba_pixel>>> anims{};
    anims.push_back(whiteAnim);
    anims.push_back(yellowAnim);

    porytiles_legacy::DecompiledTileset tiles{};

    porytiles_legacy::importAnimTiles(ctx, porytiles_legacy::CompilerMode::PRIMARY, anims, tiles);

    CHECK(tiles.anims.size() == 2);
    CHECK(tiles.anims.at(0).size() == 3);
    CHECK(tiles.anims.at(1).size() == 3);

    // white flower, frame 0
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame0_tile0.png"}));
    png::image<png::rgba_pixel> frame0Tile0Png{"Resources/Doctests/anim_flower_white/expected/frame0_tile0.png"};
    porytiles_legacy::DecompiledTileset frame0Tile0 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile0Png);
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(0) == frame0Tile0.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(0).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame0_tile1.png"}));
    png::image<png::rgba_pixel> frame0Tile1Png{"Resources/Doctests/anim_flower_white/expected/frame0_tile1.png"};
    porytiles_legacy::DecompiledTileset frame0Tile1 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile1Png);
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(1) == frame0Tile1.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(1).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame0_tile2.png"}));
    png::image<png::rgba_pixel> frame0Tile2Png{"Resources/Doctests/anim_flower_white/expected/frame0_tile2.png"};
    porytiles_legacy::DecompiledTileset frame0Tile2 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile2Png);
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(2) == frame0Tile2.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(2).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame0_tile3.png"}));
    png::image<png::rgba_pixel> frame0Tile3Png{"Resources/Doctests/anim_flower_white/expected/frame0_tile3.png"};
    porytiles_legacy::DecompiledTileset frame0Tile3 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile3Png);
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(3) == frame0Tile3.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(0).tiles.at(3).type == porytiles_legacy::TileType::ANIM);

    // white flower, frame 1
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame1_tile0.png"}));
    png::image<png::rgba_pixel> frame1Tile0Png{"Resources/Doctests/anim_flower_white/expected/frame1_tile0.png"};
    porytiles_legacy::DecompiledTileset frame1Tile0 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile0Png);
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(0) == frame1Tile0.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(0).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame1_tile1.png"}));
    png::image<png::rgba_pixel> frame1Tile1Png{"Resources/Doctests/anim_flower_white/expected/frame1_tile1.png"};
    porytiles_legacy::DecompiledTileset frame1Tile1 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile1Png);
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(1) == frame1Tile1.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(1).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame1_tile2.png"}));
    png::image<png::rgba_pixel> frame1Tile2Png{"Resources/Doctests/anim_flower_white/expected/frame1_tile2.png"};
    porytiles_legacy::DecompiledTileset frame1Tile2 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile2Png);
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(2) == frame1Tile2.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(2).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame1_tile3.png"}));
    png::image<png::rgba_pixel> frame1Tile3Png{"Resources/Doctests/anim_flower_white/expected/frame1_tile3.png"};
    porytiles_legacy::DecompiledTileset frame1Tile3 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile3Png);
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(3) == frame1Tile3.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(1).tiles.at(3).type == porytiles_legacy::TileType::ANIM);

    // white flower, frame 2
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame2_tile0.png"}));
    png::image<png::rgba_pixel> frame2Tile0Png{"Resources/Doctests/anim_flower_white/expected/frame2_tile0.png"};
    porytiles_legacy::DecompiledTileset frame2Tile0 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile0Png);
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(0) == frame2Tile0.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(0).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame2_tile1.png"}));
    png::image<png::rgba_pixel> frame2Tile1Png{"Resources/Doctests/anim_flower_white/expected/frame2_tile1.png"};
    porytiles_legacy::DecompiledTileset frame2Tile1 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile1Png);
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(1) == frame2Tile1.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(1).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame2_tile2.png"}));
    png::image<png::rgba_pixel> frame2Tile2Png{"Resources/Doctests/anim_flower_white/expected/frame2_tile2.png"};
    porytiles_legacy::DecompiledTileset frame2Tile2 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile2Png);
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(2) == frame2Tile2.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(2).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_white/expected/frame2_tile3.png"}));
    png::image<png::rgba_pixel> frame2Tile3Png{"Resources/Doctests/anim_flower_white/expected/frame2_tile3.png"};
    porytiles_legacy::DecompiledTileset frame2Tile3 =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile3Png);
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(3) == frame2Tile3.tiles.at(0));
    CHECK(tiles.anims.at(0).frames.at(2).tiles.at(3).type == porytiles_legacy::TileType::ANIM);

    // yellow flower, frame 0
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame0_tile0.png"}));
    png::image<png::rgba_pixel> frame0Tile0Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame0_tile0.png"};
    porytiles_legacy::DecompiledTileset frame0Tile0_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile0Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(0) == frame0Tile0_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(0).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame0_tile1.png"}));
    png::image<png::rgba_pixel> frame0Tile1Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame0_tile1.png"};
    porytiles_legacy::DecompiledTileset frame0Tile1_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile1Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(1) == frame0Tile1_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(1).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame0_tile2.png"}));
    png::image<png::rgba_pixel> frame0Tile2Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame0_tile2.png"};
    porytiles_legacy::DecompiledTileset frame0Tile2_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile2Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(2) == frame0Tile2_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(2).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame0_tile3.png"}));
    png::image<png::rgba_pixel> frame0Tile3Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame0_tile3.png"};
    porytiles_legacy::DecompiledTileset frame0Tile3_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame0Tile3Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(3) == frame0Tile3_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(0).tiles.at(3).type == porytiles_legacy::TileType::ANIM);

    // yellow flower, frame 1
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame1_tile0.png"}));
    png::image<png::rgba_pixel> frame1Tile0Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame1_tile0.png"};
    porytiles_legacy::DecompiledTileset frame1Tile0_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile0Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(0) == frame1Tile0_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(0).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame1_tile1.png"}));
    png::image<png::rgba_pixel> frame1Tile1Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame1_tile1.png"};
    porytiles_legacy::DecompiledTileset frame1Tile1_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile1Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(1) == frame1Tile1_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(1).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame1_tile2.png"}));
    png::image<png::rgba_pixel> frame1Tile2Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame1_tile2.png"};
    porytiles_legacy::DecompiledTileset frame1Tile2_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile2Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(2) == frame1Tile2_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(2).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame1_tile3.png"}));
    png::image<png::rgba_pixel> frame1Tile3Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame1_tile3.png"};
    porytiles_legacy::DecompiledTileset frame1Tile3_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame1Tile3Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(3) == frame1Tile3_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(1).tiles.at(3).type == porytiles_legacy::TileType::ANIM);

    // yellow flower, frame 2
    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame2_tile0.png"}));
    png::image<png::rgba_pixel> frame2Tile0Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame2_tile0.png"};
    porytiles_legacy::DecompiledTileset frame2Tile0_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile0Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(0) == frame2Tile0_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(0).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame2_tile1.png"}));
    png::image<png::rgba_pixel> frame2Tile1Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame2_tile1.png"};
    porytiles_legacy::DecompiledTileset frame2Tile1_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile1Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(1) == frame2Tile1_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(1).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame2_tile2.png"}));
    png::image<png::rgba_pixel> frame2Tile2Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame2_tile2.png"};
    porytiles_legacy::DecompiledTileset frame2Tile2_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile2Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(2) == frame2Tile2_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(2).type == porytiles_legacy::TileType::ANIM);

    REQUIRE(std::filesystem::exists(
        std::filesystem::path{"Resources/Doctests/anim_flower_yellow/expected/frame2_tile3.png"}));
    png::image<png::rgba_pixel> frame2Tile3Png_yellow{
        "Resources/Doctests/anim_flower_yellow/expected/frame2_tile3.png"};
    porytiles_legacy::DecompiledTileset frame2Tile3_yellow =
        porytiles_legacy::importTilesFromPng(ctx, porytiles_legacy::CompilerMode::PRIMARY, frame2Tile3Png_yellow);
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(3) == frame2Tile3_yellow.tiles.at(0));
    CHECK(tiles.anims.at(1).frames.at(2).tiles.at(3).type == porytiles_legacy::TileType::ANIM);
}

TEST_CASE("importLayeredTilesFromPngs should correctly import a dual layer tileset via layer type inference") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.compilerConfig.tripleLayer = false;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/dual_layer_metatiles_1/bottom.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/dual_layer_metatiles_1/middle.png"}));
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/dual_layer_metatiles_1/top.png"}));

    png::image<png::rgba_pixel> bottom{"Resources/Doctests/dual_layer_metatiles_1/bottom.png"};
    png::image<png::rgba_pixel> middle{"Resources/Doctests/dual_layer_metatiles_1/middle.png"};
    png::image<png::rgba_pixel> top{"Resources/Doctests/dual_layer_metatiles_1/top.png"};

    porytiles_legacy::DecompiledTileset tiles = porytiles_legacy::importLayeredTilesFromPngs(
        ctx, porytiles_legacy::CompilerMode::PRIMARY, std::unordered_map<std::size_t, porytiles_legacy::Attributes>{}, bottom,
        middle, top);

    // Metatile 0
    CHECK(tiles.tiles.at(0).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(1).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(2).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(3).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(4).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(5).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(6).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(7).attributes.layerType == porytiles_legacy::LayerType::COVERED);

    // Metatile 1
    CHECK(tiles.tiles.at(8).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(9).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(10).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(11).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(12).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(13).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(14).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(15).attributes.layerType == porytiles_legacy::LayerType::NORMAL);

    // Metatile 2
    CHECK(tiles.tiles.at(16).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(17).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(18).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(19).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(20).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(21).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(22).attributes.layerType == porytiles_legacy::LayerType::SPLIT);
    CHECK(tiles.tiles.at(23).attributes.layerType == porytiles_legacy::LayerType::SPLIT);

    // Metatile 3
    CHECK(tiles.tiles.at(24).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(25).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(26).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(27).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(28).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(29).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(30).attributes.layerType == porytiles_legacy::LayerType::COVERED);
    CHECK(tiles.tiles.at(31).attributes.layerType == porytiles_legacy::LayerType::COVERED);

    // Metatile 4
    CHECK(tiles.tiles.at(32).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(33).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(34).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(35).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(36).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(37).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(38).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(39).attributes.layerType == porytiles_legacy::LayerType::NORMAL);

    // Metatile 5
    CHECK(tiles.tiles.at(40).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(41).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(42).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(43).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(44).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(45).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(46).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(47).attributes.layerType == porytiles_legacy::LayerType::NORMAL);

    // Metatile 6
    CHECK(tiles.tiles.at(48).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(49).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(50).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(51).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(52).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(53).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(54).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(55).attributes.layerType == porytiles_legacy::LayerType::NORMAL);

    // Metatile 7
    CHECK(tiles.tiles.at(56).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(57).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(58).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(59).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(60).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(61).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(62).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
    CHECK(tiles.tiles.at(63).attributes.layerType == porytiles_legacy::LayerType::NORMAL);
}

TEST_CASE("importMetatileBehaviorHeader should parse metatile behaviors as expected") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;

    std::ifstream behaviorFile{"Resources/Doctests/metatile_behaviors.h"};
    auto [behaviorMap, behaviorReverseMap] =
        porytiles_legacy::importMetatileBehaviorHeader(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorFile);
    behaviorFile.close();

    CHECK(!behaviorMap.contains("MB_INVALID"));
    CHECK(behaviorMap.at("MB_NORMAL") == 0x00);
    CHECK(behaviorMap.at("MB_SHALLOW_WATER") == 0x17);
    CHECK(behaviorMap.at("MB_ICE") == 0x20);
    CHECK(behaviorMap.at("MB_UNUSED_EF") == 0xEF);

    CHECK(!behaviorReverseMap.contains(0xFF));
    CHECK(behaviorReverseMap.at(0x00) == "MB_NORMAL");
    CHECK(behaviorReverseMap.at(0x17) == "MB_SHALLOW_WATER");
    CHECK(behaviorReverseMap.at(0x20) == "MB_ICE");
    CHECK(behaviorReverseMap.at(0xEF) == "MB_UNUSED_EF");
}

TEST_CASE("importAttributesFromCsv should parse source CSVs as expected") {
    porytiles_legacy::PorytilesContext ctx{};
    ctx.printDieMsg = false;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("It should parse an Emerald-style attributes CSV correctly") {
        auto attributesMap = porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "Resources/Doctests/csv/correct_1.csv");
        CHECK_FALSE(attributesMap.contains(0));
        CHECK_FALSE(attributesMap.contains(1));
        CHECK_FALSE(attributesMap.contains(2));
        CHECK(attributesMap.contains(3));
        CHECK_FALSE(attributesMap.contains(4));
        CHECK(attributesMap.contains(5));
        CHECK_FALSE(attributesMap.contains(6));

        CHECK(attributesMap.at(3).metatileBehavior == behaviorMap.at("MB_NORMAL"));
        CHECK(attributesMap.at(5).metatileBehavior == behaviorMap.at("MB_NORMAL"));
    }

    SUBCASE("It should parse a Firered-style attributes CSV correctly") {
        auto attributesMap = porytiles_legacy::importAttributesFromCsv(ctx, porytiles_legacy::CompilerMode::PRIMARY, behaviorMap,
                                                                 "Resources/Doctests/csv/correct_2.csv");
        CHECK_FALSE(attributesMap.contains(0));
        CHECK_FALSE(attributesMap.contains(1));
        CHECK(attributesMap.contains(2));
        CHECK_FALSE(attributesMap.contains(3));
        CHECK(attributesMap.contains(4));
        CHECK_FALSE(attributesMap.contains(5));
        CHECK_FALSE(attributesMap.contains(6));

        CHECK(attributesMap.at(2).metatileBehavior == behaviorMap.at("MB_NORMAL"));
        CHECK(attributesMap.at(2).terrainType == porytiles_legacy::TerrainType::NORMAL);
        CHECK(attributesMap.at(2).encounterType == porytiles_legacy::EncounterType::NONE);
        CHECK(attributesMap.at(4).metatileBehavior == behaviorMap.at("MB_NORMAL"));
        CHECK(attributesMap.at(4).terrainType == porytiles_legacy::TerrainType::NORMAL);
        CHECK(attributesMap.at(4).encounterType == porytiles_legacy::EncounterType::NONE);
    }
}

TEST_CASE("importCompiledTileset should import a triple-layer pokeemerald tileset correctly") {
    porytiles_legacy::PorytilesContext compileCtx{};
    std::filesystem::path parentDir = porytiles_legacy::createTmpdir();
    compileCtx.output.path = parentDir;
    compileCtx.subcommand = porytiles_legacy::Subcommand::COMPILE_PRIMARY;
    compileCtx.printDieMsg = false;
    compileCtx.compilerConfig.primaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;
    compileCtx.compilerConfig.secondaryAssignAlgorithm = porytiles_legacy::AssignAlgorithm::DFS;

    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/anim_metatiles_2/primary"}));
    compileCtx.compilerSrcPaths.primarySourcePath = "Resources/Doctests/anim_metatiles_2/primary";
    REQUIRE(std::filesystem::exists(std::filesystem::path{"Resources/Doctests/metatile_behaviors.h"}));
    compileCtx.compilerSrcPaths.metatileBehaviors = "Resources/Doctests/metatile_behaviors.h";
    porytiles_legacy::drive(compileCtx);

    porytiles_legacy::PorytilesContext decompileCtx{};
    decompileCtx.decompilerSrcPaths.primarySourcePath = parentDir;

    std::ifstream metatiles{decompileCtx.decompilerSrcPaths.primaryMetatilesBin(), std::ios::binary};
    std::ifstream attributes{decompileCtx.decompilerSrcPaths.primaryAttributesBin(), std::ios::binary};
    png::image<png::index_pixel> tilesheetPng{decompileCtx.decompilerSrcPaths.primaryTilesPng()};
    std::vector<std::unique_ptr<std::ifstream>> paletteFiles{};
    std::vector<std::string> paletteFileNames{};
    for (std::size_t index = 0; index < decompileCtx.fieldmapConfig.numPalettesTotal; index++) {
        std::ostringstream filename;
        if (index < 10) {
            filename << "0";
        }
        filename << index << ".pal";
        std::filesystem::path paletteFile = decompileCtx.decompilerSrcPaths.primaryPalettes() / filename.str();
        paletteFiles.push_back(std::make_unique<std::ifstream>(paletteFile));
        paletteFileNames.emplace_back(paletteFile.c_str());
    }
    // TODO tests : (importCompiledTileset should import a triple-layer...) actually test anims import
    auto [importedTileset, attributesMap] = porytiles_legacy::importCompiledTileset(
        decompileCtx, porytiles_legacy::DecompilerMode::PRIMARY, metatiles, attributes,
        std::unordered_map<std::uint8_t, std::string>{}, tilesheetPng, paletteFiles, paletteFileNames,
        std::vector<std::vector<porytiles_legacy::AnimationPng<png::index_pixel>>>{});
    metatiles.close();
    attributes.close();
    std::for_each(paletteFiles.begin(), paletteFiles.end(),
                  [](const std::unique_ptr<std::ifstream> &stream) { stream->close(); });

    CHECK((compileCtx.compilerContext.resultTileset)->tiles.size() == importedTileset.tiles.size());
    CHECK((compileCtx.compilerContext.resultTileset)->tiles == importedTileset.tiles);

    CHECK((compileCtx.compilerContext.resultTileset)->metatileEntries.size() == importedTileset.metatileEntries.size());
    for (std::size_t entryIndex = 0; entryIndex < importedTileset.metatileEntries.size(); entryIndex++) {
        const porytiles_legacy::MetatileEntry &expectedEntry =
            (compileCtx.compilerContext.resultTileset)->metatileEntries.at(entryIndex);
        const porytiles_legacy::MetatileEntry &actualEntry = importedTileset.metatileEntries.at(entryIndex);
        CHECK(expectedEntry.tileIndex == actualEntry.tileIndex);
        CHECK(expectedEntry.hFlip == actualEntry.hFlip);
        CHECK(expectedEntry.vFlip == actualEntry.vFlip);
        CHECK(expectedEntry.paletteIndex == actualEntry.paletteIndex);
        CHECK(expectedEntry.attributes.metatileBehavior == actualEntry.attributes.metatileBehavior);
        CHECK(expectedEntry.attributes.layerType == actualEntry.attributes.layerType);
    }

    for (std::size_t palIndex = 0; palIndex < (compileCtx.compilerContext.resultTileset)->palettes.size(); palIndex++) {
        auto origPal = (compileCtx.compilerContext.resultTileset)->palettes.at(palIndex).colors;
        auto decompPal = importedTileset.palettes.at(palIndex).colors;
        for (std::size_t colorIndex = 0; colorIndex < 16; colorIndex++) {
            CHECK(origPal.at(colorIndex) == decompPal.at(colorIndex));
        }
    }

    // TODO tests : (importCompiledTileset should import a triple-layer...) check attributes map correctness

    std::filesystem::remove_all(parentDir);
}

TEST_CASE("importCompiledTileset should import a dual-layer pokefirered tileset correctly") {
    // TODO tests : (importCompiledTileset should import a dual-layer pokefirered tileset correctly)
}
#endif // DOCTEST_CONFIG_DISABLE
