#pragma once

#include <ranges>
#include <string>

#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"
#include "porytiles2/xcut/result/error.hpp"

namespace porytiles2 {

class UserDiagnostics {
  public:
    virtual ~UserDiagnostics() = default;

    void note(const std::string &msg) const
    {
        note(std::vector{msg});
    }

    virtual void note(const std::vector<std::string> &lines) const = 0;

    void warn_note(const std::string &tag, const std::string &msg) const
    {
        warn_note(tag, std::vector{msg});
    }

    virtual void warn_note(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    void warn(const std::string &tag, const std::string &msg) const
    {
        warn(tag, std::vector{msg});
    }

    virtual void warn(const std::string &tag, const std::vector<std::string> &lines) const = 0;

    void err(const std::string &msg) const
    {
        err(std::vector{msg});
    }

    virtual void err(const std::vector<std::string> &lines) const = 0;

    virtual void emit_fatal_proximate(const Error &err) const = 0;

    virtual void emit_fatal_step(const Error &err) const = 0;

    virtual void emit_fatal_root(const Error &err) const = 0;

    template <typename T, typename E>
    void fatal(const ChainableResult<T, E> &result) const
    {
        assert_or_panic(!result.has_value(), "result was not of error type");

        const auto &chain = result.chain();
        assert_or_panic(!chain.empty(), "error chain was empty");

        emit_fatal_proximate(*chain.at(0));
        if (chain.size() > 1) {
            // Emit steps for all but the first and last
            auto middle_range = std::ranges::views::drop(chain, 1) | std::ranges::views::take(chain.size() - 2);
            for (const auto &err : middle_range) {
                emit_fatal_step(*err);
            }
            // Emit the last one as root
            emit_fatal_root(*chain.back());
        }
    }
};

} // namespace porytiles2
