#include "compiler.h"

#include <algorithm>
#include <bitset>
#include <doctest.h>
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
#include "ptcontext.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {
static std::size_t insertRGBA(PtContext &ctx, const RGBATile &rgbaFrame, const RGBA32 &transparencyColor,
                              NormalizedPalette &palette, RGBA32 rgba, std::size_t row, std::size_t col, bool errWarn)
{
  /*
   * Insert an rgba32 color into a normalized palette. The color will be converted to bgr15 format in the process,
   * and possibly deduped (depending on user settings). Transparent alpha pixels will be treated as transparent, as
   * will pixels that are of transparent color (again, set by the user but default to magenta). Fails if a tile
   * contains too many unique colors or if an invalid alpha value is detected.
   */
  if (rgba.alpha == ALPHA_TRANSPARENT || rgba == transparencyColor) {
    return 0;
  }
  else if (rgba.alpha == ALPHA_OPAQUE) {
    /*
     * TODO : we lose color precision here, it would be nice to warn the user if two distinct RGBA colors they used
     * in the master sheet are going to collapse to one BGR color on the GBA. This should default fail the build,
     * but a compiler flag '--ignore-color-precision-loss' would disable this warning
     */
    auto bgr = rgbaToBgr(rgba);
    auto itrAtBgr = std::find(std::begin(palette.colors) + 1, std::begin(palette.colors) + palette.size, bgr);
    auto bgrPosInPalette = itrAtBgr - std::begin(palette.colors);
    if (bgrPosInPalette == palette.size) {
      // palette size will grow as we add to it
      if (palette.size == PAL_SIZE) {
        if (errWarn) {
          error_tooManyUniqueColorsInTile(ctx.err, rgbaFrame, row, col);
        }
        return INVALID_INDEX_PIXEL_VALUE;
      }
      palette.colors.at(palette.size++) = bgr;
    }
    return bgrPosInPalette;
  }
  if (errWarn) {
    error_invalidAlphaValue(ctx.err, rgbaFrame, rgba.alpha, row, col);
  }
  return INVALID_INDEX_PIXEL_VALUE;
}

static NormalizedTile candidate(PtContext &ctx, const RGBA32 &transparencyColor,
                                const std::vector<RGBATile> &rgbaFrames, bool hFlip, bool vFlip, bool errWarn)
{
  /*
   * NOTE: This only produces a _candidate_ normalized tile (a different choice of hFlip/vFlip might be the normal
   * form). We'll use this to generate candidates to find the true normal form.
   */
  // TODO : same color precision note as above in insertRGBA
  NormalizedTile candidateTile{transparencyColor};
  candidateTile.hFlip = hFlip;
  candidateTile.vFlip = vFlip;
  candidateTile.frames.resize(rgbaFrames.size());

  std::size_t frame = 0;
  for (const auto &rgba : rgbaFrames) {
    for (std::size_t row = 0; row < TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < TILE_SIDE_LENGTH; col++) {
        std::size_t rowWithFlip = vFlip ? TILE_SIDE_LENGTH - 1 - row : row;
        std::size_t colWithFlip = hFlip ? TILE_SIDE_LENGTH - 1 - col : col;
        std::size_t pixelValue = insertRGBA(ctx, rgba, transparencyColor, candidateTile.palette,
                                            rgba.getPixel(rowWithFlip, colWithFlip), row, col, errWarn);
        candidateTile.setPixel(frame, row, col, pixelValue);
      }
    }
    frame++;
  }

  return candidateTile;
}

static NormalizedTile normalize(PtContext &ctx, const std::vector<RGBATile> &rgbaFrames)
{
  /*
   * Normalize the given tile by checking each of the 4 possible flip states, and choosing the one that comes first in
   * "lexicographic" order, where this order is determined by the std::array spaceship operator.
   */
  auto noFlipsTile = candidate(ctx, ctx.compilerConfig.transparencyColor, rgbaFrames, false, false, true);

  // Short-circuit because transparent tiles are common in metatiles and trivially in normal form.
  if (noFlipsTile.transparent()) {
    if (rgbaFrames.at(0).type == TileType::LAYERED || rgbaFrames.at(0).type == TileType::FREESTANDING) {
      pt_logln(ctx, stderr, "{}:{}:{} = transparent", layerString(rgbaFrames.at(0).layer),
               rgbaFrames.at(0).metatileIndex, subtileString(rgbaFrames.at(0).subtile));
    }
    return noFlipsTile;
  }

  auto hFlipTile = candidate(ctx, ctx.compilerConfig.transparencyColor, rgbaFrames, true, false, false);
  auto vFlipTile = candidate(ctx, ctx.compilerConfig.transparencyColor, rgbaFrames, false, true, false);
  auto bothFlipsTile = candidate(ctx, ctx.compilerConfig.transparencyColor, rgbaFrames, true, true, false);

  std::array<const NormalizedTile *, 4> candidates = {&noFlipsTile, &hFlipTile, &vFlipTile, &bothFlipsTile};
  auto normalizedTile = std::min_element(std::begin(candidates), std::end(candidates), [](auto tile1, auto tile2) {
    return tile1->representativeFrame() < tile2->representativeFrame();
  });

  if (rgbaFrames.at(0).type == TileType::LAYERED || rgbaFrames.at(0).type == TileType::FREESTANDING) {
    pt_logln(ctx, stderr, "{}:{}:{} = [hFlip: {}, vFlip: {}]", layerString(rgbaFrames.at(0).layer),
             rgbaFrames.at(0).metatileIndex, subtileString(rgbaFrames.at(0).subtile), (**normalizedTile).hFlip,
             (**normalizedTile).vFlip);
  }

  return **normalizedTile;
}

static std::vector<IndexedNormTile> normalizeDecompTiles(PtContext &ctx, const DecompiledTileset &decompiledTileset)
{
  /*
   * For each tile in the decomp tileset, normalize it and tag it with its index in the decomp tileset. We tag the
   * animated tiles first, then tag the regular assignment tiles.
   */
  std::vector<IndexedNormTile> normalizedTiles;

  for (std::size_t animIndex = 0; animIndex < decompiledTileset.anims.size(); animIndex++) {
    const auto &anim = decompiledTileset.anims.at(animIndex);
    // We have already validated that all frames have identical dimensions, so we can use the representative frame here
    for (std::size_t tileIndex = 0; tileIndex < anim.representativeFrame().size(); tileIndex++) {
      std::vector<RGBATile> multiFrameTile{};
      // For each tile, push all frames of the tile into a vector
      for (std::size_t frameIndex = 0; frameIndex < anim.size(); frameIndex++) {
        multiFrameTile.push_back(anim.frames.at(frameIndex).tiles.at(tileIndex));
      }
      DecompiledIndex index{};
      auto normalizedTile = normalize(ctx, multiFrameTile);
      index.animated = true;
      index.animIndex = animIndex;
      index.tileIndex = tileIndex;
      normalizedTiles.emplace_back(index, normalizedTile);
    }
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "errors generated during animated tile normalization");
  }

  std::size_t tileIndex = 0;
  for (const auto &tile : decompiledTileset.tiles) {
    std::vector<RGBATile> singleFrameTile = {tile};
    auto normalizedTile = normalize(ctx, singleFrameTile);
    DecompiledIndex index{};
    index.tileIndex = tileIndex++;
    normalizedTiles.emplace_back(index, normalizedTile);
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "errors generated during tile normalization");
  }

  return normalizedTiles;
}

static std::pair<std::unordered_map<BGR15, std::size_t>, std::unordered_map<std::size_t, BGR15>>
buildColorIndexMaps(PtContext &ctx, const std::vector<IndexedNormTile> &normalizedTiles,
                    const std::unordered_map<BGR15, std::size_t> &primaryIndexMap)
{
  /*
   * Iterate over every color in each tile's NormalizedPalette, adding it to the map if not already present. We end up
   * with a map of colors to unique indexes. Optionally, we will populate the map with colors from the paired primary
   * set so that secondary tiles can possibly make use of these palettes without doubling up colors.
   */
  std::unordered_map<BGR15, std::size_t> colorIndexes;
  std::unordered_map<std::size_t, BGR15> indexesToColors;
  if (!primaryIndexMap.empty()) {
    for (const auto &[color, index] : primaryIndexMap) {
      auto [insertedValue, wasInserted] = colorIndexes.insert({color, index});
      if (!wasInserted) {
        internalerror("compiler::buildColorIndexMaps colorIndexes.insert failed");
      }
      auto [_, wasInserted2] = indexesToColors.insert(std::pair{index, color});
      if (!wasInserted2) {
        internalerror("compiler::buildColorIndexMaps indexesToColors.insert failed");
      }
    }
  }
  std::size_t colorIndex = primaryIndexMap.size();
  for (const auto &[_, normalizedTile] : normalizedTiles) {
    // i starts at 1, since first color in each palette is the transparency color
    for (int i = 1; i < normalizedTile.palette.size; i++) {
      const BGR15 &color = normalizedTile.palette.colors[i];
      bool inserted = colorIndexes.insert(std::pair{color, colorIndex}).second;
      if (inserted) {
        indexesToColors.insert(std::pair{colorIndex, color});
        colorIndex++;
      }
    }
  }

  /*
   * This error is merely a fail-early heuristic. E.g. just because a primary tileset passes this check does not mean
   * it is actually allocatable.
   */
  if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
    std::size_t allowed = (PAL_SIZE - 1) * ctx.fieldmapConfig.numPalettesInPrimary;
    if (colorIndex > allowed) {
      fatalerror_tooManyUniqueColorsTotal(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, allowed, colorIndex);
    }
  }
  else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    // use numPalettesTotal since secondary tiles can use colors from the primary set
    std::size_t allowed = (PAL_SIZE - 1) * ctx.fieldmapConfig.numPalettesTotal;
    if (colorIndex > allowed) {
      fatalerror_tooManyUniqueColorsTotal(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, allowed, colorIndex);
    }
  }
  else if (ctx.compilerConfig.mode == CompilerMode::FREESTANDING) {
    throw std::runtime_error{"TODO : support FREESTANDING mode"};
  }
  else {
    internalerror_unknownCompilerMode("compiler::buildColorIndexMaps");
  }

  return {colorIndexes, indexesToColors};
}

static ColorSet toColorSet(const std::unordered_map<BGR15, std::size_t> &colorIndexMap,
                           const NormalizedPalette &palette)
{
  /*
   * Set a color set based on a given palette. Each bit in the ColorSet represents if the color at the given index in
   * the supplied color map was present in the palette. E.g. suppose the color map has 12 unique colors. The supplied
   * palette has two colors in it, which correspond to index 2 and index 11. The ColorSet bitset would be:
   * 0010 0000 0001
   */
  ColorSet colorSet;
  // starts at 1, skip the transparent color at slot 0 in the normalized palette
  for (int i = 1; i < palette.size; i++) {
    colorSet.set(colorIndexMap.at(palette.colors.at(i)));
  }
  return colorSet;
}

static std::pair<std::vector<IndexedNormTileWithColorSet>, std::vector<ColorSet>>
matchNormalizedWithColorSets(const std::unordered_map<BGR15, std::size_t> &colorIndexMap,
                             const std::vector<IndexedNormTile> &indexedNormalizedTiles)
{
  std::vector<IndexedNormTileWithColorSet> indexedNormTilesWithColorSets;
  std::unordered_set<ColorSet> uniqueColorSets;
  std::vector<ColorSet> colorSets;
  for (const auto &[index, normalizedTile] : indexedNormalizedTiles) {
    // Compute the ColorSet for this normalized tile, then add it to our indexes
    auto colorSet = toColorSet(colorIndexMap, normalizedTile.palette);
    indexedNormTilesWithColorSets.emplace_back(index, normalizedTile, colorSet);
    if (!uniqueColorSets.contains(colorSet)) {
      colorSets.push_back(colorSet);
      uniqueColorSets.insert(colorSet);
    }
  }
  return std::pair{indexedNormTilesWithColorSets, colorSets};
}

