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
     * @brief Disallow palette resolution fallback and error out.
     */
    error,

    /**
     * @brief Use palette 00.pal.
     */
    default_pal,

    /**
     * @brief Use the Porymap-component frame PNG internal palette, if present.
     */
    internal_png_pal,

    /**
     * @brief Scan layouts config and find all tilesets paired with current tileset, see if an index is present in any.
     */
    full_tileset_scan
};

[[nodiscard]] inline std::optional<AnimPalResolutionStrategy>
anim_pal_resolution_strategy_from_str(const std::string &str)
{
    if (str == "error") {
        return std::optional{AnimPalResolutionStrategy::error};
    }
    if (str == "default_pal") {
        return std::optional{AnimPalResolutionStrategy::default_pal};
    }
    if (str == "internal_png_palette") {
        return std::optional{AnimPalResolutionStrategy::internal_png_pal};
    }
    if (str == "full_tileset_scan") {
        return std::optional{AnimPalResolutionStrategy::full_tileset_scan};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::string to_string(const AnimPalResolutionStrategy s)
{
    switch (s) {
    case AnimPalResolutionStrategy::error:
        return "error";
    case AnimPalResolutionStrategy::default_pal:
        return "default_pal";
    case AnimPalResolutionStrategy::internal_png_pal:
        return "internal_png_palette";
    case AnimPalResolutionStrategy::full_tileset_scan:
        return "full_tileset_scan";
    }
    panic("unhandled AnimPalResolutionStrategy value");
}

inline std::ostream &operator<<(std::ostream &os, const AnimPalResolutionStrategy s)
{
    return os << to_string(s);
}

} // namespace porytiles2
