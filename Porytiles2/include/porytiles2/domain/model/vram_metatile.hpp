#pragma once

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/metatile.hpp"

namespace porytiles2 {

class VramMetatile : public Metatile<IndexPixel> {
  public:
    VramMetatile() = default;

    // TODO: need a way to store palette index?
};

} // namespace porytiles2
