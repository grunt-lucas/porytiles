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
/*
 * TODO: add configurable tag filtering handling here. Handle in the UserDiagnostics interface? Or handle it here
 * in the implementation? We want to have an include-list and exclude-list of tags. Make them regex-able? Lots of
 * questions to answer.
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
    explicit StderrStyledUserDiagnostics(const gsl::not_null<TextFormatter *> format) : UserDiagnostics{format} {}

    /**
     * @brief Display a multi-line tagged informational remark to stderr.
     *
     * @details
     * Outputs informational messages with blue "remark:" prefix and tag suffix on the first line, formatted as "remark:
     * <message> [<tag>]" with appropriate indentation for subsequent lines.
     *
     * @param tag Categorization tag for the remark
     * @param lines Vector of strings representing each line of the remark
     */
    void remark(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Display a multi-line tagged note associated with a remark to stderr.
     *
     * @details
     * Outputs informational messages with cyan "note:" prefix and tag suffix on the first line, formatted as "note:
     * <message> [<tag>]" with appropriate indentation for subsequent lines. Uses identical styling to warning_note and
     * error_note since notes are informational regardless of parent diagnostic type.
     *
     * @param tag Categorization tag for the note (should match the parent remark's tag)
     * @param lines Vector of strings representing each line of the note
     */
    void remark_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Display a multi-line tagged note associated with a warning to stderr.
     *
     * @details
     * Outputs informational messages with cyan "note:" prefix and tag suffix on the first line, formatted as "note:
     * <message> [<tag>]" with appropriate indentation for subsequent lines. Uses identical styling to remark_note and
     * error_note since notes are informational regardless of parent diagnostic type.
     *
     * @param tag Categorization tag for the note (should match the parent warning's tag)
     * @param lines Vector of strings representing each line of the note
     */
    void warning_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    /**
     * @brief Display a multi-line tagged note associated with an error to stderr.
     *
     * @details
     * Outputs informational messages with cyan "note:" prefix and tag suffix on the first line, formatted as "note:
     * <message> [<tag>]" with appropriate indentation for subsequent lines. Uses identical styling to remark_note and
     * warning_note since notes are informational regardless of parent diagnostic type.
     *
     * @param tag Categorization tag for the note (should match the parent error's tag)
     * @param lines Vector of strings representing each line of the note
     */
    void error_note(const std::string &tag, const std::vector<std::string> &lines) const override;

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
    void warning(const std::string &tag, const std::vector<std::string> &lines) const override;

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
    void error(const std::string &tag, const std::vector<std::string> &lines) const override;

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
    void emit_note_impl(const std::string &tag, const std::vector<std::string> &lines) const;
};

} // namespace porytiles2
