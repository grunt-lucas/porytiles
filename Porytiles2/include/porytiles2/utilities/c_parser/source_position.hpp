#pragma once

#include <cstddef>
#include <string>

#include "fmt/format.h"

namespace porytiles2 {

/**
 * @brief Represents a position within source content.
 *
 * @details
 * SourcePosition tracks the line number and column number within source text. Line and column numbers are 1-based to
 * match conventional editor/compiler conventions. This is used by Token and error messages to provide helpful location
 * information.
 */
struct SourcePosition {
    std::size_t line{1};   ///< 1-based line number
    std::size_t column{1}; ///< 1-based column number

    /**
     * @brief Formats the position as "line:column".
     *
     * @return A string in the format "line:column"
     */
    [[nodiscard]] std::string to_string() const
    {
        return fmt::format("{}:{}", line, column);
    }

    /**
     * @brief Compares two positions for equality.
     *
     * @param other The other position to compare against
     * @return True if both line and column match
     */
    [[nodiscard]] bool operator==(const SourcePosition &other) const = default;
};

} // namespace porytiles2
