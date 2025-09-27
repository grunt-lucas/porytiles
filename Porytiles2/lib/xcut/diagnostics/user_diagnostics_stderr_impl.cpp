#include "porytiles2/xcut/diagnostics/user_diagnostics_stderr_impl.hpp"

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/text_formatter.hpp"

namespace porytiles2 {

void UserDiagnosticsStderrImpl::note(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    TextFormatter formatter{true};
    std::cerr << formatter.cyan_bold("note:") << " ";
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::warn_note(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    TextFormatter formatter{true};
    std::cerr << formatter.cyan_bold("note:") << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << formatter.cyan_bold(tag) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::warn(const std::string &tag, const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    TextFormatter formatter{true};
    std::cerr << formatter.magenta_bold("warning:") << " ";
    std::cerr << lines.at(0);
    std::cerr << " [" << formatter.magenta_bold(tag) << "]" << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::err(const std::vector<std::string> &lines) const
{
    assert_or_panic(!lines.empty(), "lines vector was empty");
    TextFormatter formatter{true};
    std::cerr << formatter.red_bold("error:") << " ";
    std::cerr << lines.at(0) << std::endl;
    for (const auto &note_line : std::ranges::views::drop(lines, 1)) {
        std::cerr << note_line << std::endl;
    }
}

void UserDiagnosticsStderrImpl::emit_fatal_proximate(const Error &err) const
{
    TextFormatter formatter{true};
    std::cerr << formatter.red_bold("fatal:") << " ";
    std::cerr << err.details(formatter) << std::endl;
}

void UserDiagnosticsStderrImpl::emit_fatal_step(const Error &err) const
{
    TextFormatter formatter{true};
    std::cerr << formatter.bold("│") << " " << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << formatter.bold("│") << " " << std::endl;
    std::cerr << formatter.bold("├") << " " << formatter.red_bold("error:") << " ";
    std::cerr << err.details(formatter) << std::endl;
}

void UserDiagnosticsStderrImpl::emit_fatal_root(const Error &err) const
{
    TextFormatter formatter{true};
    std::cerr << formatter.bold("│") << " " << std::endl;
    std::cerr << "caused by:" << std::endl;
    std::cerr << formatter.bold("│") << " " << std::endl;
    std::cerr << formatter.bold("└") << " " << formatter.red_bold("error:") << " ";
    std::cerr << err.details(formatter) << std::endl;
}

} // namespace porytiles2
