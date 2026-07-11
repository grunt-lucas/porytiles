#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>
#include <cstddef>
#include <string>

#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

inline constexpr std::size_t colors_per_pal = 16;

/// @brief Represents a palette index value within a particular palette.
///
/// @details
/// PaletteIndex is a semantic wrapper around an unsigned integer that represents an index into a palette (or
/// palette-like configuration file).
///
/// @invariant The underlying index value will always be between 0 and 15.
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

    // NOLINTNEXTLINE
    operator const std::size_t &() const &
    {
        return value_;
    }

    // NOLINTNEXTLINE
    operator std::size_t &&() &&
    {
        return std::move(value_);
    }

    [[nodiscard]] const std::size_t &value() const &
    {
        return value_;
    }

    [[nodiscard]] std::size_t &&value() &&
    {
        return std::move(value_);
    }

    [[nodiscard]] std::size_t index() const
    {
        return value_;
    }

  private:
    std::size_t value_{};
};

} // namespace porytiles
