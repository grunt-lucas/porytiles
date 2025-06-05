#pragma once

#include <array>

#include "../tiles/rgba_tile.hpp"

namespace porytiles {

class RgbaMetatile {
    std::array<RgbaTile, 4> bottom;
    std::array<RgbaTile, 4> middle;
    std::array<RgbaTile, 4> top;

  public:
    RgbaMetatile() = default;
};

} // namespace porytiles
