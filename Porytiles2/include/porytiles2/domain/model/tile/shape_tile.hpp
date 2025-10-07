#pragma once

#include <algorithm>
#include <map>
#include <ranges>

#include "porytiles2/domain/model/tile/shape_mask.hpp"

namespace porytiles2 {

template <typename PixelType>
class ShapeTile {
  public:
    virtual ~ShapeTile() = default;

    ShapeTile() = default;

    bool operator==(const ShapeTile &other) const = default;

    // CRITICAL: Custom operator< that ONLY compares keys, not values
    // This is the key to canonical orientation finding - it compares ONLY the shape masks, not the colors
    bool operator<(const ShapeTile &other) const
    {
        auto keys1 = colors_ | std::views::keys;
        auto keys2 = other.colors_ | std::views::keys;
        return std::ranges::lexicographical_compare(keys1, keys2);
    }

    [[nodiscard]] bool is_transparent() const
    {
        auto keys = colors_ | std::views::keys;
        return std::ranges::all_of(keys, &ShapeMask::is_transparent);
    }

    [[nodiscard]] ShapeTile flip(bool h, bool v) const
    {
        if (!h && !v) {
            return *this;
        }

        ShapeTile result;
        for (const auto &[mask, color] : colors_) {
            result.colors_[mask.flip(h, v)] = color;
        }
        return result;
    }

    [[nodiscard]] const std::map<ShapeMask, PixelType> &colors() const
    {
        return colors_;
    }

    void set(const ShapeMask &mask, const PixelType &color)
    {
        colors_[mask] = color;
    }

  private:
    std::map<ShapeMask, PixelType> colors_;
};

} // namespace porytiles2