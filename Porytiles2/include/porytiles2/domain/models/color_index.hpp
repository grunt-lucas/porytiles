#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>
#include <cstddef>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

inline constexpr std::size_t colors_per_pal = 16;
inline constexpr std::size_t num_pals = 16;
inline constexpr std::size_t num_colors = colors_per_pal * num_pals;

/**
 * @brief Represents a color index value for palette operations.
 *
 * @details
 * ColorIndex is a semantic wrapper around an unsigned integer that represents a unique identifier for a unique color in
 * the global ColorIndexMap construction process. This type provides type safety and semantic clarity when working with
 * color indices, distinguishing them from other integer values in the codebase.
 *
 * Unlike IndexPixel, ColorIndex does not follow the SupportsTransparency concept, as it is used to represent the
 * sequential indices assigned to non-transparent colors in the ColorIndexMap. A ColorSet is a collection of
 * \link ColorIndex ColorIndexes, \endlink which can be redeemed for their original colors by consulting the
 * ColorIndexMap.
 *
 * @invariant ColorIndex value must be less than num_colors (256). Construction with an invalid index will panic.
 */
class ColorIndex {
  public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    ColorIndex(unsigned int index) : index_{index}
    {
        // TODO: do we want validation here?
        // I think not, since ideally we should allow ColorIndexMap to grow to arbitrary size.
        // The domain compiler service can then check the size and throw based on exogenous config params.
        // if (index_ >= num_colors) {
        //     panic("invalid ColorIndex value: " + std::to_string(index_));
        // }
    }

    auto operator<=>(const ColorIndex &) const = default;

    /**
     * @brief Returns the underlying unsigned integer index value.
     *
     * @return The color index value
     */
    [[nodiscard]] unsigned int index() const
    {
        return index_;
    }

  private:
    unsigned int index_{};
};

} // namespace porytiles2
