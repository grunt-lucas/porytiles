#pragma once

#include <vector>

#include "../colors/bgr15.hpp"

namespace porytiles {

class BgrPal final {
    std::vector<Bgr15> colors_;

  public:
    BgrPal() = default;
};

} // namespace porytiles