struct AssignState {
  /*
   * One color set for each hardware palette, bits in color set will indicate which colors this HW palette will have.
   * The size of the vector should be fixed to maxPalettes.
   */
  std::vector<ColorSet> hardwarePalettes;

  // The unique color sets from the NormalizedTiles
  std::vector<ColorSet> unassigned;
};

std::size_t gRecurseCount = 0;
static bool assign(const PtContext &ctx, AssignState state, std::vector<ColorSet> &solution,
                   const std::vector<ColorSet> &primaryPalettes)
{
  gRecurseCount++;
  // TODO : this is a horrible hack avert your eyes
  if (gRecurseCount > ctx.compilerConfig.maxRecurseCount) {
    fatalerror_tooManyAssignmentRecurses(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                                         ctx.compilerConfig.maxRecurseCount);
  }

  if (state.unassigned.empty()) {
    // No tiles left to assign, found a solution!
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes), std::back_inserter(solution));
    return true;
  }

  /*
   * We will try to assign the last element to one of the 6 hw palettes, last because it is a vector so easier to
   * add/remove from the end.
   */
  ColorSet &toAssign = state.unassigned.back();

  /*
   * If we are assigning a secondary set, we'll want to first check if any of the primary palettes satisfy the color
   * constraints for this particular tile. That way we can just use the primary palette, since those are available for
   * secondary tiles to freely use.
   */
  if (!primaryPalettes.empty()) {
    for (size_t i = 0; i < primaryPalettes.size(); i++) {
      const ColorSet &palette = primaryPalettes.at(i);
      if ((palette | toAssign).count() == palette.count()) {
        /*
         * This case triggers if `toAssign' shares all its colors with one of the palettes from the primary
         * tileset. In that case, we will just reuse that palette when we make the tile in a later step. So we
         * can prep a recursive call to assign with an unchanged state (other than removing `toAssign')
         */
        std::vector<ColorSet> unassignedCopy;
        std::copy(std::begin(state.unassigned), std::end(state.unassigned), std::back_inserter(unassignedCopy));
        std::vector<ColorSet> hardwarePalettesCopy;
        std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
                  std::back_inserter(hardwarePalettesCopy));
        unassignedCopy.pop_back();
        AssignState updatedState = {hardwarePalettesCopy, unassignedCopy};

        if (assign(ctx, updatedState, solution, primaryPalettes)) {
          return true;
        }
      }
    }
  }

  /*
   * For this next step, we want to sort the hw palettes before we try iterating. Sort them by the size of their
   * intersection with the toAssign ColorSet. Effectively, this means that we will always first try branching into an
   * assignment that re-uses hw palettes more effectively. We also have a tie-breaker heuristic for cases where two
   * palettes have the same intersect size. Right now we just use palette size, but in the future we may want to look
   * at color distances so we can pick a palette with more similar colors.
   */
  std::stable_sort(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
                   [&toAssign](const auto &pal1, const auto &pal2) {
                     std::size_t pal1IntersectSize = (pal1 & toAssign).count();
                     std::size_t pal2IntersectSize = (pal2 & toAssign).count();

                     /*
                      * TODO : Instead of just using palette count, maybe can we check for color distance here and try
                      * to choose the palette that has the "closest" colors to our toAssign palette? That might be a
                      * good heuristic for attempting to keep similar colors in the same palette. I.e. especially in
                      * cases where there are no palette intersections, it may be better to first try placing the new
                      * colors into a palette with similar colors rather than into the smallest palette
                      */
                     if (pal1IntersectSize == pal2IntersectSize) {
                       return pal1.count() < pal2.count();
                     }

                     return pal1IntersectSize > pal2IntersectSize;
                   });

  for (size_t i = 0; i < state.hardwarePalettes.size(); i++) {
    const ColorSet &palette = state.hardwarePalettes.at(i);

    // > PAL_SIZE - 1 because we need to save a slot for transparency
    if ((palette | toAssign).count() > PAL_SIZE - 1) {
      /*
       *  Skip this palette, cannot assign because there is not enough room in the palette. If we end up skipping
       * all of them that means the palettes are all too full and we cannot assign this tile in the state we are
       * in. The algorithm will be forced to backtrack and try other assignments.
       */
      continue;
    }

    /*
     * Prep the recursive call to assign(). If we got here, we know it is possible to assign toAssign to the palette
     * at hardwarePalettes[i]. So we make a copy of unassigned with toAssign removed and a copy of hardwarePalettes
     * with toAssigned assigned to the palette at index i. Then we call assign again with this updated state, and
     * return true if there is a valid solution somewhere down in this recursive branch.
     */
    std::vector<ColorSet> unassignedCopy;
    std::copy(std::begin(state.unassigned), std::end(state.unassigned), std::back_inserter(unassignedCopy));
    std::vector<ColorSet> hardwarePalettesCopy;
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
              std::back_inserter(hardwarePalettesCopy));
    unassignedCopy.pop_back();
    hardwarePalettesCopy.at(i) |= toAssign;
    AssignState updatedState = {hardwarePalettesCopy, unassignedCopy};

    if (assign(ctx, updatedState, solution, primaryPalettes)) {
      return true;
    }
  }

  // No solution found
  return false;
}

static GBATile makeTile(const NormalizedTile &normalizedTile, std::size_t frame, GBAPalette palette)
{
  GBATile gbaTile{};
  std::array<std::uint8_t, PAL_SIZE> paletteIndexes{};
  paletteIndexes.at(0) = 0;
  for (int i = 1; i < normalizedTile.palette.size; i++) {
    auto it = std::find(std::begin(palette.colors) + 1, std::end(palette.colors), normalizedTile.palette.colors[i]);
    if (it == std::end(palette.colors)) {
      internalerror("compiler::makeTile it == std::end(palette.colors)");
    }
    paletteIndexes.at(i) = it - std::begin(palette.colors);
  }

  for (std::size_t i = 0; i < normalizedTile.frames.at(frame).colorIndexes.size(); i++) {
    gbaTile.colorIndexes.at(i) = paletteIndexes.at(normalizedTile.frames.at(frame).colorIndexes.at(i));
  }
  return gbaTile;
}

static void assignTilesPrimary(PtContext &ctx, CompiledTileset &compiled,
                               const std::vector<IndexedNormTileWithColorSet> &indexedNormTilesWithColorSets,
                               const std::vector<ColorSet> &assignedPalsSolution)
{
  std::unordered_map<GBATile, std::size_t> tileIndexes;

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
                             return (colorSet & ~assignedPal).none();
                           });
    if (it == std::end(assignedPalsSolution)) {
      internalerror("compiler::assignTilesPrimary it == std::end(assignedPalsSolution)");
    }
    std::size_t paletteIndex = it - std::begin(assignedPalsSolution);

    // Create the GBATile for this tile's representative frame
    GBATile representativeFrameTile =
        makeTile(normTile, NormalizedTile::representativeFrameIndex(), compiled.palettes.at(paletteIndex));

    /*
     * Warn the user if the representative frame of an animation contained a transparent tile. This is technically OK
     * but typically indicates a user oversight, or a lack of understanding of the animation system. It does not really
     * make sense to ever have a transparent animated tile in the representative frame, since it cannot be correctly
     * referenced from the metatile sheet.
     */
    if (tileIndexes.contains(representativeFrameTile) && tileIndexes.at(representativeFrameTile) == 0) {
      warn_transparentRepresentativeAnimTile(ctx.err);
    }

    // Insert this tile's representative frame into the seen tiles map
    auto inserted = tileIndexes.insert({representativeFrameTile, compiled.tiles.size()});

    // Insertion happened or tile was transparent
    if (inserted.second ||
        (tileIndexes.contains(representativeFrameTile) && tileIndexes.at(representativeFrameTile) == 0)) {
      // Insert this tile's representative frame into the tiles.png
      compiled.tiles.push_back(representativeFrameTile);
      compiled.paletteIndexesOfTile.push_back(paletteIndex);
      // Fill out the anim structure
      compiled.anims.at(index.animIndex)
          .frames.at(NormalizedTile::representativeFrameIndex())
          .tiles.push_back(representativeFrameTile);
    }
    else if (tileIndexes.contains(representativeFrameTile) && tileIndexes.at(representativeFrameTile) != 0) {
      // TODO : better error context
      throw PtException{"detected duplicate representative anim tile, not allowed"};
    }

    // Put the rest of this tile's frames into the anim structure for the emitter
    for (std::size_t frameIndex = 1; frameIndex < normTile.frames.size(); frameIndex++) {
      GBATile frameNTile = makeTile(normTile, frameIndex, compiled.palettes.at(paletteIndex));
      compiled.anims.at(index.animIndex).frames.at(frameIndex).tiles.push_back(frameNTile);
    }
  }

  /*
   * Process regular tiles. The user will have used frame 0 of an animated tile to indicate that a particular metatile
   * has an animated component. Since we already processed animated tiles, we'll link up the assignment. Other tiles
   * will be added and linked.
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
                             return (colorSet & ~assignedPal).none();
                           });
    if (it == std::end(assignedPalsSolution)) {
      internalerror("compiler::assignTilesPrimary it == std::end(assignedPalsSolution)");
    }
    std::size_t paletteIndex = it - std::begin(assignedPalsSolution);
    GBATile gbaTile =
        makeTile(normTile, NormalizedTile::representativeFrameIndex(), compiled.palettes.at(paletteIndex));
    // insert only updates the map if the key is not already present
    auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
    if (inserted.second) {
      compiled.tiles.push_back(gbaTile);
      compiled.paletteIndexesOfTile.push_back(paletteIndex);
    }
    std::size_t tileIndex = inserted.first->second;
    compiled.assignments.at(index.tileIndex) = {tileIndex, paletteIndex, normTile.hFlip, normTile.vFlip};
  }
  compiled.tileIndexes = tileIndexes;

  // error out if there were too many unique tiles
  if (compiled.tiles.size() > ctx.fieldmapConfig.numTilesInPrimary) {
    fatalerror_tooManyUniqueTiles(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, compiled.tiles.size(),
                                  ctx.fieldmapConfig.numTilesInPrimary);
  }

  // TODO : warn user if there are any representative tiles that did not appear in the assignments
}

static void assignTilesSecondary(PtContext &ctx, CompiledTileset &compiled,
                                 const std::vector<IndexedNormTileWithColorSet> &indexedNormTilesWithColorSets,
                                 const std::vector<ColorSet> &primaryPaletteColorSets,
                                 const std::vector<ColorSet> &assignedPalsSolution)
{
  std::vector<ColorSet> allColorSets{};
  allColorSets.insert(allColorSets.end(), primaryPaletteColorSets.begin(), primaryPaletteColorSets.end());
  allColorSets.insert(allColorSets.end(), assignedPalsSolution.begin(), assignedPalsSolution.end());
  std::unordered_map<GBATile, std::size_t> tileIndexes;

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
      return (colorSet & ~assignedPal).none();
    });
    if (it == std::end(allColorSets)) {
      internalerror("compiler::assignTilesSecondary it == std::end(allColorSets)");
    }
    std::size_t paletteIndex = it - std::begin(allColorSets);

    // Create the GBATile for this tile's representative frame
    GBATile representativeFrameTile =
        makeTile(normTile, NormalizedTile::representativeFrameIndex(), compiled.palettes[paletteIndex]);

    /*
     * Warn the user if the representative frame of an animation contained a transparent tile, for same reasons as the
     * primary set case. If representativeFrameTile was elsewhere present in the primary set, this is a user error
     * because it renders the animation inoperable, any reference to the repTile in the secondary set will be linked to
     * the primary tile as opposed to the animation.
     */
    if (ctx.compilerContext.pairedPrimaryTiles->tileIndexes.contains(representativeFrameTile)) {
      if (ctx.compilerContext.pairedPrimaryTiles->tileIndexes.at(representativeFrameTile) == 0) {
        warn_transparentRepresentativeAnimTile(ctx.err);
      }
      else {
        // TODO : better error context
        throw PtException{"representative frame tile was present in paired primary set"};
      }
    }

    // Insert this tile's representative frame into the seen tiles map
    auto inserted = tileIndexes.insert({representativeFrameTile, compiled.tiles.size()});

    // Insertion happened or tile was transparent
    if (inserted.second || (ctx.compilerContext.pairedPrimaryTiles->tileIndexes.contains(representativeFrameTile) &&
                            ctx.compilerContext.pairedPrimaryTiles->tileIndexes.at(representativeFrameTile) == 0)) {
      // Insert this tile's representative frame into the tiles.png
      compiled.tiles.push_back(representativeFrameTile);
      compiled.paletteIndexesOfTile.push_back(paletteIndex);
      // Fill out the anim structure
      compiled.anims.at(index.animIndex)
          .frames.at(NormalizedTile::representativeFrameIndex())
          .tiles.push_back(representativeFrameTile);
    }
    else if (tileIndexes.contains(representativeFrameTile) && tileIndexes.at(representativeFrameTile) != 0) {
      // TODO : better error context
      throw PtException{"detected duplicate representative anim tile, not allowed"};
    }

    // Put the rest of this tile's frames into the anim structure for the emitter
    for (std::size_t frameIndex = 1; frameIndex < normTile.frames.size(); frameIndex++) {
      GBATile frameNTile = makeTile(normTile, frameIndex, compiled.palettes.at(paletteIndex));
      compiled.anims.at(index.animIndex).frames.at(frameIndex).tiles.push_back(frameNTile);
    }
  }

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
      return (colorSet & ~assignedPal).none();
    });
    if (it == std::end(allColorSets)) {
      internalerror("compiler::assignTilesSecondary it == std::end(allColorSets)");
    }
    std::size_t paletteIndex = it - std::begin(allColorSets);
    GBATile gbaTile = makeTile(normTile, NormalizedTile::representativeFrameIndex(), compiled.palettes[paletteIndex]);
    if (ctx.compilerContext.pairedPrimaryTiles->tileIndexes.contains(gbaTile)) {
      // Tile was in the primary set
      compiled.assignments.at(index.tileIndex) = {ctx.compilerContext.pairedPrimaryTiles->tileIndexes.at(gbaTile),
                                                  paletteIndex, normTile.hFlip, normTile.vFlip};
    }
    else {
      // Tile was in the secondary set
      auto inserted = tileIndexes.insert({gbaTile, compiled.tiles.size()});
      if (inserted.second) {
        compiled.tiles.push_back(gbaTile);
        compiled.paletteIndexesOfTile.push_back(paletteIndex);
      }
      std::size_t tileIndex = inserted.first->second;
      // Offset the tile index by the secondary tileset VRAM location, which is just the size of the primary tiles
      compiled.assignments.at(index.tileIndex) = {tileIndex + ctx.fieldmapConfig.numTilesInPrimary, paletteIndex,
                                                  normTile.hFlip, normTile.vFlip};
    }
  }
  compiled.tileIndexes = tileIndexes;

  // error out if there were too many unique tiles
  if (compiled.tiles.size() > ctx.fieldmapConfig.numTilesInSecondary()) {
    fatalerror_tooManyUniqueTiles(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, compiled.tiles.size(),
                                  ctx.fieldmapConfig.numTilesInSecondary());
  }

  // TODO : warn user if there are any representative tiles that did not appear in the assignments
}

