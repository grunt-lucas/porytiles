#pragma once

#include "porytiles2/domain/model/color_set.hpp"
#include "porytiles2/domain/model/tile/shape_tile.hpp"

namespace porytiles2 {

class ColorIndexShapeTile final : public ShapeTile<ColorIndex> {
  public:
    ColorIndexShapeTile() = default;

    explicit ColorIndexShapeTile(const ShapeTile &base_tile)
    {
        for (const auto &[mask, color_index] : base_tile.colors()) {
            set(mask, color_index);
        }
    }
};

} // namespace porytiles2
