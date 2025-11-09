#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/*
 * TODO: change this class to ConfigurableUserDiagnostics. We should expose for configuration:
 * 1. Where the diagnostics go: stderr, stdout, a file?
 * 2. Colors on or off
 * 3. Diagnostic tag include and exclude filters
 * 4. Diagnostic tag settable limits: stop showing diagnostics with a given tag when a limit is hit
 */

/**
 * @brief Concrete implementation of UserDiagnostics that outputs structured messages to stderr, optionally with colored
 * formatting.
 *
 * @details
 * StderrStyledUserDiagnostics provides a terminal-based implementation of the UserDiagnostics interface. It outputs all
 * diagnostic messages to stderr with some additional pretty-print structuring. If provided with an
 * AnsiStyledTextFormatter, it will additionally use canonical diagnostic coloring and styling (canonical, i.e. magenta
 * for warnings, red for errors, bolding where appropriate, etc.) via ANSI terminal codes. The implementation includes:
 *
 * - **Colored Output**: Uses ANSI codes for terminal colors (cyan for notes, magenta for warnings, red for errors)
 * - **Multi-line Support**: First line gets the appropriate prefix, subsequent lines are indented with visual
 * guidelines to aid reading
 * - **Error Chain Visualization**: Fatal errors are displayed with tree-like formatting using Unicode box-drawing
 * characters to show error hierarchy
 */
class StderrStyledUserDiagnostics final : public UserDiagnostics {
  public:
    explicit StderrStyledUserDiagnostics(const gsl::not_null<TextFormatter *> format) : format_{format} {}

    /**
     * @brief Display a multi-line tagged informational note to stderr.
     *
     * @details
     * Outputs informational messages with cyan "note:" prefix and tag suffix on the first line, formatted as "note:
     * <message> [<tag>]" with appropriate indentation for subsequent lines.
     *
     * @param tag Categorization tag for the note
     * @param lines Vector of strings representing each line of the note
     */
    void note(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Display a multi-line tagged warning note to stderr.
     *
     * @details
     * Outputs warning notes with cyan "note:" prefix and tag suffix on the first line, formatted as "note: <message>
     * [<tag>]" with appropriate indentation for subsequent lines.
     *
     * @param tag Categorization tag for the warning note
     * @param lines Vector of strings representing each line of the warning note
     */
    void warn_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Display a multi-line tagged warning to stderr.
     *
     * @details
     * Outputs warnings with magenta "warning:" prefix and tag suffix on the first line, formatted as "warning:
     * <message> [<tag>]" with appropriate indentation for subsequent lines.
     *
     * @param tag Categorization tag for the warning
     * @param lines Vector of strings representing each line of the warning
     */
    void warn(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Display a multi-line tagged error message to stderr.
     *
     * @details
     * Outputs error messages with red "error:" prefix and tag suffix on the first line, formatted as "error: <message>
     * [<tag>]" with appropriate indentation for subsequent lines.
     *
     * @param tag Categorization tag for the error
     * @param lines Vector of strings representing each line of the error
     */
    void err(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Emit the proximate (immediate) error in a fatal error chain to stderr.
     *
     * @details
     * Displays the most immediate error in a fatal error chain with red "fatal:" prefix and the highest visual
     * prominence. This represents the direct cause of the failure.
     *
     * @param err The proximate error to display
     */
    void emit_fatal_proximate(const Error &err) const override;

    /**
     * @brief Emit an intermediate step error in a fatal error chain to stderr.
     *
     * @details
     * Displays intermediate errors in a fatal error chain using tree-like formatting with Unicode box-drawing
     * characters (├ symbols). These represent the intermediate causes between the proximate and root errors in the
     * failure chain.
     *
     * @param err The step error to display
     */
    void emit_fatal_step(const Error &err) const override;

    /**
     * @brief Emit the root cause error in a fatal error chain to stderr.
     *
     * @details
     * Displays the root cause error in a fatal error chain using tree-like formatting with Unicode box-drawing
     * characters (└ symbols). This represents the original underlying cause of the failure and appears as the final
     * item in the error hierarchy visualization.
     *
     * @param err The root cause error to display
     */
    void emit_fatal_root(const Error &err) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles2