std::unique_ptr<CompiledTileset> compile(PtContext &ctx, const DecompiledTileset &decompiledTileset)
{
  if (ctx.compilerConfig.mode == CompilerMode::SECONDARY &&
      (ctx.fieldmapConfig.numPalettesInPrimary != ctx.compilerContext.pairedPrimaryTiles->palettes.size())) {
    internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize("compiler::compile",
                                                             ctx.fieldmapConfig.numPalettesInPrimary,
                                                             ctx.compilerContext.pairedPrimaryTiles->palettes.size());
  }

  auto compiled = std::make_unique<CompiledTileset>();

  if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
    compiled->palettes.resize(ctx.fieldmapConfig.numPalettesInPrimary);
    std::size_t inputMetatileCount = (decompiledTileset.tiles.size() / ctx.fieldmapConfig.numTilesPerMetatile);
    if (inputMetatileCount > ctx.fieldmapConfig.numMetatilesInPrimary) {
      throw PtException{"input metatile count (" + std::to_string(inputMetatileCount) +
                        ") exceeded primary metatile limit (" +
                        std::to_string(ctx.fieldmapConfig.numMetatilesInPrimary) + ")"};
    }
  }
  else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    compiled->palettes.resize(ctx.fieldmapConfig.numPalettesTotal);
    std::size_t inputMetatileCount = (decompiledTileset.tiles.size() / ctx.fieldmapConfig.numTilesPerMetatile);
    if (inputMetatileCount > ctx.fieldmapConfig.numMetatilesInSecondary()) {
      throw PtException{"input metatile count (" + std::to_string(inputMetatileCount) +
                        ") exceeded secondary metatile limit (" +
                        std::to_string(ctx.fieldmapConfig.numMetatilesInSecondary()) + ")"};
    }
  }
  else if (ctx.compilerConfig.mode == CompilerMode::FREESTANDING) {
    throw std::runtime_error{"TODO : support FREESTANDING mode"};
  }
  else {
    internalerror_unknownCompilerMode("compiler::compile");
  }
  compiled->assignments.resize(decompiledTileset.tiles.size());

  /*
   * Build indexed normalized tiles, order of this vector matches the decompiled iteration order, with animated tiles
   * at the beginning
   */
  std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(ctx, decompiledTileset);

  /*
   * Map each unique color to a unique index between 0 and 240 (15 colors per palette * 16 palettes MAX)
   */
  std::unordered_map<BGR15, std::size_t> emptyPrimaryColorIndexMap;
  const std::unordered_map<BGR15, std::size_t> *primaryColorIndexMap = &emptyPrimaryColorIndexMap;
  if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    primaryColorIndexMap = &(ctx.compilerContext.pairedPrimaryTiles->colorIndexMap);
  }
  auto [colorToIndex, indexToColor] = buildColorIndexMaps(ctx, indexedNormTiles, *primaryColorIndexMap);
  compiled->colorIndexMap = colorToIndex;

  /*
   * colorSets is a vector: this enforces a well-defined ordering so tileset compilation results are identical across
   * all compilers and platforms. A ColorSet is just a bitset<240> that marks which colors are present (indexes are
   * based on the colorIndexMaps from above)
   */
  auto [indexedNormTilesWithColorSets, colorSets] = matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

  /*
   * Run palette assignment:
   * `assignedPalsSolution' is an out param that the assign function will populate when it finds a solution
   */
  std::vector<ColorSet> assignedPalsSolution;
  std::vector<ColorSet> tmpHardwarePalettes;
  if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
    assignedPalsSolution.reserve(ctx.fieldmapConfig.numPalettesInPrimary);
    tmpHardwarePalettes.resize(ctx.fieldmapConfig.numPalettesInPrimary);
  }
  else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    assignedPalsSolution.reserve(ctx.fieldmapConfig.numPalettesInSecondary());
    tmpHardwarePalettes.resize(ctx.fieldmapConfig.numPalettesInSecondary());
  }
  else if (ctx.compilerConfig.mode == CompilerMode::FREESTANDING) {
    throw std::runtime_error{"TODO : support FREESTANDING mode"};
  }
  else {
    internalerror_unknownCompilerMode("compiler::compile");
  }
  std::vector<ColorSet> unassignedNormPalettes;
  std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassignedNormPalettes));
  std::stable_sort(std::begin(unassignedNormPalettes), std::end(unassignedNormPalettes),
                   [](const auto &cs1, const auto &cs2) { return cs1.count() < cs2.count(); });
  std::vector<ColorSet> primaryPaletteColorSets{};
  if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    /*
     * Construct ColorSets for the primary palettes, assign can use these to decide if a tile is entirely covered by a
     * primary palette and hence does not need to extend the search by assigning its colors to one of the new secondary
     * palettes.
     */
    primaryPaletteColorSets.reserve(ctx.compilerContext.pairedPrimaryTiles->palettes.size());
    for (std::size_t i = 0; i < ctx.compilerContext.pairedPrimaryTiles->palettes.size(); i++) {
      const auto &gbaPalette = ctx.compilerContext.pairedPrimaryTiles->palettes.at(i);
      primaryPaletteColorSets.emplace_back();
      for (std::size_t j = 1; j < gbaPalette.size; j++) {
        primaryPaletteColorSets.at(i).set(colorToIndex.at(gbaPalette.colors.at(j)));
      }
    }
  }

  AssignState state = {tmpHardwarePalettes, unassignedNormPalettes};
  gRecurseCount = 0;
  bool assignSuccessful = assign(ctx, state, assignedPalsSolution, primaryPaletteColorSets);
  if (!assignSuccessful) {
    // TODO : better error context
    throw PtException{"failed to allocate palettes"};
  }

  /*
   * Copy the assignments into the compiled palettes. In a future version we will support sibling tiles (tile sharing)
   * and so we may need to do something fancier here so that the colors align correctly.
   */
  if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
    for (std::size_t i = 0; i < ctx.fieldmapConfig.numPalettesInPrimary; i++) {
      ColorSet palAssignments = assignedPalsSolution.at(i);
      compiled->palettes.at(i).colors.at(0) = rgbaToBgr(ctx.compilerConfig.transparencyColor);
      std::size_t colorIndex = 1;
      for (std::size_t j = 0; j < palAssignments.size(); j++) {
        if (palAssignments.test(j)) {
          compiled->palettes.at(i).colors.at(colorIndex) = indexToColor.at(j);
          colorIndex++;
        }
      }
      compiled->palettes.at(i).size = colorIndex;
    }
  }
  else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    for (std::size_t i = 0; i < ctx.fieldmapConfig.numPalettesInPrimary; i++) {
      // Copy the primary set's palettes into this tileset so tiles can use them
      for (std::size_t j = 0; j < PAL_SIZE; j++) {
        compiled->palettes.at(i).colors.at(j) = ctx.compilerContext.pairedPrimaryTiles->palettes.at(i).colors.at(j);
      }
    }
    for (std::size_t i = ctx.fieldmapConfig.numPalettesInPrimary; i < ctx.fieldmapConfig.numPalettesTotal; i++) {
      ColorSet palAssignments = assignedPalsSolution.at(i - ctx.fieldmapConfig.numPalettesInPrimary);
      compiled->palettes.at(i).colors.at(0) = rgbaToBgr(ctx.compilerConfig.transparencyColor);
      std::size_t colorIndex = 1;
      for (std::size_t j = 0; j < palAssignments.size(); j++) {
        if (palAssignments.test(j)) {
          compiled->palettes.at(i).colors.at(colorIndex) = indexToColor.at(j);
          colorIndex++;
        }
      }
      compiled->palettes.at(i).size = colorIndex;
    }
  }
  else if (ctx.compilerConfig.mode == CompilerMode::FREESTANDING) {
    throw std::runtime_error{"TODO : support FREESTANDING mode"};
  }
  else {
    internalerror_unknownCompilerMode("compiler::compile");
  }

  // Setup the compiled animations
  compiled->anims.reserve(decompiledTileset.anims.size());
  for (std::size_t animIndex = 0; animIndex < decompiledTileset.anims.size(); animIndex++) {
    compiled->anims.emplace_back(decompiledTileset.anims.at(animIndex).animName);
    compiled->anims.at(animIndex).frames.reserve(decompiledTileset.anims.at(animIndex).frames.size());
    for (std::size_t frameIndex = 0; frameIndex < decompiledTileset.anims.at(animIndex).frames.size(); frameIndex++) {
      compiled->anims.at(animIndex).frames.emplace_back(
          decompiledTileset.anims.at(animIndex).frames.at(frameIndex).frameName);
    }
  }

  /*
   * Build the tile assignments.
   */
  if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
    assignTilesPrimary(ctx, *compiled, indexedNormTilesWithColorSets, assignedPalsSolution);
  }
  else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    assignTilesSecondary(ctx, *compiled, indexedNormTilesWithColorSets, primaryPaletteColorSets, assignedPalsSolution);
  }
  else if (ctx.compilerConfig.mode == CompilerMode::FREESTANDING) {
    throw std::runtime_error{"TODO : support FREESTANDING mode"};
  }
  else {
    internalerror_unknownCompilerMode("compiler::compile");
  }

  return compiled;
}
} // namespace porytiles

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("insertRGBA should add new colors in order and return the correct index for a given color")
{
  porytiles::PtContext ctx{};
  ctx.err.printErrors = false;

  porytiles::NormalizedPalette palette1{};
  palette1.size = 1;
  palette1.colors = {};

  porytiles::RGBATile dummy{};
  dummy.type = porytiles::TileType::FREESTANDING;
  dummy.tileIndex = 0;

  // Transparent should return 0
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1, porytiles::RGBA_MAGENTA, 0, 0, true) ==
        0);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}, 0, 0, true) == 0);

  // insert colors
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 1);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{8, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 2);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{16, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 3);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{24, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 4);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{32, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 5);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{40, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 6);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{48, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 7);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{56, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 8);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{64, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 9);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 10);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{80, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 11);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{88, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 12);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{96, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 13);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{104, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 14);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 15);

  // repeat colors should return their indexes
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 10);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}, 0, 0, true) == 15);

  // Transparent should still return 0
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1, porytiles::RGBA_MAGENTA, 0, 0, true) ==
        0);
  CHECK(insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1,
                   porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}, 0, 0, true) == 0);

  // Should generate an error, palette full
  insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1, porytiles::RGBA_CYAN, 0, 0, true);
  CHECK(ctx.err.errCount == 1);

  // invalid alpha value, must be opaque or transparent, generates another error
  insertRGBA(ctx, dummy, ctx.compilerConfig.transparencyColor, palette1, porytiles::RGBA32{0, 0, 0, 12}, 0, 0, true);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("candidate should return the NormalizedTile with requested flips")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/corners.png"));
  png::image<png::rgba_pixel> png1{"res/tests/corners.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
  porytiles::RGBATile tile = tiles.tiles[0];

  SUBCASE("case: no flips")
  {
    std::vector<porytiles::RGBATile> singleFrameTile = {tile};
    porytiles::NormalizedTile candidate =
        porytiles::candidate(ctx, ctx.compilerConfig.transparencyColor, singleFrameTile, false, false, true);
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
    CHECK(candidate.representativeFrame().colorIndexes[0] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[7] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[9] == 3);
    CHECK(candidate.representativeFrame().colorIndexes[14] == 4);
    CHECK(candidate.representativeFrame().colorIndexes[18] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[21] == 5);
    CHECK(candidate.representativeFrame().colorIndexes[42] == 3);
    CHECK(candidate.representativeFrame().colorIndexes[45] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[49] == 6);
    CHECK(candidate.representativeFrame().colorIndexes[54] == 7);
    CHECK(candidate.representativeFrame().colorIndexes[56] == 8);
    CHECK(candidate.representativeFrame().colorIndexes[63] == 5);
  }

  SUBCASE("case: hFlip")
  {
    std::vector<porytiles::RGBATile> singleFrameTile = {tile};
    porytiles::NormalizedTile candidate =
        porytiles::candidate(ctx, ctx.compilerConfig.transparencyColor, singleFrameTile, true, false, true);
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
    CHECK(candidate.representativeFrame().colorIndexes[0] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[7] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[9] == 3);
    CHECK(candidate.representativeFrame().colorIndexes[14] == 4);
    CHECK(candidate.representativeFrame().colorIndexes[18] == 5);
    CHECK(candidate.representativeFrame().colorIndexes[21] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[42] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[45] == 4);
    CHECK(candidate.representativeFrame().colorIndexes[49] == 6);
    CHECK(candidate.representativeFrame().colorIndexes[54] == 7);
    CHECK(candidate.representativeFrame().colorIndexes[56] == 5);
    CHECK(candidate.representativeFrame().colorIndexes[63] == 8);
  }

  SUBCASE("case: vFlip")
  {
    std::vector<porytiles::RGBATile> singleFrameTile = {tile};
    porytiles::NormalizedTile candidate =
        porytiles::candidate(ctx, ctx.compilerConfig.transparencyColor, singleFrameTile, false, true, true);
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
    CHECK(candidate.representativeFrame().colorIndexes[0] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[7] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[9] == 3);
    CHECK(candidate.representativeFrame().colorIndexes[14] == 4);
    CHECK(candidate.representativeFrame().colorIndexes[18] == 5);
    CHECK(candidate.representativeFrame().colorIndexes[21] == 6);
    CHECK(candidate.representativeFrame().colorIndexes[42] == 7);
    CHECK(candidate.representativeFrame().colorIndexes[45] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[49] == 5);
    CHECK(candidate.representativeFrame().colorIndexes[54] == 8);
    CHECK(candidate.representativeFrame().colorIndexes[56] == 6);
    CHECK(candidate.representativeFrame().colorIndexes[63] == 7);
  }

  SUBCASE("case: hFlip and vFlip")
  {
    std::vector<porytiles::RGBATile> singleFrameTile = {tile};
    porytiles::NormalizedTile candidate =
        porytiles::candidate(ctx, ctx.compilerConfig.transparencyColor, singleFrameTile, true, true, true);
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
    CHECK(candidate.representativeFrame().colorIndexes[0] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[7] == 2);
    CHECK(candidate.representativeFrame().colorIndexes[9] == 3);
    CHECK(candidate.representativeFrame().colorIndexes[14] == 4);
    CHECK(candidate.representativeFrame().colorIndexes[18] == 5);
    CHECK(candidate.representativeFrame().colorIndexes[21] == 6);
    CHECK(candidate.representativeFrame().colorIndexes[42] == 1);
    CHECK(candidate.representativeFrame().colorIndexes[45] == 7);
    CHECK(candidate.representativeFrame().colorIndexes[49] == 8);
    CHECK(candidate.representativeFrame().colorIndexes[54] == 6);
    CHECK(candidate.representativeFrame().colorIndexes[56] == 7);
    CHECK(candidate.representativeFrame().colorIndexes[63] == 5);
  }
}

