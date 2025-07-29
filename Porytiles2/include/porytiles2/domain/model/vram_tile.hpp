#pragma once

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

class VramTile final : public Tile<IndexPixel> {
  public:
    VramTile() = default;
};

} // namespace porytiles2
