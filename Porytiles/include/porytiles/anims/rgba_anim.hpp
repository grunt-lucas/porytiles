#pragma once

#include <vector>

#include "../tiles/rgba_tile.hpp"

namespace porytiles {

class RgbaAnim {
    std::vector<RgbaTile> key_frame_;
    std::vector<std::vector<RgbaTile>> frames_;

  public:
    RgbaAnim() = default;
};

} // namespace porytiles
