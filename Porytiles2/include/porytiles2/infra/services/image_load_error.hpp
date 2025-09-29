#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

class ImageLoadError final : public Error {
  public:
    enum class Type { file_not_found, unsupported_channel_count, other_load_error };

    struct ChannelCount {
        int channel_count_;
    };

    struct OtherLoadError {
        std::string load_error_;
    };

    ImageLoadError(Type type, std::string filename, std::variant<std::monostate, ChannelCount, OtherLoadError> params)
        : type_{type}, filename_{std::move(filename)}, params_{std::move(params)}
    {
    }

    // Convenience constructors for specific error types
    static ImageLoadError file_not_found(const std::string &filename)
    {
        return ImageLoadError{Type::file_not_found, filename, std::monostate{}};
    }

    static ImageLoadError unsupported_channel_count(const std::string &filename, int channel_count)
    {
        return ImageLoadError{Type::unsupported_channel_count, filename, ChannelCount{channel_count}};
    }

    static ImageLoadError other_load_error(const std::string &filename, const std::string &error_msg)
    {
        return ImageLoadError{Type::other_load_error, filename, OtherLoadError{error_msg}};
    }

    [[nodiscard]] Type type() const
    {
        return type_;
    }
    [[nodiscard]] const std::string &filename() const
    {
        return filename_;
    }
    [[nodiscard]] const std::variant<std::monostate, ChannelCount, OtherLoadError> &params() const
    {
        return params_;
    }

    [[nodiscard]] std::string details(const TextFormatter &formatter) const override
    {
        switch (type_) {
        case Type::file_not_found:
            return formatter.style(filename_ + ":", Style::bold) + " file not found";
        case Type::unsupported_channel_count: {
            auto channel_count = std::get<ChannelCount>(params_).channel_count_;
            return formatter.style(filename_ + ":", Style::bold) +
                   " unsupported channel count: " + formatter.style(std::to_string(channel_count), Style::bold);
        }
        case Type::other_load_error: {
            auto load_error = std::get<OtherLoadError>(params_).load_error_;
            return formatter.style(filename_ + ":", Style::bold) + " could not be loaded: '" + load_error + "'";
        }
        default:
            panic("unhandled ImageLoadError type");
        }
    }

    [[nodiscard]] std::unique_ptr<Error> clone() const override
    {
        return std::make_unique<ImageLoadError>(this->type_, this->filename_, this->params_);
    }

  private:
    Type type_;
    std::string filename_;
    std::variant<std::monostate, ChannelCount, OtherLoadError> params_;
};

} // namespace porytiles2
