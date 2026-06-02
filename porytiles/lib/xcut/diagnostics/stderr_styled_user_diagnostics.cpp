#include "porytiles/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

void StderrStyledUserDiagnostics::remark(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector is empty");
    assert_or_panic(!tag.empty(), "tag is empty");

    std::cerr << formatter().style("remark ", Style::bold | Style::blue)
              << formatter().style("[", Style::bold | Style::blue) << formatter().style(tag, Style::bold | Style::blue)
              << formatter().style("]:", Style::bold | Style::blue) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::blue) << std::endl;
    for (const auto &line : lines) {
        std::cerr << formatter().style("│", Style::bold | Style::blue) << " " << line << std::endl;
    }
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
    for (const auto &line : lines) {
        std::cerr << formatter().style("│", Style::bold | Style::magenta) << " " << line << std::endl;
    }
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
    for (const auto &line : lines) {
        std::cerr << formatter().style("│", Style::bold | Style::red) << " " << line << std::endl;
    }
    std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
}

void StderrStyledUserDiagnostics::emit_fatal_proximate(const Error &err) const
{
    auto lines = err.details(formatter());
    if (!lines.empty()) {
        std::cerr << formatter().style("fatal:", Style::bold | Style::red) << std::endl;
        std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
        for (const auto &line : lines) {
            std::cerr << formatter().style("│", Style::bold | Style::red) << " " << line << std::endl;
        }
        std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
    }
}

void StderrStyledUserDiagnostics::emit_fatal_step(const Error &err) const
{
    std::cerr << formatter().style("├ caused by:", Style::bold | Style::red) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::red) << " " << std::endl;

    auto lines = err.details(formatter());
    if (!lines.empty()) {
        std::cerr << formatter().style("│", Style::bold | Style::red) << " " << lines.at(0) << std::endl;
        for (const auto &line : std::ranges::views::drop(lines, 1)) {
            std::cerr << formatter().style("│", Style::bold | Style::red) << " " << line << std::endl;
        }
    }
    std::cerr << formatter().style("│", Style::bold | Style::red) << std::endl;
}

void StderrStyledUserDiagnostics::emit_fatal_root(const Error &err) const
{
    std::cerr << formatter().style("├ root cause:", Style::bold | Style::red) << std::endl;
    std::cerr << formatter().style("│", Style::bold | Style::red) << " " << std::endl;

    auto lines = err.details(formatter());
    if (!lines.empty()) {
        std::cerr << formatter().style("│", Style::bold | Style::red) << " " << lines.at(0) << std::endl;
        for (const auto &line : std::ranges::views::drop(lines, 1)) {
            std::cerr << formatter().style("│", Style::bold | Style::red) << " " << line << std::endl;
        }
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
    for (const auto &line : lines) {
        std::cerr << formatter().style("│", Style::bold | Style::cyan) << " " << line << std::endl;
    }
    std::cerr << formatter().style("│", Style::bold | Style::cyan) << std::endl;
}

} // namespace porytiles
