#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/utilities/text/text_wrap.hpp"

namespace {

using namespace porytiles;

// The gutter "│ " that prefixes every body line occupies two visible columns.
constexpr std::size_t gutter_columns = 2;

/// @brief Prints the message body lines under a "│ " gutter, auto-wrapping each to the configured width.
///
/// @details
/// Each element of @p lines is a caller-supplied logical line (an explicit line break the caller wanted preserved).
/// It is further wrapped to fit @p wrap_width, so a long logical line spills onto extra gutter-prefixed physical lines
/// instead of overrunning the terminal. A @p wrap_width of 0 prints the lines unwrapped.
void print_body(
    const TextFormatter &fmt, const Style style, const std::vector<std::string> &lines, const std::size_t wrap_width)
{
    const std::size_t body_width =
        wrap_width == 0 ? 0 : (wrap_width > gutter_columns ? wrap_width - gutter_columns : 1);
    const std::string gutter = fmt.style("│", style);
    for (const auto &logical : lines) {
        for (const auto &physical : wrap_ansi_line(logical, body_width)) {
            std::cerr << gutter << " " << physical << std::endl;
        }
    }
}

} // namespace

namespace porytiles {

void StderrStyledUserDiagnostics::remark(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector is empty");
    assert_or_panic(!tag.empty(), "tag is empty");

    std::cerr << formatter().style("remark ", Style::bold | Style::blue)
              << formatter().style("[", Style::bold | Style::blue) << formatter().style(tag, Style::bold | Style::blue)
              << formatter().style("]:", Style::bold | Style::blue) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::blue) << std::endl;
    print_body(formatter(), Style::bold | Style::blue, lines, wrap_width_);
    std::cerr << formatter().style("│", Style::bold | Style::blue) << std::endl;
}

void StderrStyledUserDiagnostics::warning(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector is empty");
    assert_or_panic(!tag.empty(), "tag is empty");

    std::cerr << formatter().style("warning ", Style::bold | Style::magenta)
              << formatter().style("[", Style::bold | Style::magenta)
              << formatter().style(tag, Style::bold | Style::magenta)
              << formatter().style("]:", Style::bold | Style::magenta) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::magenta) << std::endl;
    print_body(formatter(), Style::bold | Style::magenta, lines, wrap_width_);
    std::cerr << formatter().style("│", Style::bold | Style::magenta) << std::endl;
}

void StderrStyledUserDiagnostics::error(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector is empty");
    assert_or_panic(!tag.empty(), "tag is empty");

    std::cerr << formatter().style("error ", Style::bold | Style::red)
              << formatter().style("[", Style::bold | Style::red) << formatter().style(tag, Style::bold | Style::red)
              << formatter().style("]:", Style::bold | Style::red) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
    print_body(formatter(), Style::bold | Style::red, lines, wrap_width_);
    std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
}

void StderrStyledUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    auto lines = err.details(formatter());
    if (!lines.empty()) {
        std::cerr << formatter().style("fatal:", Style::bold | Style::red) << std::endl;
        std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
        print_body(formatter(), Style::bold | Style::red, lines, wrap_width_);
        std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
    }
}

void StderrStyledUserDiagnostics::emit_fatal_step(const Error &err) const
{
    std::cerr << formatter().style("├ caused by:", Style::bold | Style::red) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::red) << " " << std::endl;

    auto lines = err.details(formatter());
    if (!lines.empty()) {
        print_body(formatter(), Style::bold | Style::red, lines, wrap_width_);
    }
    std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
}

void StderrStyledUserDiagnostics::emit_fatal_root(const Error &err) const
{
    std::cerr << formatter().style("├ root cause:", Style::bold | Style::red) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::red) << " " << std::endl;

    auto lines = err.details(formatter());
    if (!lines.empty()) {
        print_body(formatter(), Style::bold | Style::red, lines, wrap_width_);
    }
    std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
}

void StderrStyledUserDiagnostics::remark_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    emit_note_impl(tag, lines);
}

void StderrStyledUserDiagnostics::warning_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    emit_note_impl(tag, lines);
}

void StderrStyledUserDiagnostics::error_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    emit_note_impl(tag, lines);
}

void StderrStyledUserDiagnostics::emit_note_impl(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector is empty");
    assert_or_panic(!tag.empty(), "tag is empty");

    std::cerr << formatter().style("note ", Style::bold | Style::cyan)
              << formatter().style("[", Style::bold | Style::cyan) << formatter().style(tag, Style::bold | Style::cyan)
              << formatter().style("]:", Style::bold | Style::cyan) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::cyan) << std::endl;
    print_body(formatter(), Style::bold | Style::cyan, lines, wrap_width_);
    std::cerr << formatter().style("│", Style::bold | Style::cyan) << std::endl;
}

} // namespace porytiles
