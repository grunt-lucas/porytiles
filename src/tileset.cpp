#include "tileset.h"

#include "cli_parser.h"
#include "tsoutput.h"

namespace tscreate {
const int NUM_BG_PALS = 16;

Tileset::Tileset(const int maxPalettes) : maxPalettes{maxPalettes} {
    palettes.reserve(this->maxPalettes);
    for (int i = 0; i < getMaxPalettes(); i++) {
        palettes.emplace_back(gOptTransparentColor);
    }
}

void Tileset::buildPalettes(const RgbTiledPng& masterTiles) {
    for (int i = 0; i < masterTiles.size(); i++) {
        const Tile<RgbColor>& tile = masterTiles.tileAt(i);

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
        verboseLog(masterTiles.tileDebugString(i) + ": " +
                   std::to_string(uniqueTileColors.size()) + " unique color(s)");
        for (const auto& color: uniqueTileColors) {
            verboseLog(color.prettyString());
        }
        verboseLog("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    }
}
} // namespace tscreate
