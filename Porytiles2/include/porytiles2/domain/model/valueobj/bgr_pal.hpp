#pragma once

#include <vector>

#include "porytiles2/domain/model/valueobj/bgr15.hpp"

namespace porytiles2 {

class BgrPal final {
    std::vector<Bgr15> colors_;

  public:
    BgrPal() = default;
};

} // namespace porytiles2
