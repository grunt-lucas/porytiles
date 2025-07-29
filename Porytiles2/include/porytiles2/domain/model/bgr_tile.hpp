#pragma once

#include "porytiles2/domain/model/bgr15.hpp"
#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

class BgrTile final : public Tile<Bgr15> {
  public:
    BgrTile() = default;
};

} // namespace porytiles2
