#pragma once

#include <filesystem>
#include <string>

#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Resolves the user's preferred text editor and launches it on a file.
///
/// @details
/// Editor resolution follows some standard environment variable conventions, checked in the following order:
/// @c PORYTILES_EDITOR, then @c VISUAL, then @c EDITOR. Empty values are skipped. When none of the variables are set,
/// the launcher falls back to the first of @c nano, @c vim, @c vi found on the @c PATH.
///
/// The resolved editor command runs through the shell, so environment values that include arguments (e.g.
/// @c "code --wait") work as expected. The file path is quoted before interpolation, so paths with spaces or shell
/// metacharacters work correctly.
class EditorLauncher {
  public:
    /// @brief Resolves the editor command via the environment variable cascade.
    ///
    /// @return The editor shell command, or an error when no editor could be resolved
    [[nodiscard]] ChainableResult<std::string> resolve_editor_command() const;

    /// @brief Opens the given file in the resolved editor and blocks until the editor exits.
    ///
    /// @param file_path The file to open in the editor
    /// @return An error when no editor could be resolved, the editor could not be launched, or the editor exited with a
    /// non-zero status
    [[nodiscard]] ChainableResult<void> edit_file(const std::filesystem::path &file_path) const;
};

} // namespace porytiles
