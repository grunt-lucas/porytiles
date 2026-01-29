#pragma once

#include <optional>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Strategy for handling duplicate key frame tiles in animation decompilation.
 *
 * @details
 * When decompiling animations, it's possible for multiple key frame tiles to be identical. This is problematic because
 * during recompilation, identical tiles cannot be distinguished. This enum determines how to handle such cases.
 */
enum class AnimKeyFrameResolutionStrategy {
    /**
     * @brief Emit a formatted error and fail decompilation.
     */
    error,

    /**
     * @brief Mangle duplicate tiles to make them unique, then backport changes to tiles.png.
     */
    mangle
};

/**
 * @brief Parses a string into an AnimKeyFrameResolutionStrategy.
 *
 * @param str The string to parse ("error" or "mangle")
 * @return The parsed strategy, or std::nullopt if the string is invalid
 */
[[nodiscard]] inline std::optional<AnimKeyFrameResolutionStrategy>
anim_key_frame_resolution_strategy_from_str(const std::string &str)
{
    if (str == "error") {
        return std::optional{AnimKeyFrameResolutionStrategy::error};
    }
    if (str == "mangle") {
        return std::optional{AnimKeyFrameResolutionStrategy::mangle};
    }
    return std::nullopt;
}

/**
 * @brief Converts an AnimKeyFrameResolutionStrategy to its string representation.
 *
 * @param s The strategy to convert
 * @return The string representation ("error" or "mangle")
 */
[[nodiscard]] inline std::string to_string(const AnimKeyFrameResolutionStrategy s)
{
    switch (s) {
    case AnimKeyFrameResolutionStrategy::error:
        return "error";
    case AnimKeyFrameResolutionStrategy::mangle:
        return "mangle";
    }
    panic("unhandled AnimKeyFrameResolutionStrategy value");
}

/**
 * @brief Stream insertion operator for AnimKeyFrameResolutionStrategy.
 *
 * @param os The output stream
 * @param s The strategy to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const AnimKeyFrameResolutionStrategy s)
{
    return os << to_string(s);
}

} // namespace porytiles2
