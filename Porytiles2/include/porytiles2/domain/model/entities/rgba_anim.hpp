#pragma once

#include <vector>

#include "porytiles2/domain/model/valueobj/rgba_tile.hpp"

namespace porytiles2 {

class RgbaAnim {
  public:
    RgbaAnim() = default;

  private:
    std::vector<RgbaTile> key_frame_;
    std::vector<std::vector<RgbaTile>> frames_;
    std::string name_;
};

} // namespace porytiles2
