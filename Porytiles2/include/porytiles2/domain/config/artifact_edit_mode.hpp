#pragma once

#include <optional>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Specifies whether artifacts (tiles or palettes) can be modified during patch compilation.
 *
 * @details
 * Controls the behavior of tile and palette handling when compiling a tileset patch. In locked mode, the compiler must
 * use only the existing artifacts from the base tileset without modifications. In patch mode, the compiler can modify
 * unused or "open" slots as needed. In optimize mode, artifacts are cleared and packed optimally (Porytiles1 behavior).
 */
enum class ArtifactEditMode {
    /**
     * @brief Artifacts cannot be edited; must use existing from base tileset.
     */
    locked,

    /**
     * @brief Artifacts can be freely edited in "open" slots during compilation.
     */
    patch,

    /**
     * @brief The Porytiles1 behavior: artifact is cleared and packed optimally.
     */
    optimize
};

[[nodiscard]] inline std::optional<ArtifactEditMode> artifact_edit_mode_from_str(const std::string &str)
{
    if (str == "locked") {
        return std::optional{ArtifactEditMode::locked};
    }
    if (str == "patch") {
        return std::optional{ArtifactEditMode::patch};
    }
    if (str == "optimize") {
        return std::optional{ArtifactEditMode::optimize};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::string to_string(const ArtifactEditMode m)
{
    switch (m) {
    case ArtifactEditMode::locked:
        return "locked";
    case ArtifactEditMode::patch:
        return "patch";
    case ArtifactEditMode::optimize:
        return "optimize";
    }
    panic("unhandled ArtifactEditMode value");
}

inline std::ostream &operator<<(std::ostream &os, const ArtifactEditMode m)
{
    return os << to_string(m);
}

} // namespace porytiles2
