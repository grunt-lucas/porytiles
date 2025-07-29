#pragma once

#include "porytiles2/domain/model/bgr15.hpp"
#include "porytiles2/domain/model/metatile.hpp"

namespace porytiles2 {

class BgrMetatile : public Metatile<Bgr15> {
  public:
    BgrMetatile() = default;
};

} // namespace porytiles2
