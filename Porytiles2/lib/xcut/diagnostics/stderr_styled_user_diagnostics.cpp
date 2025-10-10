#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

void StderrStyledUserDiagnostics::note(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    /*
     * TODO: instead of hardcoding AnsiStyledTextFormatter here, we should have this class detect if stderr is a TTY and
     * select a text formatter accordingly.
     */
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("note:", Style::bold | Style::cyan) << " ";
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::warn_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("note:", Style::bold | Style::cyan) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << formatter.style(tag, Style::bold | Style::cyan) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::warn(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("warning:", Style::bold | Style::magenta) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << formatter.style(tag, Style::bold | Style::magenta) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::err(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("error:", Style::bold | Style::red) << " ";
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("fatal:", Style::bold | Style::red) << " ";
    std::cerr << err.details(formatter) << std::endl;
}

void StderrStyledUserDiagnostics::emit_fatal_step(const Error &err) const
{
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << formatter.style("├", Style::bold) << " " << formatter.style("error:", Style::bold | Style::red) << " ";
    std::cerr << err.details(formatter) << std::endl;
}

void StderrStyledUserDiagnostics::emit_fatal_root(const Error &err) const
{
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << "root cause:" << std::endl;
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << formatter.style("└", Style::bold) << " " << formatter.style("error:", Style::bold | Style::red) << " ";
    std::cerr << err.details(formatter) << std::endl;
}

} // namespace porytiles2
