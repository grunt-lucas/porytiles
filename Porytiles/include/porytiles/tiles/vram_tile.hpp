#pragma once

#include "./tile.hpp"
#include "./tile_metadata.hpp"

namespace porytiles {

class VramTile : public Tile<std::uint8_t> {
  public:
    VramTile() : Tile{TileType::kVram} {}

    [[nodiscard]] bool IsTransparent() const {
        return Tile::IsTransparent(0);
    }
};

} // namespace porytiles
