#pragma once

#include <porytiles2/domain/value_objects/bgr_pal.hpp>
#include <porytiles2/domain/value_objects/tile.hpp>

namespace porytiles {

class BgrInternalTile final : public Tile<std::uint8_t> {
    BgrPal pal_;

  public:
    BgrInternalTile() = default;
};

} // namespace porytiles
