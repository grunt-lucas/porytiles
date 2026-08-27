#pragma once

#include <memory>
#include <string>
#include <vector>

#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief Abstract interface for all error types used in ChainableResult error chains.
///
/// @details
/// All concrete error types used with ChainableResult must derive from this interface. Implementations should be
/// immutable value types that capture all relevant context about a failure. The clone pattern enables proper copying
/// of errors when building error chains.
class Error {
  public:
    virtual ~Error() = default;

    /// @brief Returns a formatted multi-line string representation of the error.
    ///
    /// @details
    /// Each element in the returned vector represents one line of the error message. The TextFormatter controls whether
    /// ANSI styling codes are included based on TTY status.
    ///
    /// @param formatter The TextFormatter to use for conditional formatting based on TTY status
    /// @return A vector of formatted strings describing the error, one per line
    [[nodiscard]] virtual std::vector<std::string> details(const TextFormatter &formatter) const = 0;

    [[nodiscard]] virtual std::string join(const TextFormatter &formatter, const std::string &delimiter = "\n") const
    {
        const auto lines = details(formatter);
        if (lines.empty()) {
            return "";
        }

        std::string result;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) {
                result += delimiter;
            }
            result += lines[i];
        }
        return result;
    }

    [[nodiscard]] virtual std::unique_ptr<Error> clone() const = 0;
};

/// @brief General-purpose error implementation with formatted message support.
///
/// @details
/// FormattableError is a concrete Error implementation for common error scenarios where a specialized error type would
/// be unnecessary overhead. It supports plain string messages, format strings with styled FormatParam substitution, and
/// multi-line error messages. TTY-aware styling is handled automatically through TextFormatter.
class FormattableError final : public Error {
  public:
    FormattableError() = default;

    explicit FormattableError(std::string text)
    {
        if (!text.empty()) {
            text_.push_back(std::move(text));
        }
    }

    explicit FormattableError(std::string text, std::vector<FormatParam> params)
    {
        if (!text.empty() || !params.empty()) {
            text_.push_back(std::move(text));
            params_.push_back(std::move(params));
        }
    }

    template <typename FirstParam, typename... RestParams>
        requires(
            !std::is_same_v<std::decay_t<FirstParam>, std::vector<FormatParam>> &&
            std::is_same_v<std::decay_t<FirstParam>, FormatParam> &&
            (std::is_same_v<std::decay_t<RestParams>, FormatParam> && ...))
    explicit FormattableError(std::string text, FirstParam &&first, RestParams &&...rest)
    {
        text_.push_back(std::move(text));

        std::vector<FormatParam> line_params;
        line_params.reserve(1 + sizeof...(RestParams));
        line_params.push_back(std::forward<FirstParam>(first));
        (line_params.push_back(std::forward<RestParams>(rest)), ...);
        params_.push_back(std::move(line_params));
    }

    explicit FormattableError(std::vector<std::string> lines) : text_{std::move(lines)} {}

    explicit FormattableError(std::vector<std::string> lines, std::vector<std::vector<FormatParam>> params)
        : text_{std::move(lines)}, params_{std::move(params)}
    {
    }

    [[nodiscard]] std::vector<std::string> details(const TextFormatter &formatter) const override
    {
        std::vector<std::string> result;
        result.reserve(text_.size());

        for (std::size_t i = 0; i < text_.size(); ++i) {
            if (i < params_.size() && !params_[i].empty()) {
                result.push_back(formatter.format(text_[i], params_[i]));
            }
            else {
                result.push_back(text_[i]);
            }
        }

        return result;
    }

    [[nodiscard]] bool has_details() const
    {
        for (const auto &line : text_) {
            if (!line.empty()) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<FormattableError>(text_, params_);
    }

  private:
    std::vector<std::string> text_;
    std::vector<std::vector<FormatParam>> params_;
};

} // namespace porytiles
