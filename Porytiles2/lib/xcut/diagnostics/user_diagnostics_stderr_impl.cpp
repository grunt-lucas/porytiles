#include "porytiles2/xcut/diagnostics/user_diagnostics_stderr_impl.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

void UserDiagnosticsStderrImpl::note(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("note:", Style::cyan) << " ";
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::warn_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("note:", Style::cyan) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << formatter.style(tag, Style::cyan) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::warn(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("warning:", Style::magenta) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << formatter.style(tag, Style::magenta) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::err(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("error:", Style::red) << " ";
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::emit_fatal_proximate(const Error &err) const
{
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("fatal:", Style::red) << " ";
    std::cerr << err.details(formatter) << std::endl;
}

void UserDiagnosticsStderrImpl::emit_fatal_step(const Error &err) const
{
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << formatter.style("├", Style::bold) << " " << formatter.style("error:", Style::red) << " ";
    std::cerr << err.details(formatter) << std::endl;
}

void UserDiagnosticsStderrImpl::emit_fatal_root(const Error &err) const
{
    AnsiStyledTextFormatter formatter{};
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << formatter.style("│", Style::bold) << " " << std::endl;
    std::cerr << formatter.style("└", Style::bold) << " " << formatter.style("error:", Style::red) << " ";
    std::cerr << err.details(formatter) << std::endl;
}

} // namespace porytiles2
