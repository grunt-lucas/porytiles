#pragma once

#include "./bgr_pal.hpp"
#include "./tile.hpp"

namespace porytiles {

class BgrInternalTile final : public Tile<std::uint8_t> {
    BgrPal pal_;

  public:
    BgrInternalTile() = default;
};

} // namespace porytiles
