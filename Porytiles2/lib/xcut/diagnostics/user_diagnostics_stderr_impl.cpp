#include "porytiles2/xcut/diagnostics/user_diagnostics_stderr_impl.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "fmt/xchar.h"

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

void UserDiagnosticsStderrImpl::note(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << fmt::format("{} ", fmt::styled("note:", fmt::emphasis::bold | fg(fmt::terminal_color::cyan)));
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::warn_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << fmt::format("{} ", fmt::styled("note:", fmt::emphasis::bold | fg(fmt::terminal_color::cyan)));
    std::cerr << lines.at(0);
    std::cerr << fmt::format(" [{}]", fmt::styled(tag, fmt::emphasis::bold | fg(fmt::terminal_color::cyan)))
              << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::warn(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << fmt::format("{} ", fmt::styled("warning:", fmt::emphasis::bold | fg(fmt::terminal_color::magenta)));
    std::cerr << lines.at(0);
    std::cerr << fmt::format(" [{}]", fmt::styled(tag, fmt::emphasis::bold | fg(fmt::terminal_color::magenta)))
              << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::err(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    std::cerr << fmt::format("{} ", fmt::styled("error:", fmt::emphasis::bold | fg(fmt::terminal_color::red)));
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::emit_fatal_proximate(const Error &err) const
{
    std::cerr << fmt::format("{} ", fmt::styled("fatal:", fmt::emphasis::bold | fg(fmt::terminal_color::red)));
    std::cerr << err.details(TextFormatter{true}) << std::endl;
}

void UserDiagnosticsStderrImpl::emit_fatal_step(const Error &err) const
{
    std::cerr << fmt::format("{} ", fmt::styled("│", fmt::emphasis::bold)) << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << fmt::format("{} ", fmt::styled("│", fmt::emphasis::bold)) << std::endl;
    std::cerr << fmt::format(
        "{} {} ",
        fmt::styled("├", fmt::emphasis::bold),
        fmt::styled("error:", fmt::emphasis::bold | fg(fmt::terminal_color::red)));
    std::cerr << err.details(TextFormatter{true}) << std::endl;
}

void UserDiagnosticsStderrImpl::emit_fatal_root(const Error &err) const
{
    std::cerr << fmt::format("{} ", fmt::styled("│", fmt::emphasis::bold)) << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << fmt::format("{} ", fmt::styled("│", fmt::emphasis::bold)) << std::endl;
    std::cerr << fmt::format(
        "{} {} ",
        fmt::styled("└", fmt::emphasis::bold),
        fmt::styled("error:", fmt::emphasis::bold | fg(fmt::terminal_color::red)));
    std::cerr << err.details(TextFormatter{true}) << std::endl;
}

} // namespace porytiles2
