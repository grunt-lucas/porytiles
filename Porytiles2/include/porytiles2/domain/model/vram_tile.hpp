#pragma once

#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

class VramTile final : public Tile<std::uint8_t> {
  public:
    VramTile() = default;

    [[nodiscard]] bool is_transparent() const {
        return Tile::is_transparent(0);
    }
};

} // namespace porytiles2
