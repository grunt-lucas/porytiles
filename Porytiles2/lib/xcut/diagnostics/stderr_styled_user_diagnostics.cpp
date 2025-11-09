#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

void StderrStyledUserDiagnostics::note(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    /*
     * TODO: instead of hardcoding AnsiStyledTextFormatter here, we should have this class detect if stderr is a TTY and
     * select a text formatter accordingly.
     */
    std::cerr << format_->style("note:", Style::bold | Style::cyan) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << format_->style(tag, Style::bold | Style::cyan) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << format_->style("│", Style::bold | Style::cyan) << " " << note_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::warn_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << format_->style("note:", Style::bold | Style::cyan) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << format_->style(tag, Style::bold | Style::cyan) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << format_->style("│", Style::bold | Style::cyan) << " " << note_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::warn(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << format_->style("warning:", Style::bold | Style::magenta) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << format_->style(tag, Style::bold | Style::magenta) << "]" << std::endl;
    for (const auto &warn_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << format_->style("│", Style::bold | Style::magenta) << " " << warn_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::err(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << format_->style("error:", Style::bold | Style::red) << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << format_->style(tag, Style::bold | Style::red) << "]" << std::endl;
    for (const auto &err_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << format_->style("│", Style::bold | Style::red) << " " << err_line << std::endl;
    }
}

void StderrStyledUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    // TODO: emit some kind of cool ascii art for start of fatal chain
    // something like:
    // |-------- FATAL ERROR CHAIN --------|
    // |-----------------------------------|
    // or something similar
    auto lines = err.details(*format_);
    if (!lines.empty()) {
        std::cerr << format_->style("fatal:", Style::bold | Style::red) << " ";
        std::cerr << lines.at(0) << std::endl;
        for (const auto &line : std::ranges::views::drop(lines, 1)) {
            std::cerr << format_->style("│", Style::bold | Style::red) << " " << line << std::endl;
        }
    }
}

void StderrStyledUserDiagnostics::emit_fatal_step(const Error &err) const
{
    std::cerr << format_->style("│", Style::bold | Style::red) << " " << std::endl;
    std::cerr << format_->style("├ caused by:", Style::bold | Style::red) << std::endl;
    std::cerr << format_->style("│", Style::bold | Style::red) << " " << std::endl;

    auto lines = err.details(*format_);
    if (!lines.empty()) {
        std::cerr << format_->style("│", Style::bold | Style::red) << " " << lines.at(0) << std::endl;
        for (const auto &line : std::ranges::views::drop(lines, 1)) {
            std::cerr << format_->style("│", Style::bold | Style::red) << " " << line << std::endl;
        }
    }
}

void StderrStyledUserDiagnostics::emit_fatal_root(const Error &err) const
{
    std::cerr << format_->style("│", Style::bold | Style::red) << " " << std::endl;
    std::cerr << format_->style("├ root cause:", Style::bold | Style::red) << std::endl;
    std::cerr << format_->style("│", Style::bold | Style::red) << " " << std::endl;

    auto lines = err.details(*format_);
    if (!lines.empty()) {
        std::cerr << format_->style("│", Style::bold | Style::red) << " " << lines.at(0) << std::endl;
        for (const auto &line : std::ranges::views::drop(lines, 1)) {
            std::cerr << format_->style("│", Style::bold | Style::red) << " " << line << std::endl;
        }
    }
}

} // namespace porytiles2
