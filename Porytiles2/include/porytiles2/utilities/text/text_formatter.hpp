#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace porytiles2 {

enum class Style : uint32_t {
    none = 0,
    bold = 1 << 0,
    red = 1 << 1,
    green = 1 << 2,
    blue = 1 << 3,
    yellow = 1 << 4,
    cyan = 1 << 5,
    magenta = 1 << 6
};

// Bitwise operators for Style enum
[[nodiscard]] constexpr Style operator|(Style lhs, Style rhs)
{
    return static_cast<Style>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

[[nodiscard]] constexpr Style operator&(Style lhs, Style rhs)
{
    return static_cast<Style>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

constexpr Style &operator|=(Style &lhs, Style rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr Style &operator&=(Style &lhs, Style rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

// Helper function to check if a style flag is set
[[nodiscard]] constexpr bool has_style(Style styles, Style flag)
{
    return (styles & flag) != Style::none;
}

struct FormatParam {
    std::string text;
    Style styles;
};

class TextFormatter {
  public:
    virtual ~TextFormatter() = default;

    [[nodiscard]] virtual std::string style(const std::string &text, Style styles) const = 0;

    [[nodiscard]] virtual std::string
    format(const std::string &format_str, const std::vector<FormatParam> &params) const;

    // template<typename... FormatParams>
    // [[nodiscard]] std::string format(const std::string &format_str, FormatParams&&... params) const {
    //     static_assert((std::is_same_v<std::decay_t<FormatParams>, FormatParam> && ...));
    //     std::vector<FormatParam> param_vector{std::forward<FormatParams>(params)...};
    //     return this->format(format_str, param_vector);
    // }
};

using FormattedMessageBuilder = std::function<std::vector<std::string>(const TextFormatter &)>;

} // namespace porytiles2
