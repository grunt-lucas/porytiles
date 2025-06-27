#pragma once

#include <vector>

#include "porytiles2/domain/valueobj/Bgr15.hpp"

namespace porytiles {

class BgrPal final {
    std::vector<Bgr15> colors_;

  public:
    BgrPal() = default;
};

} // namespace porytiles
