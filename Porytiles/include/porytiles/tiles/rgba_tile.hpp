#pragma once

#include "../colors/rgba32.hpp"
#include "./tile.hpp"
#include "./tile_metadata.hpp"

namespace porytiles {

class RgbaTile : public Tile<Rgba32> {
  public:
    explicit RgbaTile(const TileType t) : Tile{t} {}

    [[nodiscard]] bool IsTransparent(const Rgba32 &transparency_rgba) const;

    [[nodiscard]] bool EqualsBgr(const RgbaTile &other) const;
};

} // namespace porytiles