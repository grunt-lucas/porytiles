#pragma once

#include <optional>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Specifies whether tiles can be modified during patch compilation.
 *
 * @details
 * Controls the behavior of tile handling when compiling a tileset in patch mode. In fixed mode, the compiler must use
 * only the existing tiles from the base tileset in the TilesPngWorkspace. In free mode, the compiler can add new tiles
 * to open slots in TilesPngWorkspace as needed.
 */
enum class PatchTilesMode {
    /** @brief Tiles cannot be added; must use existing tiles from base tileset */
    fixed,
    /** @brief Tiles can be freely added to open slots during patch compilation */
    free
};

[[nodiscard]] inline std::optional<PatchTilesMode> patch_tiles_mode_from_str(const std::string &str)
{
    if (str == "fixed") {
        return std::optional{PatchTilesMode::fixed};
    }
    if (str == "free") {
        return std::optional{PatchTilesMode::free};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::string to_string(const PatchTilesMode m)
{
    switch (m) {
    case PatchTilesMode::fixed:
        return "fixed";
    case PatchTilesMode::free:
        return "free";
    }
    panic("unhandled PatchTilesMode value");
}

inline std::ostream &operator<<(std::ostream &os, const PatchTilesMode m)
{
    return os << to_string(m);
}

/**
 * @brief Specifies whether palettes can be modified during patch compilation.
 *
 * @details
 * Controls the behavior of palette handling when compiling a tileset patch. In fixed mode, the compiler may use only
 * the existing palettes from the base tileset without modifications. In free mode, the compiler can modify unused
 * palette slots as needed.
 */
enum class PatchPalMode {
    /** @brief Palettes cannot be modified; must use existing palettes from base tileset as-is */
    fixed,
    /** @brief Unused palette slots can be freely modified during patch compilation */
    free
};

[[nodiscard]] inline std::optional<PatchPalMode> patch_pal_mode_from_str(const std::string &str)
{
    if (str == "fixed") {
        return std::optional{PatchPalMode::fixed};
    }
    if (str == "free") {
        return std::optional{PatchPalMode::free};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::string to_string(const PatchPalMode m)
{
    switch (m) {
    case PatchPalMode::fixed:
        return "fixed";
    case PatchPalMode::free:
        return "free";
    }
    panic("unhandled PatchPalMode value");
}

inline std::ostream &operator<<(std::ostream &os, const PatchPalMode m)
{
    return os << to_string(m);
}

} // namespace porytiles2
