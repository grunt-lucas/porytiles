#include "tileset.h"

#include "cli_parser.h"
#include "tsoutput.h"
#include "tile.h"
#include "palette.h"

namespace tscreate {
namespace {
std::unordered_set<RgbColor> pixelsNotInPalette(const RgbTile& tile, const Palette& palette) {
    std::unordered_set<RgbColor> uniquePixels = tile.uniquePixels();
    std::unordered_set<RgbColor> paletteIndex = palette.getIndex();
    std::unordered_set<RgbColor> pixelsNotInPalette;
    std::copy_if(uniquePixels.begin(), uniquePixels.end(),
                 std::inserter(pixelsNotInPalette, pixelsNotInPalette.begin()),
                 [&paletteIndex](const RgbColor& needle) { return paletteIndex.find(needle) == paletteIndex.end(); });
    return pixelsNotInPalette;
}

int paletteWithFewestColors(const std::vector<Palette>& palettes) {
    int indexOfMin = 0;
    // Palettes can only have 16 colors, so 1000 will work fine.
    size_t minColors = 1000;
    for (int i = 0; i < palettes.size(); i++) {
        auto remainingColors = palettes.at(i).remainingColors();
        if (remainingColors < minColors) {
            indexOfMin = i;
            minColors = remainingColors;
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

void processTile(const RgbTiledPng& masterTiles, int tileIndex, const std::vector<Palette>& palettes) {
    const RgbTile& tile = masterTiles.tileAt(tileIndex);

    /*
     * If this is a primer-marker tile, skip it.
     */
    if (tile.isUniformly(gOptPrimerColor)) {
        verboseLog("skipping primer marker " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * Add each RgbColor in the tile to an unordered_set, `uniqueTileColors'
     */
    std::unordered_set<RgbColor> uniqueTileColors = tile.uniquePixels();
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
        unseenTileColors.push_back(pixelsNotInPalette(tile, palette));
        logString = masterTiles.tileDebugString(tileIndex) + ": " +
                    std::to_string(unseenTileColors[j].size()) + " color(s) not in palette " +
                    std::to_string(j) + ": [";
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
        logString = masterTiles.tileDebugString(tileIndex) + ": all colors are unseen";
        verboseLog(logString);
        logString.clear();
        int palWithFewestColors = paletteWithFewestColors(palettes);
        palettes[palWithFewestColors];
        // add colors to palWithFewest
        return;
    }

    /*
     * If `size(unseenTileColors) > 0`, `uniqueTileColors` shares some, but not all, colors with this palette. Save off
     * how many. Do this for each palette, we'll want to use the palette that is the closest match. If the closest match
     * palette doesn't have room for the new colors, try the next closest match. If no palette has room, fail the whole
     * program
     */
}
} // namespace (anonymous)

Tileset::Tileset(const int maxPalettes) : maxPalettes{maxPalettes} {
    palettes.reserve(this->maxPalettes);
    for (int i = 0; i < getMaxPalettes(); i++) {
        palettes.emplace_back(gOptTransparentColor);
    }
}

void Tileset::buildPalettes(const RgbTiledPng& masterTiles) {
    /*
     * TODO : before the main tile scan, we can do a sibling tile scan to pre-build palettes with matching indices that
     * support sibling tiles
     */

    for (int i = 0; i < masterTiles.size(); i++) {
        processTile(masterTiles, i, palettes);
    }
}
} // namespace tscreate
