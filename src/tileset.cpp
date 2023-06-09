#include "tileset.h"

#include "cli_parser.h"
#include "tsoutput.h"
#include "tile.h"

namespace tscreate {
const int NUM_BG_PALS = 16;

Tileset::Tileset(const int maxPalettes) : maxPalettes{maxPalettes} {
    palettes.reserve(this->maxPalettes);
    for (int i = 0; i < getMaxPalettes(); i++) {
        palettes.emplace_back(gOptTransparentColor);
    }
}

static std::unordered_set<RgbColor> pixelsNotInPalette(const RgbTile& tile, const Palette& palette) {
    std::unordered_set<RgbColor> uniquePixels = tile.uniquePixels();
    std::unordered_set<RgbColor> paletteIndex = palette.getIndex();
    std::unordered_set<RgbColor> pixelsNotInPalette;
    std::copy_if(uniquePixels.begin(), uniquePixels.end(),
                 std::inserter(pixelsNotInPalette, pixelsNotInPalette.begin()),
                 [&paletteIndex](const RgbColor& needle) { return paletteIndex.find(needle) == paletteIndex.end(); });
    return pixelsNotInPalette;
}

void Tileset::buildPalettes(const RgbTiledPng& masterTiles) {
    for (int i = 0; i < masterTiles.size(); i++) {
        const RgbTile& tile = masterTiles.tileAt(i);

        /*
         * If this is a primer-marker tile, skip it.
         */
        if (tile.isUniformly(gOptPrimerColor)) {
            verboseLog("skipping primer marker " + masterTiles.tileDebugString(i));
            continue;
        }

        /*
         * Add each RgbColor in the tile to an unordered_set
         */
        std::unordered_set<RgbColor> uniqueTileColors = tile.uniquePixels();
        std::string logString = masterTiles.tileDebugString(i) + ": " +
                                std::to_string(uniqueTileColors.size()) + " unique color(s): [";
        for (const auto& color: uniqueTileColors) {
            logString += color.prettyString() + "; ";
        }
        logString += "]";
        verboseLog(logString);
        logString.clear();

        /*
         * Compute `diff = tileColorSet - paletteColorSet` for each palette, where `-` is set difference.
         */
        for (int j = 0; j < palettes.size(); j++) {
            const Palette& palette = palettes.at(j);
            std::unordered_set<RgbColor> colorsNotInPalette = pixelsNotInPalette(tile, palette);
            logString = masterTiles.tileDebugString(i) + ": " +
                        std::to_string(colorsNotInPalette.size()) + " color(s) not in palette " +
                        std::to_string(j) + ": [";
            for (const auto& color: colorsNotInPalette) {
                logString += color.prettyString() + "; ";
            }
            logString += "]";
            verboseLog(logString);
            logString.clear();
        }
    }
}
} // namespace tscreate