TEST_CASE("normalize should return the normal form of the given tile")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/corners.png"));
  png::image<png::rgba_pixel> png1{"res/tests/corners.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
  porytiles::RGBATile tile = tiles.tiles[0];

  std::vector<porytiles::RGBATile> singleFrameTile = {tile};
  porytiles::NormalizedTile normalizedTile = porytiles::normalize(ctx, singleFrameTile);
  CHECK(normalizedTile.palette.size == 9);
  CHECK_FALSE(normalizedTile.hFlip);
  CHECK_FALSE(normalizedTile.vFlip);
  CHECK(normalizedTile.representativeFrame().colorIndexes[0] == 1);
  CHECK(normalizedTile.representativeFrame().colorIndexes[7] == 2);
  CHECK(normalizedTile.representativeFrame().colorIndexes[9] == 3);
  CHECK(normalizedTile.representativeFrame().colorIndexes[14] == 4);
  CHECK(normalizedTile.representativeFrame().colorIndexes[18] == 2);
  CHECK(normalizedTile.representativeFrame().colorIndexes[21] == 5);
  CHECK(normalizedTile.representativeFrame().colorIndexes[42] == 3);
  CHECK(normalizedTile.representativeFrame().colorIndexes[45] == 1);
  CHECK(normalizedTile.representativeFrame().colorIndexes[49] == 6);
  CHECK(normalizedTile.representativeFrame().colorIndexes[54] == 7);
  CHECK(normalizedTile.representativeFrame().colorIndexes[56] == 8);
  CHECK(normalizedTile.representativeFrame().colorIndexes[63] == 5);
}

