#include "porytiles2/xcut/result/text_formatter.hpp"

#include <string>
#include <vector>

#include "fmt/args.h"
#include "fmt/format.h"

namespace {

std::string substitute_params(const std::string &format_str, const std::vector<std::string> &params)
{
    // Use fmt::vformat for parameter substitution while keeping it internal
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (const auto &param : params) {
        store.push_back(param);
    }
    return fmt::vformat(format_str, store);
}

} // namespace

namespace porytiles2 {

std::string TextFormatter::bold(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[1m" + text + "\033[0m";
}

std::string TextFormatter::red(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[31m" + text + "\033[0m";
}

std::string TextFormatter::cyan(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[36m" + text + "\033[0m";
}

std::string TextFormatter::magenta(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[35m" + text + "\033[0m";
}

std::string TextFormatter::red_bold(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[1;31m" + text + "\033[0m";
}

std::string TextFormatter::cyan_bold(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[1;36m" + text + "\033[0m";
}

std::string TextFormatter::magenta_bold(const std::string &text) const
{
    if (!is_a_tty_) {
        return text;
    }
    return "\033[1;35m" + text + "\033[0m";
}

std::string
TextFormatter::format_with_bold_params(const std::string &format_str, const std::vector<std::string> &params) const
{
    if (!is_a_tty_) {
        return substitute_params(format_str, params);
    }

    // Create bold versions of each parameter
    std::vector<std::string> bold_params;
    bold_params.reserve(params.size());
    for (const auto &param : params) {
        bold_params.push_back(bold(param));
    }

    return substitute_params(format_str, bold_params);
}

} // namespace porytiles2