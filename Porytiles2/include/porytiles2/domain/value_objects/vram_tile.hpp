#pragma once

#include <porytiles2/domain/value_objects/tile.hpp>

namespace porytiles {

class VramTile final : public Tile<std::uint8_t> {
  public:
    VramTile() = default;

    [[nodiscard]] bool IsTransparent() const {
        return Tile::IsTransparent(0);
    }
};

} // namespace porytiles
