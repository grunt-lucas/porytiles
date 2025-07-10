#pragma once

#include <array>

#include "porytiles2/domain/model/valueobj/RgbaTile.hpp"

namespace porytiles {

class RgbaMetatile {
  std::array<RgbaTile, 4> bottom_;
  std::array<RgbaTile, 4> middle_;
  std::array<RgbaTile, 4> top_;

public:
  RgbaMetatile() = default;
};

} // namespace porytiles
