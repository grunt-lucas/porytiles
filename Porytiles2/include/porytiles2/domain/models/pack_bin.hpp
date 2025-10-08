#pragma once

#include "porytiles2/domain/model/color_set.hpp"

namespace porytiles2 {

class PackBin {
  public:
    PackBin(unsigned int pal_index, const ColorSet &color_set) : pal_index_{pal_index}, color_set_{color_set} {}

    [[nodiscard]] unsigned int pal_index() const
    {
        return pal_index_;
    }

    [[nodiscard]] const ColorSet &color_set() const
    {
        return color_set_;
    }

    [[nodiscard]] ColorSet &color_set()
    {
        return color_set_;
    }

  private:
    unsigned int pal_index_;
    ColorSet color_set_;
};

} // namespace porytiles2