TEST_CASE("normalizeDecompTiles should correctly normalize all tiles in the decomp tileset")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);

  std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(ctx, tiles);

  CHECK(indexedNormTiles.size() == 4);

  // First tile normal form is vFlipped, palette should have 2 colors
  CHECK(indexedNormTiles[0].second.representativeFrame().colorIndexes[0] == 0);
  CHECK(indexedNormTiles[0].second.representativeFrame().colorIndexes[7] == 1);
  for (int i = 56; i <= 63; i++) {
    CHECK(indexedNormTiles[0].second.representativeFrame().colorIndexes[i] == 1);
  }
  CHECK(indexedNormTiles[0].second.palette.size == 2);
  CHECK(indexedNormTiles[0].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(indexedNormTiles[0].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
  CHECK_FALSE(indexedNormTiles[0].second.hFlip);
  CHECK(indexedNormTiles[0].second.vFlip);
  CHECK(indexedNormTiles[0].first.tileIndex == 0);

  // Second tile already in normal form, palette should have 3 colors
  CHECK(indexedNormTiles[1].second.representativeFrame().colorIndexes[0] == 0);
  CHECK(indexedNormTiles[1].second.representativeFrame().colorIndexes[54] == 1);
  CHECK(indexedNormTiles[1].second.representativeFrame().colorIndexes[55] == 1);
  CHECK(indexedNormTiles[1].second.representativeFrame().colorIndexes[62] == 1);
  CHECK(indexedNormTiles[1].second.representativeFrame().colorIndexes[63] == 2);
  CHECK(indexedNormTiles[1].second.palette.size == 3);
  CHECK(indexedNormTiles[1].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(indexedNormTiles[1].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
  CHECK(indexedNormTiles[1].second.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
  CHECK_FALSE(indexedNormTiles[1].second.hFlip);
  CHECK_FALSE(indexedNormTiles[1].second.vFlip);
  CHECK(indexedNormTiles[1].first.tileIndex == 1);

  // Third tile normal form is hFlipped, palette should have 3 colors
  CHECK(indexedNormTiles[2].second.representativeFrame().colorIndexes[0] == 0);
  CHECK(indexedNormTiles[2].second.representativeFrame().colorIndexes[7] == 1);
  CHECK(indexedNormTiles[2].second.representativeFrame().colorIndexes[56] == 1);
  CHECK(indexedNormTiles[2].second.representativeFrame().colorIndexes[63] == 2);
  CHECK(indexedNormTiles[2].second.palette.size == 3);
  CHECK(indexedNormTiles[2].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(indexedNormTiles[2].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
  CHECK(indexedNormTiles[2].second.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
  CHECK_FALSE(indexedNormTiles[2].second.vFlip);
  CHECK(indexedNormTiles[2].second.hFlip);
  CHECK(indexedNormTiles[2].first.tileIndex == 2);

  // Fourth tile normal form is hFlipped and vFlipped, palette should have 2 colors
  CHECK(indexedNormTiles[3].second.representativeFrame().colorIndexes[0] == 0);
  CHECK(indexedNormTiles[3].second.representativeFrame().colorIndexes[7] == 1);
  for (int i = 56; i <= 63; i++) {
    CHECK(indexedNormTiles[3].second.representativeFrame().colorIndexes[i] == 1);
  }
  CHECK(indexedNormTiles[3].second.palette.size == 2);
  CHECK(indexedNormTiles[3].second.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(indexedNormTiles[3].second.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
  CHECK(indexedNormTiles[3].second.hFlip);
  CHECK(indexedNormTiles[3].second.vFlip);
  CHECK(indexedNormTiles[3].first.tileIndex == 3);
}

TEST_CASE("normalizeDecompTiles should correctly normalize multi-frame animated tiles")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> tilesPng{"res/tests/2x2_pattern_2.png"};

  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, tilesPng);

  REQUIRE(std::filesystem::exists("res/tests/anim_flower_white"));
  REQUIRE(std::filesystem::exists("res/tests/anim_flower_yellow"));
  REQUIRE(std::filesystem::exists("res/tests/anim_water_1"));

  porytiles::AnimationPng<png::rgba_pixel> white00{png::image<png::rgba_pixel>{"res/tests/anim_flower_white/00.png"},
                                                   "anim_flower_white", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> white01{png::image<png::rgba_pixel>{"res/tests/anim_flower_white/01.png"},
                                                   "anim_flower_white", "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> white02{png::image<png::rgba_pixel>{"res/tests/anim_flower_white/02.png"},
                                                   "anim_flower_white", "02.png"};

  porytiles::AnimationPng<png::rgba_pixel> yellow00{png::image<png::rgba_pixel>{"res/tests/anim_flower_yellow/00.png"},
                                                    "anim_flower_yellow", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> yellow01{png::image<png::rgba_pixel>{"res/tests/anim_flower_yellow/01.png"},
                                                    "anim_flower_yellow", "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> yellow02{png::image<png::rgba_pixel>{"res/tests/anim_flower_yellow/02.png"},
                                                    "anim_flower_yellow", "02.png"};

  porytiles::AnimationPng<png::rgba_pixel> water00{png::image<png::rgba_pixel>{"res/tests/anim_water_1/00.png"},
                                                   "anim_water_1", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> water01{png::image<png::rgba_pixel>{"res/tests/anim_water_1/01.png"},
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

  porytiles::importAnimTiles(ctx, anims, tiles);

  std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(ctx, tiles);

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

TEST_CASE("buildColorIndexMaps should build a map of all unique colors in the decomp tileset")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
  std::vector<IndexedNormTile> normalizedTiles = porytiles::normalizeDecompTiles(ctx, tiles);

  auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(ctx, normalizedTiles, {});

  CHECK(colorToIndex.size() == 4);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);
}

TEST_CASE("toColorSet should return the correct bitset based on the supplied palette")
{
  std::unordered_map<porytiles::BGR15, std::size_t> colorIndexMap = {
      {porytiles::rgbaToBgr(porytiles::RGBA_BLUE), 0},   {porytiles::rgbaToBgr(porytiles::RGBA_RED), 1},
      {porytiles::rgbaToBgr(porytiles::RGBA_GREEN), 2},  {porytiles::rgbaToBgr(porytiles::RGBA_CYAN), 3},
      {porytiles::rgbaToBgr(porytiles::RGBA_YELLOW), 4},
  };

  SUBCASE("palette 1")
  {
    porytiles::NormalizedPalette palette{};
    palette.size = 2;
    palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
    palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_RED);

    ColorSet colorSet = porytiles::toColorSet(colorIndexMap, palette);
    CHECK(colorSet.count() == 1);
    CHECK(colorSet.test(1));
  }

  SUBCASE("palette 2")
  {
    porytiles::NormalizedPalette palette{};
    palette.size = 4;
    palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
    palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_YELLOW);
    palette.colors[2] = porytiles::rgbaToBgr(porytiles::RGBA_GREEN);
    palette.colors[3] = porytiles::rgbaToBgr(porytiles::RGBA_CYAN);

    ColorSet colorSet = porytiles::toColorSet(colorIndexMap, palette);
    CHECK(colorSet.count() == 3);
    CHECK(colorSet.test(4));
    CHECK(colorSet.test(2));
    CHECK(colorSet.test(3));
  }
}

TEST_CASE("matchNormalizedWithColorSets should return the expected data structures")
{
  porytiles::PtContext ctx{};

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
  std::vector<IndexedNormTile> indexedNormTiles = porytiles::normalizeDecompTiles(ctx, tiles);
  auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(ctx, indexedNormTiles, {});

  CHECK(colorToIndex.size() == 4);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 0);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 1);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 2);
  CHECK(colorToIndex[porytiles::rgbaToBgr(porytiles::RGBA_CYAN)] == 3);

  auto [indexedNormTilesWithColorSets, colorSets] =
      porytiles::matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

  CHECK(indexedNormTilesWithColorSets.size() == 4);
  // colorSets size is 3 because first and fourth tiles have the same palette
  CHECK(colorSets.size() == 3);

  // First tile has 1 non-transparent color, color should be BLUE
  CHECK(std::get<0>(indexedNormTilesWithColorSets[0]).tileIndex == 0);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).representativeFrame().colorIndexes[0] == 0);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).representativeFrame().colorIndexes[7] == 1);
  for (int i = 56; i <= 63; i++) {
    CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).representativeFrame().colorIndexes[i] == 1);
  }
  CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.size == 2);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.colors[0] ==
        porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
  CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[0]).hFlip);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[0]).vFlip);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[0]).count() == 1);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[0]).test(0));
  CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[0])) !=
        colorSets.end());

  // Second tile has two non-transparent colors, RED and GREEN
  CHECK(std::get<0>(indexedNormTilesWithColorSets[1]).tileIndex == 1);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).representativeFrame().colorIndexes[0] == 0);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).representativeFrame().colorIndexes[54] == 1);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).representativeFrame().colorIndexes[55] == 1);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).representativeFrame().colorIndexes[62] == 1);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).representativeFrame().colorIndexes[63] == 2);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.size == 3);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[0] ==
        porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[1]).palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
  CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[1]).hFlip);
  CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[1]).vFlip);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).count() == 2);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).test(1));
  CHECK(std::get<2>(indexedNormTilesWithColorSets[1]).test(2));
  CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[1])) !=
        colorSets.end());

  // Third tile has two non-transparent colors, CYAN and GREEN
  CHECK(std::get<0>(indexedNormTilesWithColorSets[2]).tileIndex == 2);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).representativeFrame().colorIndexes[0] == 0);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).representativeFrame().colorIndexes[7] == 1);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).representativeFrame().colorIndexes[56] == 1);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).representativeFrame().colorIndexes[63] == 2);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.size == 3);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[0] ==
        porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
  CHECK_FALSE(std::get<1>(indexedNormTilesWithColorSets[2]).vFlip);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[2]).hFlip);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).count() == 2);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).test(1));
  CHECK(std::get<2>(indexedNormTilesWithColorSets[2]).test(3));
  CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[2])) !=
        colorSets.end());

  // Fourth tile has 1 non-transparent color, color should be BLUE
  CHECK(std::get<0>(indexedNormTilesWithColorSets[3]).tileIndex == 3);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).representativeFrame().colorIndexes[0] == 0);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).representativeFrame().colorIndexes[7] == 1);
  for (int i = 56; i <= 63; i++) {
    CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).representativeFrame().colorIndexes[i] == 1);
  }
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.size == 2);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.colors[0] ==
        porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).hFlip);
  CHECK(std::get<1>(indexedNormTilesWithColorSets[3]).vFlip);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[3]).count() == 1);
  CHECK(std::get<2>(indexedNormTilesWithColorSets[3]).test(0));
  CHECK(std::find(colorSets.begin(), colorSets.end(), std::get<2>(indexedNormTilesWithColorSets[3])) !=
        colorSets.end());
}

TEST_CASE("assign should correctly assign all normalized palettes or fail if impossible")
{
  porytiles::PtContext ctx{};
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  SUBCASE("It should successfully allocate a simple 2x2 tileset png")
  {
    constexpr int SOLUTION_SIZE = 2;
    ctx.fieldmapConfig.numPalettesInPrimary = SOLUTION_SIZE;
    ctx.compilerConfig.maxRecurseCount = 20;

    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
    std::vector<IndexedNormTile> indexedNormTiles = porytiles::normalizeDecompTiles(ctx, tiles);
    auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(ctx, indexedNormTiles, {});
    auto [indexedNormTilesWithColorSets, colorSets] =
        porytiles::matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

    // Set up the state struct
    std::vector<ColorSet> solution;
    solution.reserve(SOLUTION_SIZE);
    std::vector<ColorSet> hardwarePalettes;
    hardwarePalettes.resize(SOLUTION_SIZE);
    std::vector<ColorSet> unassigned;
    std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
    std::stable_sort(std::begin(unassigned), std::end(unassigned),
                     [](const auto &cs1, const auto &cs2) { return cs1.count() < cs2.count(); });
    porytiles::AssignState state = {hardwarePalettes, unassigned};

    porytiles::gRecurseCount = 0;
    CHECK(porytiles::assign(ctx, state, solution, {}));
    CHECK(solution.size() == SOLUTION_SIZE);
    CHECK(solution.at(0).count() == 1);
    CHECK(solution.at(1).count() == 3);
    CHECK(solution.at(0).test(0));
    CHECK(solution.at(1).test(1));
    CHECK(solution.at(1).test(2));
    CHECK(solution.at(1).test(3));
  }

  SUBCASE("It should successfully allocate a large, complex PNG")
  {
    constexpr int SOLUTION_SIZE = 5;
    ctx.fieldmapConfig.numPalettesInPrimary = SOLUTION_SIZE;
    ctx.compilerConfig.maxRecurseCount = 200;

    REQUIRE(std::filesystem::exists("res/tests/compile_raw_set_1/set.png"));
    png::image<png::rgba_pixel> png1{"res/tests/compile_raw_set_1/set.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
    std::vector<IndexedNormTile> indexedNormTiles = porytiles::normalizeDecompTiles(ctx, tiles);
    auto [colorToIndex, indexToColor] = porytiles::buildColorIndexMaps(ctx, indexedNormTiles, {});
    auto [indexedNormTilesWithColorSets, colorSets] =
        porytiles::matchNormalizedWithColorSets(colorToIndex, indexedNormTiles);

    // Set up the state struct
    std::vector<ColorSet> solution;
    solution.reserve(SOLUTION_SIZE);
    std::vector<ColorSet> hardwarePalettes;
    hardwarePalettes.resize(SOLUTION_SIZE);
    std::vector<ColorSet> unassigned;
    std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassigned));
    std::stable_sort(std::begin(unassigned), std::end(unassigned),
                     [](const auto &cs1, const auto &cs2) { return cs1.count() < cs2.count(); });
    porytiles::AssignState state = {hardwarePalettes, unassigned};

    porytiles::gRecurseCount = 0;
    CHECK(porytiles::assign(ctx, state, solution, {}));
    CHECK(solution.size() == SOLUTION_SIZE);
    CHECK(solution.at(0).count() == 11);
    CHECK(solution.at(1).count() == 12);
    CHECK(solution.at(2).count() == 14);
    CHECK(solution.at(3).count() == 14);
    CHECK(solution.at(4).count() == 15);
  }
}

