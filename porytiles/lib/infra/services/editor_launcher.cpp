#include "porytiles/infra/services/editor_launcher.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "porytiles/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles;

/// @brief Reads an environment variable, treating unset and empty values the same.
std::optional<std::string> non_empty_env(const char *name)
{
    const char *value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string{value};
}

/// @brief Checks whether an executable with the given name exists in any PATH directory.
bool found_on_path(const std::string &name)
{
    const auto path_env = non_empty_env("PATH");
    if (!path_env.has_value()) {
        return false;
    }

    std::istringstream stream{path_env.value()};
    std::string dir;
    while (std::getline(stream, dir, ':')) {
        if (dir.empty()) {
            continue;
        }
        const std::filesystem::path candidate = std::filesystem::path{dir} / name;
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec) && access(candidate.c_str(), X_OK) == 0) {
            return true;
        }
    }
    return false;
}

/// @brief Quotes text for safe interpolation into a POSIX shell command.
///
/// @details
/// Wraps the text in single quotes. A literal single quote has no in-quote escape in POSIX shells, so it becomes
/// '\''. Close the quoted string, emit an escaped quote, reopen the quoted string.
std::string shell_quote(const std::string &text)
{
    std::string quoted = "'";
    for (const char c : text) {
        if (c == '\'') {
            quoted += "'\\''";
        }
        else {
            quoted += c;
        }
    }
    quoted += '\'';
    return quoted;
}

} // namespace

namespace porytiles {

ChainableResult<std::string> EditorLauncher::resolve_editor_command() const
{
    constexpr std::array env_vars = {"PORYTILES_EDITOR", "VISUAL", "EDITOR"};
    for (const char *var : env_vars) {
        if (auto value = non_empty_env(var); value.has_value()) {
            return std::move(value).value();
        }
    }

    constexpr std::array fallbacks = {"nano", "vim", "vi"};
    for (const char *fallback : fallbacks) {
        if (found_on_path(fallback)) {
            return std::string{fallback};
        }
    }

    return FormattableError{
        "No editor found. Set the '{}' environment variable to your preferred editor.",
        FormatParam{"EDITOR", Style::bold}};
}

ChainableResult<void> EditorLauncher::edit_file(const std::filesystem::path &file_path) const
{
    PT_TRY_ASSIGN_PASS_ERR(editor, resolve_editor_command(), void);

    const std::string command = editor + " " + shell_quote(file_path.string());
    const int status = std::system(command.c_str());

    if (status == -1) {
        return FormattableError{"Failed to launch editor '{}'.", FormatParam{editor, Style::bold}};
    }
    if (WIFSIGNALED(status)) {
        return FormattableError{
            "Editor '{}' was terminated by signal {}.",
            FormatParam{editor, Style::bold},
            FormatParam{std::to_string(WTERMSIG(status))}};
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        return FormattableError{
            "Editor '{}' exited with status {}.",
            FormatParam{editor, Style::bold},
            FormatParam{std::to_string(WEXITSTATUS(status))}};
    }

    return {};
}

} // namespace porytiles
