#pragma once

#include "../colors/rgba32.hpp"
#include "./tile.hpp"
#include "./tile_metadata.hpp"

namespace porytiles {

class RgbaTile : public Tile<Rgba32> {
  public:
    explicit RgbaTile(const TileType t) : Tile{t} {}
};

} // namespace porytiles