TEST_CASE("makeTile should create the expected GBATile from the given NormalizedTile and GBAPalette")
{
  porytiles::PtContext ctx{};
  ctx.compilerConfig.transparencyColor = porytiles::RGBA_MAGENTA;
  ctx.fieldmapConfig.numPalettesInPrimary = 2;
  ctx.fieldmapConfig.numTilesInPrimary = 4;
  ctx.compilerConfig.maxRecurseCount = 5;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
  std::vector<IndexedNormTile> indexedNormTiles = normalizeDecompTiles(ctx, tiles);
  auto compiledTiles = porytiles::compile(ctx, tiles);

  porytiles::GBATile tile0 = porytiles::makeTile(
      indexedNormTiles[0].second, porytiles::NormalizedTile::representativeFrameIndex(), compiledTiles->palettes[0]);
  CHECK_FALSE(indexedNormTiles[0].second.hFlip);
  CHECK(indexedNormTiles[0].second.vFlip);
  CHECK(tile0.colorIndexes[0] == 0);
  CHECK(tile0.colorIndexes[7] == 1);
  for (size_t i = 56; i < 64; i++) {
    CHECK(tile0.colorIndexes[i] == 1);
  }

  porytiles::GBATile tile1 = porytiles::makeTile(
      indexedNormTiles[1].second, porytiles::NormalizedTile::representativeFrameIndex(), compiledTiles->palettes[1]);
  CHECK_FALSE(indexedNormTiles[1].second.hFlip);
  CHECK_FALSE(indexedNormTiles[1].second.vFlip);
  CHECK(tile1.colorIndexes[0] == 0);
  CHECK(tile1.colorIndexes[54] == 1);
  CHECK(tile1.colorIndexes[55] == 1);
  CHECK(tile1.colorIndexes[62] == 1);
  CHECK(tile1.colorIndexes[63] == 2);

  porytiles::GBATile tile2 = porytiles::makeTile(
      indexedNormTiles[2].second, porytiles::NormalizedTile::representativeFrameIndex(), compiledTiles->palettes[1]);
  CHECK(indexedNormTiles[2].second.hFlip);
  CHECK_FALSE(indexedNormTiles[2].second.vFlip);
  CHECK(tile2.colorIndexes[0] == 0);
  CHECK(tile2.colorIndexes[7] == 3);
  CHECK(tile2.colorIndexes[56] == 3);
  CHECK(tile2.colorIndexes[63] == 1);

  porytiles::GBATile tile3 = porytiles::makeTile(
      indexedNormTiles[3].second, porytiles::NormalizedTile::representativeFrameIndex(), compiledTiles->palettes[0]);
  CHECK(indexedNormTiles[3].second.hFlip);
  CHECK(indexedNormTiles[3].second.vFlip);
  CHECK(tile3.colorIndexes[0] == 0);
  CHECK(tile3.colorIndexes[7] == 1);
  for (size_t i = 56; i < 64; i++) {
    CHECK(tile3.colorIndexes[i] == 1);
  }
}

TEST_CASE("compile simple example should perform as expected")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 2;
  ctx.fieldmapConfig.numTilesInPrimary = 4;
  ctx.compilerConfig.maxRecurseCount = 5;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_2.png"));
  png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_2.png"};
  porytiles::DecompiledTileset tiles = porytiles::importTilesFromPng(ctx, png1);
  auto compiledTiles = porytiles::compile(ctx, tiles);

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
   * Check that all the assignments are correct.
   */
  CHECK(compiledTiles->assignments[0].tileIndex == 1);
  CHECK(compiledTiles->assignments[0].paletteIndex == 0);
  CHECK_FALSE(compiledTiles->assignments[0].hFlip);
  CHECK(compiledTiles->assignments[0].vFlip);

  CHECK(compiledTiles->assignments[1].tileIndex == 2);
  CHECK(compiledTiles->assignments[1].paletteIndex == 1);
  CHECK_FALSE(compiledTiles->assignments[1].hFlip);
  CHECK_FALSE(compiledTiles->assignments[1].vFlip);

  CHECK(compiledTiles->assignments[2].tileIndex == 3);
  CHECK(compiledTiles->assignments[2].paletteIndex == 1);
  CHECK(compiledTiles->assignments[2].hFlip);
  CHECK_FALSE(compiledTiles->assignments[2].vFlip);

  CHECK(compiledTiles->assignments[3].tileIndex == 1);
  CHECK(compiledTiles->assignments[3].paletteIndex == 0);
  CHECK(compiledTiles->assignments[3].hFlip);
  CHECK(compiledTiles->assignments[3].vFlip);
}

TEST_CASE("compile function should fill out primary CompiledTileset struct with expected values")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/top.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_3/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_3/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_3/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomPrimary, middlePrimary, topPrimary);

  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary);

  // Check that tiles are as expected
  CHECK(compiledPrimary->tiles.size() == 5);
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/expected_tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_3/primary/expected_tiles.png"};
  for (std::size_t tileIndex = 0; tileIndex < compiledPrimary->tiles.size(); tileIndex++) {
    for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
        CHECK(compiledPrimary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
              expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]);
      }
    }
  }

  // Check that paletteIndexesOfTile are correct
  CHECK(compiledPrimary->paletteIndexesOfTile.size() == 5);
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

  // Check that all assignments are correct
  CHECK(compiledPrimary->assignments.size() == porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

  CHECK(compiledPrimary->assignments[0].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[0].vFlip);
  CHECK(compiledPrimary->assignments[0].tileIndex == 1);
  CHECK(compiledPrimary->assignments[0].paletteIndex == 2);

  CHECK_FALSE(compiledPrimary->assignments[1].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[1].vFlip);
  CHECK(compiledPrimary->assignments[1].tileIndex == 0);
  CHECK(compiledPrimary->assignments[1].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[2].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[2].vFlip);
  CHECK(compiledPrimary->assignments[2].tileIndex == 0);
  CHECK(compiledPrimary->assignments[2].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[3].hFlip);
  CHECK(compiledPrimary->assignments[3].vFlip);
  CHECK(compiledPrimary->assignments[3].tileIndex == 2);
  CHECK(compiledPrimary->assignments[3].paletteIndex == 1);

  CHECK_FALSE(compiledPrimary->assignments[4].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[4].vFlip);
  CHECK(compiledPrimary->assignments[4].tileIndex == 0);
  CHECK(compiledPrimary->assignments[4].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[5].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[5].vFlip);
  CHECK(compiledPrimary->assignments[5].tileIndex == 0);
  CHECK(compiledPrimary->assignments[5].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[6].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[6].vFlip);
  CHECK(compiledPrimary->assignments[6].tileIndex == 3);
  CHECK(compiledPrimary->assignments[6].paletteIndex == 1);

  CHECK_FALSE(compiledPrimary->assignments[7].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[7].vFlip);
  CHECK(compiledPrimary->assignments[7].tileIndex == 0);
  CHECK(compiledPrimary->assignments[7].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[8].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[8].vFlip);
  CHECK(compiledPrimary->assignments[8].tileIndex == 0);
  CHECK(compiledPrimary->assignments[8].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[9].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[9].vFlip);
  CHECK(compiledPrimary->assignments[9].tileIndex == 4);
  CHECK(compiledPrimary->assignments[9].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[10].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[10].vFlip);
  CHECK(compiledPrimary->assignments[10].tileIndex == 0);
  CHECK(compiledPrimary->assignments[10].paletteIndex == 0);

  CHECK_FALSE(compiledPrimary->assignments[11].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[11].vFlip);
  CHECK(compiledPrimary->assignments[11].tileIndex == 0);
  CHECK(compiledPrimary->assignments[11].paletteIndex == 0);

  for (std::size_t index = ctx.fieldmapConfig.numTilesPerMetatile;
       index < porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile; index++) {
    CHECK_FALSE(compiledPrimary->assignments[index].hFlip);
    CHECK_FALSE(compiledPrimary->assignments[index].vFlip);
    CHECK(compiledPrimary->assignments[index].tileIndex == 0);
    CHECK(compiledPrimary->assignments[index].paletteIndex == 0);
  }

  // Check that colorIndexMap is correct
  CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_RED)] == 0);
  CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_YELLOW)] == 1);
  CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_GREEN)] == 2);
  CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_BLUE)] == 3);
  CHECK(compiledPrimary->colorIndexMap[porytiles::rgbaToBgr(porytiles::RGBA_WHITE)] == 4);

  // Check that tileIndexes is correct
  CHECK(compiledPrimary->tileIndexes.size() == compiledPrimary->tiles.size());
  CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[0]] == 0);
  CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[1]] == 1);
  CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[2]] == 2);
  CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[3]] == 3);
  CHECK(compiledPrimary->tileIndexes[compiledPrimary->tiles[4]] == 4);
}

TEST_CASE("compile function should fill out secondary CompiledTileset struct with expected values")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/primary/top.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_3/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_3/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_3/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomPrimary, middlePrimary, topPrimary);

  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary);

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/secondary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/secondary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/secondary/top.png"));
  png::image<png::rgba_pixel> bottomSecondary{"res/tests/simple_metatiles_3/secondary/bottom.png"};
  png::image<png::rgba_pixel> middleSecondary{"res/tests/simple_metatiles_3/secondary/middle.png"};
  png::image<png::rgba_pixel> topSecondary{"res/tests/simple_metatiles_3/secondary/top.png"};
  porytiles::DecompiledTileset decompiledSecondary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomSecondary, middleSecondary, topSecondary);
  ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;
  ctx.compilerContext.pairedPrimaryTiles = std::move(compiledPrimary);
  auto compiledSecondary = porytiles::compile(ctx, decompiledSecondary);

  // Check that tiles are as expected
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_3/secondary/expected_tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_3/secondary/expected_tiles.png"};
  for (std::size_t tileIndex = 0; tileIndex < compiledSecondary->tiles.size(); tileIndex++) {
    for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
        CHECK(compiledSecondary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
              expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]);
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

  // Check that all assignments are correct
  CHECK(compiledSecondary->assignments.size() == porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

  CHECK_FALSE(compiledSecondary->assignments[0].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[0].vFlip);
  CHECK(compiledSecondary->assignments[0].tileIndex == 0);
  CHECK(compiledSecondary->assignments[0].paletteIndex == 0);

  CHECK_FALSE(compiledSecondary->assignments[1].hFlip);
  CHECK(compiledSecondary->assignments[1].vFlip);
  CHECK(compiledSecondary->assignments[1].tileIndex == 0 + ctx.fieldmapConfig.numTilesInPrimary);
  CHECK(compiledSecondary->assignments[1].paletteIndex == 2);

  CHECK_FALSE(compiledSecondary->assignments[2].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[2].vFlip);
  CHECK(compiledSecondary->assignments[2].tileIndex == 1 + ctx.fieldmapConfig.numTilesInPrimary);
  CHECK(compiledSecondary->assignments[2].paletteIndex == 3);

  CHECK_FALSE(compiledSecondary->assignments[3].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[3].vFlip);
  CHECK(compiledSecondary->assignments[3].tileIndex == 0);
  CHECK(compiledSecondary->assignments[3].paletteIndex == 0);

  CHECK_FALSE(compiledSecondary->assignments[4].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[4].vFlip);
  CHECK(compiledSecondary->assignments[4].tileIndex == 0);
  CHECK(compiledSecondary->assignments[4].paletteIndex == 0);

  CHECK_FALSE(compiledSecondary->assignments[5].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[5].vFlip);
  CHECK(compiledSecondary->assignments[5].tileIndex == 2 + ctx.fieldmapConfig.numTilesInPrimary);
  CHECK(compiledSecondary->assignments[5].paletteIndex == 3);

  CHECK_FALSE(compiledSecondary->assignments[6].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[6].vFlip);
  CHECK(compiledSecondary->assignments[6].tileIndex == 3 + ctx.fieldmapConfig.numTilesInPrimary);
  CHECK(compiledSecondary->assignments[6].paletteIndex == 3);

  CHECK_FALSE(compiledSecondary->assignments[7].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[7].vFlip);
  CHECK(compiledSecondary->assignments[7].tileIndex == 0);
  CHECK(compiledSecondary->assignments[7].paletteIndex == 0);

  CHECK_FALSE(compiledSecondary->assignments[8].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[8].vFlip);
  CHECK(compiledSecondary->assignments[8].tileIndex == 4 + ctx.fieldmapConfig.numTilesInPrimary);
  CHECK(compiledSecondary->assignments[8].paletteIndex == 3);

  CHECK_FALSE(compiledSecondary->assignments[9].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[9].vFlip);
  CHECK(compiledSecondary->assignments[9].tileIndex == 0);
  CHECK(compiledSecondary->assignments[9].paletteIndex == 0);

  CHECK_FALSE(compiledSecondary->assignments[10].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[10].vFlip);
  CHECK(compiledSecondary->assignments[10].tileIndex == 0);
  CHECK(compiledSecondary->assignments[10].paletteIndex == 0);

  CHECK(compiledSecondary->assignments[11].hFlip);
  CHECK(compiledSecondary->assignments[11].vFlip);
  CHECK(compiledSecondary->assignments[11].tileIndex == 5 + ctx.fieldmapConfig.numTilesInPrimary);
  CHECK(compiledSecondary->assignments[11].paletteIndex == 5);

  for (std::size_t index = ctx.fieldmapConfig.numTilesPerMetatile;
       index < porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile; index++) {
    CHECK_FALSE(compiledSecondary->assignments[index].hFlip);
    CHECK_FALSE(compiledSecondary->assignments[index].vFlip);
    CHECK(compiledSecondary->assignments[index].tileIndex == 0);
    CHECK(compiledSecondary->assignments[index].paletteIndex == 0);
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
  CHECK(compiledSecondary->tileIndexes.size() == compiledSecondary->tiles.size());
  CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[0]] == 0);
  CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[1]] == 1);
  CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[2]] == 2);
  CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[3]] == 3);
  CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[4]] == 4);
  CHECK(compiledSecondary->tileIndexes[compiledSecondary->tiles[5]] == 5);
}

