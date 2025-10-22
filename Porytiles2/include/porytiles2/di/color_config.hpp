#pragma once

#include <string>
#include <unistd.h> // For isatty()

namespace porytiles2::di {

/**
 * @brief Runtime configuration derived from CLI arguments and environment.
 *
 * @details
 * This structure captures all runtime parameters that affect dependency injection decisions. It's used to conditionally
 * install different component implementations based on user preferences and environment detection (e.g., TTY detection
 * for color output).
 */
struct ColorConfig {
    bool no_color{false};          ///< User explicitly disabled color output
    bool verbose{false};           ///< Enable verbose logging
    std::string project_root{"."}; ///< Root directory of the project
    std::string tileset_name;      ///< Name of the tileset being processed

    /**
     * @brief Determines if styled (ANSI) output should be used.
     *
     * @details
     * Color output is enabled when:
     * - User has not set --no-color flag
     * - AND stderr is connected to a TTY
     *
     * @return True if ANSI styled output should be used, false for plain text
     */
    [[nodiscard]] bool should_use_color() const
    {
        return !no_color && isatty(STDERR_FILENO);
    }
};

} // namespace porytiles2::di
