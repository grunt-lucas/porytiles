#include "tileset.h"

#include "cli_parser.h"
#include "tsoutput.h"
#include "tile.h"
#include "palette.h"
#include "tsexception.h"

#include <iostream>

namespace porytiles {
namespace {
int paletteWithFewestColors(const std::vector<Palette>& palettes) {
    int indexOfMin = 0;
    // Palettes can only have 15 colors, so 1000 will work fine as a starting value
    size_t minColors = 1000;
    for (int i = 0; i < palettes.size(); i++) {
        auto size = palettes.at(i).size();
        if (size < minColors) {
            indexOfMin = i;
            minColors = size;
        }
    }
    return indexOfMin;
}

bool areAllTileColorsUnseen(std::unordered_set<RgbColor> uniqueTileColors,
                            std::vector<std::unordered_set<RgbColor>> unseenTileColors) {
    return std::all_of(unseenTileColors.begin(), unseenTileColors.end(),
                       [&uniqueTileColors](std::unordered_set<RgbColor>& colors) {
                           return uniqueTileColors.size() == colors.size();
                       });
}

int getClosestPaletteWithRoom(const RgbTiledPng& masterTiles, int tileIndex, std::vector<Palette>& palettes,
                              std::vector<std::unordered_set<RgbColor>> unseenTileColors) {
    int indexOfClosestPalette = -1;
    // Palettes can only have 15 colors, so 1000 will work fine as a starting value
    size_t missingColorCount = 1000;
    for (int i = 0; i < unseenTileColors.size(); i++) {
        if (unseenTileColors[i].size() < missingColorCount &&
            unseenTileColors[i].size() <= palettes[i].remainingColors()) {
            /*
             * TODO : do we also want to condition this on palette size? Right now: if two (or more) palettes are
             * equally close, and each indeed has room, we just return the first palette that satisfies both conditions.
             * Do we also want to make sure we return the smaller of the two (or more)?
             */
            missingColorCount = unseenTileColors[i].size();
            indexOfClosestPalette = i;
        }
    }
    if (indexOfClosestPalette == -1) {
        // This happens when no palette has room
        throw TsException{masterTiles.tileDebugString(tileIndex) + ": not enough palettes to allocate this tile"};
    }
    return indexOfClosestPalette;
}

void assignTileToPalette(const RgbTiledPng& masterTiles, int tileIndex, std::vector<Palette>& palettes) {
    const RgbTile& tile = masterTiles.tileAt(tileIndex);

    /*
     * If this is a total transparent tile, skip it.
     */
    if (tile.isUniformly(gOptTransparentColor)) {
        verboseLog("skipping transparent " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * If this is a primer-marker tile, skip it.
     */
    if (tile.isUniformly(gOptPrimerColor)) {
        verboseLog("skipping primer marker " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * Add each RgbColor in the tile to an unordered_set, `uniqueTileColors`
     */
    std::unordered_set<RgbColor> uniqueTileColors = tile.uniquePixels(gOptTransparentColor);
    std::string logString = masterTiles.tileDebugString(tileIndex) + ": " +
                            std::to_string(uniqueTileColors.size()) + " unique color(s): [";
    for (const auto& color: uniqueTileColors) {
        logString += color.prettyString() + "; ";
    }
    logString += "]";
    verboseLog(logString);
    logString.clear();

    /*
     * Compute `unseenTileColors[i] = uniqueTileColors - palette[i]` for each palette, where `-` is set difference.
     */
    std::vector<std::unordered_set<RgbColor>> unseenTileColors;
    unseenTileColors.reserve(palettes.size());
    for (int j = 0; j < palettes.size(); j++) {
        const Palette& palette = palettes.at(j);
        unseenTileColors.push_back(tile.pixelsNotInPalette(palette));
        logString = masterTiles.tileDebugString(tileIndex) + ": " +
                    std::to_string(unseenTileColors[j].size()) + " color(s) not in palette " +
                    std::to_string(j) + "(s=" + std::to_string(palettes[j].size()) + "): [";
        for (const auto& color: unseenTileColors[j]) {
            logString += color.prettyString() + "; ";
        }
        logString += "]";
        verboseLog(logString);
        logString.clear();
    }

    /*
     * If `size(unseenTileColors[i]) == 0`, that means `palette[i]` fully covers `uniqueTileColors`, and so we can
     * return, we have found a palette that will work for this tile.
     */
    for (int j = 0; j < unseenTileColors.size(); j++) {
        const std::unordered_set<RgbColor>& diff = unseenTileColors[j];
        if (diff.empty()) {
            logString = masterTiles.tileDebugString(tileIndex) + ": matched palette " + std::to_string(j);
            verboseLog(logString);
            logString.clear();
            return;
        }
    }

    /*
     * If `size(unseenTileColors[i]) == size(uniqueTileColors)`, that means this tile doesn't share any colors with
     * `palette[i]`. If this is true for every palette, then we have a tile with only new colors. In this case, let's
     * add the new colors to the palette with the fewest number of colors so far.
     */
    bool tileColorsAllUnseen = areAllTileColorsUnseen(uniqueTileColors, unseenTileColors);
    if (tileColorsAllUnseen) {
        int palWithFewestColors = paletteWithFewestColors(palettes);
        if (uniqueTileColors.size() > palettes[palWithFewestColors].remainingColors()) {
            throw TsException{masterTiles.tileDebugString(tileIndex) + ": not enough palettes to allocate this tile"};
        }
        for (const auto& color: uniqueTileColors) {
            palettes[palWithFewestColors].addColorAtEnd(color);
        }
        logString = masterTiles.tileDebugString(tileIndex) + ": all colors were unseen, added to palette " +
                    std::to_string(palWithFewestColors) + ", " +
                    std::to_string(palettes[palWithFewestColors].remainingColors()) + " colors remaining";
        verboseLog(logString);
        logString.clear();
        return;
    }

    /*
     * If `size(unseenTileColors[i]) > 0`, `uniqueTileColors` shares some, but not all, colors with `palette[i]`. Save
     * off how many. Do this for each palette, we'll want to use the palette that is the closest match. If the closest
     * match palette doesn't have room for the new colors, try the next closest match. If no palette has room, fail the
     * whole program
     */
    int closestPalWithRoom = getClosestPaletteWithRoom(masterTiles, tileIndex, palettes, unseenTileColors);
    for (const auto& color: unseenTileColors[closestPalWithRoom]) {
        palettes[closestPalWithRoom].addColorAtEnd(color);
    }
    logString = masterTiles.tileDebugString(tileIndex) + ": contained some new colors, added to palette " +
                std::to_string(closestPalWithRoom) + ", " +
                std::to_string(palettes[closestPalWithRoom].remainingColors()) + " colors remaining";
    verboseLog(logString);
    logString.clear();
}

void
indexTile(const RgbTiledPng& masterTiles, int tileIndex, std::vector<Palette>& palettes,
          std::vector<IndexedTile>& tiles,
          std::unordered_set<IndexedTile>& tilesIndex) {
    const RgbTile& tile = masterTiles.tileAt(tileIndex);

    /*
     * If this is a total transparent tile, skip it.
     */
    if (tile.isUniformly(gOptTransparentColor)) {
        verboseLog("skipping transparent " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * If this is a sibling control tile, skip it.
     */
    if (tile.isUniformly(gOptSiblingColor)) {
        verboseLog("skipping sibling control " + masterTiles.tileDebugString(tileIndex));
        return;
    }
}
} // namespace (anonymous)

Tileset::Tileset(const int maxPalettes) : maxPalettes{maxPalettes} {
    palettes.reserve(this->maxPalettes);
    for (int i = 0; i < getMaxPalettes(); i++) {
        // prefill palette vector with empty palettes
        palettes.emplace_back();
    }
}

void Tileset::alignSiblings(const RgbTiledPng& masterTiles) {
    /*
     * TODO : before the main tile scan, we can do a sibling tile scan to pre-build palettes with matching indices that
     * support sibling tiles
     */
    verboseLog("--------------- ALIGNING SIBLINGS ---------------");
}

void Tileset::buildPalettes(const RgbTiledPng& masterTiles) {
    verboseLog("--------------- BUILDING PALETTES ---------------");
    for (int i = 0; i < masterTiles.size(); i++) {
        assignTileToPalette(masterTiles, i, palettes);
    }
}

void Tileset::indexTiles(const RgbTiledPng& masterTiles) {
    bool inPrimerBlock = false;
    std::string logString;

    verboseLog("--------------- INDEXING TILES ---------------");
    for (int i = 0; i < masterTiles.size(); i++) {
        if (masterTiles.tileAt(i).isUniformly(gOptPrimerColor)) {
            inPrimerBlock = !inPrimerBlock;
            logString = inPrimerBlock ? "entered" : "exited";
            logString += " primer block " + masterTiles.tileDebugString(i);
            verboseLog(logString);
            logString.clear();
            continue;
        }

        if (!inPrimerBlock)
            indexTile(masterTiles, i, palettes, tiles, tilesIndex);
    }
}
} // namespace porytiles
