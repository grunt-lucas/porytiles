#pragma once

#include "porytiles2/domain/value_objects/BgrPal.hpp"
#include "porytiles2/domain/value_objects/Tile.hpp"

namespace porytiles {

class BgrInternalTile final : public Tile<std::uint8_t> {
    BgrPal pal_;

  public:
    BgrInternalTile() = default;
};

} // namespace porytiles
