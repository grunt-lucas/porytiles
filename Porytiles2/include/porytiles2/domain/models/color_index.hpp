#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>
#include <cstddef>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Represents a color index value for palette operations.
 *
 * @details
 * ColorIndex is a semantic wrapper around an integer that represents a unique identifier for a unique color in the
 * global ColorIndexMap construction process. This type provides type safety and semantic clarity when working with
 * color indices, distinguishing them from other integer values in the codebase.
 *
 * Unlike IndexPixel, ColorIndex does not follow the SupportsTransparency concept, as it is used to represent the
 * sequential indices assigned to non-transparent colors in the ColorIndexMap. A ColorSet is a collection of
 * \link ColorIndex ColorIndexes\endlink, which can be redeemed for their original colors by consulting the
 * ColorIndexMap.
 */
class ColorIndex {
  public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    ColorIndex(std::size_t index) : index_{index} {}

    auto operator<=>(const ColorIndex &) const = default;

    /**
     * @brief Implicit conversion to const reference of the underlying value.
     *
     * @details
     * Allows ColorIndex to be used transparently where the underlying type is expected.
     *
     * @return A const reference to the stored value
     */
    // NOLINTNEXTLINE
    operator const std::size_t &() const &
    {
        return index_;
    }

    /**
     * @brief Implicit conversion to rvalue reference of the underlying value.
     *
     * @details
     * Enables move semantics when the ColorIndex is an rvalue.
     *
     * @return An rvalue reference to the stored value
     */
    // NOLINTNEXTLINE
    operator std::size_t &&() &&
    {
        return std::move(index_);
    }

    /**
     * @brief Gets a const reference to the underlying value.
     *
     * @return A const reference to the stored value
     */
    [[nodiscard]] const std::size_t &value() const &
    {
        return index_;
    }

    /**
     * @brief Gets an rvalue reference to the underlying value.
     *
     * @return An rvalue reference to the stored value
     */
    [[nodiscard]] std::size_t &&value() &&
    {
        return std::move(index_);
    }

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
    std::size_t index_{};
};

} // namespace porytiles2
