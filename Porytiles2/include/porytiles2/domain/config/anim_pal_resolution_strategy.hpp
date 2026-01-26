#pragma once

#include <optional>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief TODO: fill in
 */
enum class AnimPalResolutionStrategy {
    /**
     * @brief Use palette 00.pal.
     */
    default_pal,

    /**
     * @brief Use the Porymap-component frame PNG internal palette, if present.
     */
    internal_png_palette
};

[[nodiscard]] inline std::optional<AnimPalResolutionStrategy>
anim_pal_resolution_strategy_from_str(const std::string &str)
{
    if (str == "default_pal") {
        return std::optional{AnimPalResolutionStrategy::default_pal};
    }
    if (str == "internal_png_palette") {
        return std::optional{AnimPalResolutionStrategy::internal_png_palette};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::string to_string(const AnimPalResolutionStrategy s)
{
    switch (s) {
    case AnimPalResolutionStrategy::default_pal:
        return "default_pal";
    case AnimPalResolutionStrategy::internal_png_palette:
        return "internal_png_palette";
    }
    panic("unhandled AnimPalResolutionStrategy value");
}

inline std::ostream &operator<<(std::ostream &os, const AnimPalResolutionStrategy s)
{
    return os << to_string(s);
}

} // namespace porytiles2
