#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace porytiles2 {

enum class Style { bold, red, green, blue, yellow, cyan, magenta };

struct FormatParam {
    std::string text;
    Style style;
};

class TextFormatter {
  public:
    virtual ~TextFormatter() = default;

    [[nodiscard]] virtual std::string style(const std::string &text, Style style) const = 0;

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
