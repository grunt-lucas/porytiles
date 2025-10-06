#pragma once

#include <cstddef>

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

inline constexpr std::size_t colors_per_pal = 16;
inline constexpr std::size_t num_pals = 16;
inline constexpr std::size_t num_colors = colors_per_pal * num_pals;

class ColorIndex {
  public:
    ColorIndex() = default;

    // NOLINTNEXTLINE(google-explicit-constructor)
    ColorIndex(unsigned int index) : index_{index}
    {
        if (index_ >= num_colors) {
            panic("invalid ColorIndex value: " + std::to_string(index_));
        }
    }

    auto operator<=>(const ColorIndex &) const = default;

    [[nodiscard]] unsigned int index() const
    {
        return index_;
    }

  private:
    unsigned int index_;
};

} // namespace porytiles2