TEST_CASE("compile function should correctly compile primary set with animated tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/top.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/anim_metatiles_1/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/anim_metatiles_1/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/anim_metatiles_1/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomPrimary, middlePrimary, topPrimary);

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/anims/flower_white"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/anims/water"));

  porytiles::AnimationPng<png::rgba_pixel> flowerWhite00{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/flower_white/00.png"}, "flower_white",
      "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> flowerWhite01{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/flower_white/01.png"}, "flower_white",
      "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> flowerWhite02{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/flower_white/02.png"}, "flower_white",
      "02.png"};
  porytiles::AnimationPng<png::rgba_pixel> water00{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/water/00.png"}, "water", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> water01{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/water/01.png"}, "water", "01.png"};

  std::vector<porytiles::AnimationPng<png::rgba_pixel>> flowerWhiteAnim{};
  std::vector<porytiles::AnimationPng<png::rgba_pixel>> waterAnim{};

  flowerWhiteAnim.push_back(flowerWhite00);
  flowerWhiteAnim.push_back(flowerWhite01);
  flowerWhiteAnim.push_back(flowerWhite02);

  waterAnim.push_back(water00);
  waterAnim.push_back(water01);

  std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
  anims.push_back(flowerWhiteAnim);
  anims.push_back(waterAnim);

  porytiles::importAnimTiles(ctx, anims, decompiledPrimary);

  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary);

  CHECK(compiledPrimary->tiles.size() == 10);

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/expected_tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/anim_metatiles_1/primary/expected_tiles.png"};
  for (std::size_t tileIndex = 0; tileIndex < compiledPrimary->tiles.size(); tileIndex++) {
    for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
        CHECK(compiledPrimary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
              expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]);
      }
    }
  }

  // Check that paletteIndexesOfTile is correct
  CHECK(compiledPrimary->paletteIndexesOfTile.size() == 10);
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

  // Check that all assignments are correct
  CHECK(compiledPrimary->assignments.size() == porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

  // Metatile 0 bottom
  CHECK_FALSE(compiledPrimary->assignments[0].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[0].vFlip);
  CHECK(compiledPrimary->assignments[0].tileIndex == 0);
  CHECK(compiledPrimary->assignments[0].paletteIndex == 0);
  CHECK_FALSE(compiledPrimary->assignments[1].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[1].vFlip);
  CHECK(compiledPrimary->assignments[1].tileIndex == 0);
  CHECK(compiledPrimary->assignments[1].paletteIndex == 0);
  CHECK_FALSE(compiledPrimary->assignments[2].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[2].vFlip);
  CHECK(compiledPrimary->assignments[2].tileIndex == 0);
  CHECK(compiledPrimary->assignments[2].paletteIndex == 0);
  CHECK_FALSE(compiledPrimary->assignments[3].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[3].vFlip);
  CHECK(compiledPrimary->assignments[3].tileIndex == 0);
  CHECK(compiledPrimary->assignments[3].paletteIndex == 0);
  // Metatile 0 middle
  CHECK(compiledPrimary->assignments[4].hFlip);
  CHECK(compiledPrimary->assignments[4].vFlip);
  CHECK(compiledPrimary->assignments[4].tileIndex == 6);
  CHECK(compiledPrimary->assignments[4].paletteIndex == 2);
  CHECK(compiledPrimary->assignments[5].hFlip);
  CHECK(compiledPrimary->assignments[5].vFlip);
  CHECK(compiledPrimary->assignments[5].tileIndex == 7);
  CHECK(compiledPrimary->assignments[5].paletteIndex == 2);
  CHECK_FALSE(compiledPrimary->assignments[6].hFlip);
  CHECK(compiledPrimary->assignments[6].vFlip);
  CHECK(compiledPrimary->assignments[6].tileIndex == 8);
  CHECK(compiledPrimary->assignments[6].paletteIndex == 2);
  CHECK(compiledPrimary->assignments[7].hFlip);
  CHECK(compiledPrimary->assignments[7].vFlip);
  CHECK(compiledPrimary->assignments[7].tileIndex == 9);
  CHECK(compiledPrimary->assignments[7].paletteIndex == 2);
  // Metatile 0 top
  CHECK_FALSE(compiledPrimary->assignments[8].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[8].vFlip);
  CHECK(compiledPrimary->assignments[8].tileIndex == 0);
  CHECK(compiledPrimary->assignments[8].paletteIndex == 0);
  CHECK_FALSE(compiledPrimary->assignments[9].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[9].vFlip);
  CHECK(compiledPrimary->assignments[9].tileIndex == 0);
  CHECK(compiledPrimary->assignments[9].paletteIndex == 0);
  CHECK_FALSE(compiledPrimary->assignments[10].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[10].vFlip);
  CHECK(compiledPrimary->assignments[10].tileIndex == 0);
  CHECK(compiledPrimary->assignments[10].paletteIndex == 0);
  CHECK_FALSE(compiledPrimary->assignments[11].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[11].vFlip);
  CHECK(compiledPrimary->assignments[11].tileIndex == 0);
  CHECK(compiledPrimary->assignments[11].paletteIndex == 0);

  // Metatile 1 bottom
  CHECK(compiledPrimary->assignments[12].hFlip);
  CHECK(compiledPrimary->assignments[12].vFlip);
  CHECK(compiledPrimary->assignments[12].tileIndex == 6);
  CHECK(compiledPrimary->assignments[12].paletteIndex == 2);
  CHECK(compiledPrimary->assignments[13].hFlip);
  CHECK(compiledPrimary->assignments[13].vFlip);
  CHECK(compiledPrimary->assignments[13].tileIndex == 7);
  CHECK(compiledPrimary->assignments[13].paletteIndex == 2);
  CHECK_FALSE(compiledPrimary->assignments[14].hFlip);
  CHECK(compiledPrimary->assignments[14].vFlip);
  CHECK(compiledPrimary->assignments[14].tileIndex == 8);
  CHECK(compiledPrimary->assignments[14].paletteIndex == 2);
  CHECK(compiledPrimary->assignments[15].hFlip);
  CHECK(compiledPrimary->assignments[15].vFlip);
  CHECK(compiledPrimary->assignments[15].tileIndex == 9);
  CHECK(compiledPrimary->assignments[15].paletteIndex == 2);
  // Metatile 1 middle
  CHECK_FALSE(compiledPrimary->assignments[16].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[16].vFlip);
  CHECK(compiledPrimary->assignments[16].tileIndex == 1);
  CHECK(compiledPrimary->assignments[16].paletteIndex == 2);
  CHECK_FALSE(compiledPrimary->assignments[17].hFlip);
  CHECK_FALSE(compiledPrimary->assignments[17].vFlip);
  CHECK(compiledPrimary->assignments[17].tileIndex == 2);
  CHECK(compiledPrimary->assignments[17].paletteIndex == 2);
  CHECK_FALSE(compiledPrimary->assignments[18].hFlip);
  CHECK(compiledPrimary->assignments[18].vFlip);
  CHECK(compiledPrimary->assignments[18].tileIndex == 3);
  CHECK(compiledPrimary->assignments[18].paletteIndex == 2);
  CHECK(compiledPrimary->assignments[19].hFlip);
  CHECK(compiledPrimary->assignments[19].vFlip);
  CHECK(compiledPrimary->assignments[19].tileIndex == 4);
  CHECK(compiledPrimary->assignments[19].paletteIndex == 2);
  // Metatile 1 top is blank, don't bother testing

  // Metatile 2 bottom is blank, don't bother testing
  // Metatile 2 middle
  CHECK_FALSE(compiledPrimary->assignments[28].hFlip);
  CHECK(compiledPrimary->assignments[28].vFlip);
  CHECK(compiledPrimary->assignments[28].tileIndex == 5);
  CHECK(compiledPrimary->assignments[28].paletteIndex == 1);
  CHECK_FALSE(compiledPrimary->assignments[29].hFlip);
  CHECK(compiledPrimary->assignments[29].vFlip);
  CHECK(compiledPrimary->assignments[29].tileIndex == 5);
  CHECK(compiledPrimary->assignments[29].paletteIndex == 1);
  CHECK_FALSE(compiledPrimary->assignments[30].hFlip);
  CHECK(compiledPrimary->assignments[30].vFlip);
  CHECK(compiledPrimary->assignments[30].tileIndex == 5);
  CHECK(compiledPrimary->assignments[30].paletteIndex == 1);
  CHECK_FALSE(compiledPrimary->assignments[31].hFlip);
  CHECK(compiledPrimary->assignments[31].vFlip);
  CHECK(compiledPrimary->assignments[31].tileIndex == 5);
  CHECK(compiledPrimary->assignments[31].paletteIndex == 1);
  // Metatile 2 top is blank, don't bother testing

  // Verify integrity of anims structure
  CHECK(compiledPrimary->anims.size() == 2);

  CHECK(compiledPrimary->anims.at(0).frames.size() == 3);
  CHECK(compiledPrimary->anims.at(0).frames.at(0).tiles.size() == 4);
  CHECK(compiledPrimary->anims.at(0).frames.at(1).tiles.size() == 4);
  CHECK(compiledPrimary->anims.at(0).frames.at(2).tiles.size() == 4);

  CHECK(compiledPrimary->anims.at(1).frames.size() == 2);
  CHECK(compiledPrimary->anims.at(1).frames.at(0).tiles.size() == 1);
  CHECK(compiledPrimary->anims.at(1).frames.at(1).tiles.size() == 1);
}

