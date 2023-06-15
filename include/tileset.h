#ifndef PORYTILES_TILESET_H
#define PORYTILES_TILESET_H

#include "tile.h"
#include "palette.h"
#include "rgb_tiled_png.h"

#include <png.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace porytiles {
// Tileset images will be 16 tiles wide to match vanilla
constexpr size_t FINAL_IMAGE_WIDTH_IN_TILES = 16;
constexpr int NUM_BG_PALS = 12;

class Tileset {
    std::vector<IndexedTile> tiles;
    // std::pair is finalTilesetIndex,paletteIndex
    std::unordered_map<IndexedTile, std::pair<size_t, size_t>> tilesData;
    //std::unordered_set<IndexedTile> tilesIndex;
    std::vector<Palette> palettes;
    size_t maxPalettes;

public:
    explicit Tileset(size_t maxPalettes);

    [[nodiscard]] size_t getMaxPalettes() const { return maxPalettes; }

    void validateControlTileLayout(const RgbTiledPng& masterTiles) const;

    void alignSiblings(const RgbTiledPng& masterTiles);

    void buildPalettes(const RgbTiledPng& masterTiles);

    void indexTiles(const RgbTiledPng& masterTiles);

    void writeTileset();
};
} // namespace porytiles

#endif // PORYTILES_TILESET_H