#pragma once

#include "porytiles2/domain/model/metatile.hpp"
#include "porytiles2/domain/model/rgba_tile.hpp"

namespace porytiles2 {

class RgbaMetatile : public Metatile<Rgba32> {
  public:
    RgbaMetatile() = default;
};

} // namespace porytiles2
