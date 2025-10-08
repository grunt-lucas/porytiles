#pragma once

#include "porytiles2/domain/models/color_set.hpp"

namespace porytiles2 {

class PackSet {
  public:
    static constexpr int unassigned = -1;

    PackSet(unsigned int tile_index, const ColorSet &color_set)
        : tile_index_{tile_index}, color_set_{color_set}, assigned_pal_index_{unassigned}
    {
    }

    [[nodiscard]] unsigned int tile_index() const
    {
        return tile_index_;
    }

    [[nodiscard]] const ColorSet &color_set() const
    {
        return color_set_;
    }

    [[nodiscard]] int assigned_pal_index() const
    {
        return assigned_pal_index_;
    }

    void assign_pal_index(int pal_index)
    {
        assigned_pal_index_ = pal_index;
    }

  private:
    unsigned int tile_index_;
    ColorSet color_set_;
    int assigned_pal_index_;
};

} // namespace porytiles2
