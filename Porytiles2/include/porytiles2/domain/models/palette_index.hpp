#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>
#include <cstddef>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

inline constexpr std::size_t colors_per_pal = 16;

/**
 * @brief Represents a palette index value within a particular palette.
 *
 * @details
 * PaletteIndex is a semantic wrapper around an unsigned integer that represents an index into a palette (or
 * palette-like configuration file).
 *
 * @invariant The underlying index value will always be between 0 and 15.
 */
class PaletteIndex {
  public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    PaletteIndex(std::size_t value) : value_{value}
    {
        if (value >= colors_per_pal) {
            panic("invalid PaletteIndex value " + std::to_string(value));
        }
    }

    auto operator<=>(const PaletteIndex &) const = default;

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
        return value_;
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
        return std::move(value_);
    }

    /**
     * @brief Gets a const reference to the underlying value.
     *
     * @return A const reference to the stored value
     */
    [[nodiscard]] const std::size_t &value() const &
    {
        return value_;
    }

    /**
     * @brief Gets an rvalue reference to the underlying value.
     *
     * @return An rvalue reference to the stored value
     */
    [[nodiscard]] std::size_t &&value() &&
    {
        return std::move(value_);
    }

    /**
     * @brief Returns the underlying index value.
     *
     * @return The color index value
     */
    [[nodiscard]] std::size_t index() const
    {
        return value_;
    }

  private:
    std::size_t value_{};
};

} // namespace porytiles2