TEST_CASE("compile function should correctly compile secondary set with animated tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/top.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/anim_metatiles_1/primary/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/anim_metatiles_1/primary/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/anim_metatiles_1/primary/top.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomPrimary, middlePrimary, topPrimary);

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/anims/flower_white"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/primary/anims/water"));

  porytiles::AnimationPng<png::rgba_pixel> flowerWhite00{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/flower_white/00.png"}, "flower_white",
      "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> flowerWhite01{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/flower_white/01.png"}, "flower_white",
      "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> flowerWhite02{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/flower_white/02.png"}, "flower_white",
      "02.png"};
  porytiles::AnimationPng<png::rgba_pixel> water00{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/water/00.png"}, "water", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> water01{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/primary/anims/water/01.png"}, "water", "01.png"};

  std::vector<porytiles::AnimationPng<png::rgba_pixel>> flowerWhiteAnim{};
  std::vector<porytiles::AnimationPng<png::rgba_pixel>> waterAnim{};

  flowerWhiteAnim.push_back(flowerWhite00);
  flowerWhiteAnim.push_back(flowerWhite01);
  flowerWhiteAnim.push_back(flowerWhite02);

  waterAnim.push_back(water00);
  waterAnim.push_back(water01);

  std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
  anims.push_back(flowerWhiteAnim);
  anims.push_back(waterAnim);

  porytiles::importAnimTiles(ctx, anims, decompiledPrimary);

  auto compiledPrimary = porytiles::compile(ctx, decompiledPrimary);
  ctx.compilerContext.pairedPrimaryTiles = std::move(compiledPrimary);
  ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/secondary/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/secondary/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/secondary/top.png"));
  png::image<png::rgba_pixel> bottomSecondary{"res/tests/anim_metatiles_1/secondary/bottom.png"};
  png::image<png::rgba_pixel> middleSecondary{"res/tests/anim_metatiles_1/secondary/middle.png"};
  png::image<png::rgba_pixel> topSecondary{"res/tests/anim_metatiles_1/secondary/top.png"};
  porytiles::DecompiledTileset decompiledSecondary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomSecondary, middleSecondary, topSecondary);

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/secondary/anims/flower_red"));

  porytiles::AnimationPng<png::rgba_pixel> flowerRed00{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/secondary/anims/flower_red/00.png"}, "flower_white",
      "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> flowerRed01{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/secondary/anims/flower_red/01.png"}, "flower_white",
      "01.png"};
  porytiles::AnimationPng<png::rgba_pixel> flowerRed02{
      png::image<png::rgba_pixel>{"res/tests/anim_metatiles_1/secondary/anims/flower_red/02.png"}, "flower_white",
      "02.png"};

  std::vector<porytiles::AnimationPng<png::rgba_pixel>> flowerRedAnim{};

  flowerRedAnim.push_back(flowerRed00);
  flowerRedAnim.push_back(flowerRed01);
  flowerRedAnim.push_back(flowerRed02);

  std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> animsSecondary{};
  animsSecondary.push_back(flowerRedAnim);

  porytiles::importAnimTiles(ctx, animsSecondary, decompiledSecondary);

  auto compiledSecondary = porytiles::compile(ctx, decompiledSecondary);

  CHECK(compiledSecondary->tiles.size() == 8);

  REQUIRE(std::filesystem::exists("res/tests/anim_metatiles_1/secondary/expected_tiles.png"));
  png::image<png::index_pixel> expectedPng{"res/tests/anim_metatiles_1/secondary/expected_tiles.png"};
  for (std::size_t tileIndex = 0; tileIndex < compiledSecondary->tiles.size(); tileIndex++) {
    for (std::size_t row = 0; row < porytiles::TILE_SIDE_LENGTH; row++) {
      for (std::size_t col = 0; col < porytiles::TILE_SIDE_LENGTH; col++) {
        CHECK(compiledSecondary->tiles[tileIndex].colorIndexes[col + (row * porytiles::TILE_SIDE_LENGTH)] ==
              expectedPng[row][col + (tileIndex * porytiles::TILE_SIDE_LENGTH)]);
      }
    }
  }

  // Check that paletteIndexesOfTile is correct
  CHECK(compiledSecondary->paletteIndexesOfTile.size() == 8);
  CHECK(compiledSecondary->paletteIndexesOfTile[0] == 5);
  CHECK(compiledSecondary->paletteIndexesOfTile[1] == 5);
  CHECK(compiledSecondary->paletteIndexesOfTile[2] == 5);
  CHECK(compiledSecondary->paletteIndexesOfTile[3] == 5);
  CHECK(compiledSecondary->paletteIndexesOfTile[4] == 3);
  CHECK(compiledSecondary->paletteIndexesOfTile[5] == 3);
  CHECK(compiledSecondary->paletteIndexesOfTile[6] == 3);
  CHECK(compiledSecondary->paletteIndexesOfTile[7] == 3);

  // Check that all assignments are correct
  CHECK(compiledSecondary->assignments.size() == porytiles::METATILES_IN_ROW * ctx.fieldmapConfig.numTilesPerMetatile);

  // Metatile 0 bottom
  CHECK_FALSE(compiledSecondary->assignments[0].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[0].vFlip);
  CHECK(compiledSecondary->assignments[0].tileIndex == 0);
  CHECK(compiledSecondary->assignments[0].paletteIndex == 0);
  CHECK_FALSE(compiledSecondary->assignments[1].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[1].vFlip);
  CHECK(compiledSecondary->assignments[1].tileIndex == 0);
  CHECK(compiledSecondary->assignments[1].paletteIndex == 0);
  CHECK_FALSE(compiledSecondary->assignments[2].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[2].vFlip);
  CHECK(compiledSecondary->assignments[2].tileIndex == 0);
  CHECK(compiledSecondary->assignments[2].paletteIndex == 0);
  CHECK_FALSE(compiledSecondary->assignments[3].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[3].vFlip);
  CHECK(compiledSecondary->assignments[3].tileIndex == 0);
  CHECK(compiledSecondary->assignments[3].paletteIndex == 0);
  // Metatile 0 middle
  CHECK_FALSE(compiledSecondary->assignments[4].hFlip);
  CHECK(compiledSecondary->assignments[4].vFlip);
  CHECK(compiledSecondary->assignments[4].tileIndex == 5);
  CHECK(compiledSecondary->assignments[4].paletteIndex == 1);
  CHECK_FALSE(compiledSecondary->assignments[5].hFlip);
  CHECK(compiledSecondary->assignments[5].vFlip);
  CHECK(compiledSecondary->assignments[5].tileIndex == 5);
  CHECK(compiledSecondary->assignments[5].paletteIndex == 1);
  CHECK_FALSE(compiledSecondary->assignments[6].hFlip);
  CHECK(compiledSecondary->assignments[6].vFlip);
  CHECK(compiledSecondary->assignments[6].tileIndex == 5);
  CHECK(compiledSecondary->assignments[6].paletteIndex == 1);
  CHECK_FALSE(compiledSecondary->assignments[7].hFlip);
  CHECK(compiledSecondary->assignments[7].vFlip);
  CHECK(compiledSecondary->assignments[7].tileIndex == 5);
  CHECK(compiledSecondary->assignments[7].paletteIndex == 1);
  // Metatile 0 top
  CHECK_FALSE(compiledSecondary->assignments[8].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[8].vFlip);
  CHECK(compiledSecondary->assignments[8].tileIndex == 0);
  CHECK(compiledSecondary->assignments[8].paletteIndex == 0);
  CHECK_FALSE(compiledSecondary->assignments[9].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[9].vFlip);
  CHECK(compiledSecondary->assignments[9].tileIndex == 0);
  CHECK(compiledSecondary->assignments[9].paletteIndex == 0);
  CHECK_FALSE(compiledSecondary->assignments[10].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[10].vFlip);
  CHECK(compiledSecondary->assignments[10].tileIndex == 0);
  CHECK(compiledSecondary->assignments[10].paletteIndex == 0);
  CHECK_FALSE(compiledSecondary->assignments[11].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[11].vFlip);
  CHECK(compiledSecondary->assignments[11].tileIndex == 0);
  CHECK(compiledSecondary->assignments[11].paletteIndex == 0);

  // Metatile 1 bottom
  CHECK(compiledSecondary->assignments[12].hFlip);
  CHECK(compiledSecondary->assignments[12].vFlip);
  CHECK(compiledSecondary->assignments[12].tileIndex == 6);
  CHECK(compiledSecondary->assignments[12].paletteIndex == 2);
  CHECK(compiledSecondary->assignments[13].hFlip);
  CHECK(compiledSecondary->assignments[13].vFlip);
  CHECK(compiledSecondary->assignments[13].tileIndex == 7);
  CHECK(compiledSecondary->assignments[13].paletteIndex == 2);
  CHECK_FALSE(compiledSecondary->assignments[14].hFlip);
  CHECK(compiledSecondary->assignments[14].vFlip);
  CHECK(compiledSecondary->assignments[14].tileIndex == 8);
  CHECK(compiledSecondary->assignments[14].paletteIndex == 2);
  CHECK(compiledSecondary->assignments[15].hFlip);
  CHECK(compiledSecondary->assignments[15].vFlip);
  CHECK(compiledSecondary->assignments[15].tileIndex == 9);
  CHECK(compiledSecondary->assignments[15].paletteIndex == 2);
  // Metatile 1 middle
  CHECK_FALSE(compiledSecondary->assignments[16].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[16].vFlip);
  CHECK(compiledSecondary->assignments[16].tileIndex == 512);
  CHECK(compiledSecondary->assignments[16].paletteIndex == 5);
  CHECK(compiledSecondary->assignments[17].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[17].vFlip);
  CHECK(compiledSecondary->assignments[17].tileIndex == 513);
  CHECK(compiledSecondary->assignments[17].paletteIndex == 5);
  CHECK_FALSE(compiledSecondary->assignments[18].hFlip);
  CHECK(compiledSecondary->assignments[18].vFlip);
  CHECK(compiledSecondary->assignments[18].tileIndex == 514);
  CHECK(compiledSecondary->assignments[18].paletteIndex == 5);
  CHECK(compiledSecondary->assignments[19].hFlip);
  CHECK(compiledSecondary->assignments[19].vFlip);
  CHECK(compiledSecondary->assignments[19].tileIndex == 515);
  CHECK(compiledSecondary->assignments[19].paletteIndex == 5);
  // Metatile 1 top is blank, don't bother testing

  // Metatile 2 bottom is blank, don't bother testing
  // Metatile 2 middle
  CHECK_FALSE(compiledSecondary->assignments[28].hFlip);
  CHECK(compiledSecondary->assignments[28].vFlip);
  CHECK(compiledSecondary->assignments[28].tileIndex == 516);
  CHECK(compiledSecondary->assignments[28].paletteIndex == 3);
  CHECK_FALSE(compiledSecondary->assignments[29].hFlip);
  CHECK(compiledSecondary->assignments[29].vFlip);
  CHECK(compiledSecondary->assignments[29].tileIndex == 517);
  CHECK(compiledSecondary->assignments[29].paletteIndex == 3);
  CHECK_FALSE(compiledSecondary->assignments[30].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[30].vFlip);
  CHECK(compiledSecondary->assignments[30].tileIndex == 518);
  CHECK(compiledSecondary->assignments[30].paletteIndex == 3);
  CHECK_FALSE(compiledSecondary->assignments[31].hFlip);
  CHECK_FALSE(compiledSecondary->assignments[31].vFlip);
  CHECK(compiledSecondary->assignments[31].tileIndex == 519);
  CHECK(compiledSecondary->assignments[31].paletteIndex == 3);
  // Metatile 2 top is blank, don't bother testing

  // Verify integrity of anims structure
  CHECK(compiledSecondary->anims.size() == 1);

  CHECK(compiledSecondary->anims.at(0).frames.size() == 3);
  CHECK(compiledSecondary->anims.at(0).frames.at(0).tiles.size() == 4);
  CHECK(compiledSecondary->anims.at(0).frames.at(1).tiles.size() == 4);
  CHECK(compiledSecondary->anims.at(0).frames.at(2).tiles.size() == 4);
}
