#pragma once

#include <vector>

#include "porytiles2/domain/valueobj/RgbaTile.hpp"

namespace porytiles {

class RgbaAnim {
  std::vector<RgbaTile> key_frame_;
  std::vector<std::vector<RgbaTile>> frames_;
  std::string name_;

public:
  RgbaAnim() = default;
};

} // namespace porytiles
