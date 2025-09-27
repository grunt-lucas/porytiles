#pragma once

#include <string>
#include <vector>

#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/error.hpp"

namespace porytiles2 {

class UserDiagnosticsStderrImpl final : public UserDiagnostics {
  public:
    void note(const std::vector<std::string> &lines) const override;

    void warn_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    void warn(const std::string &tag, const std::vector<std::string> &lines) const override;

    void err(const std::vector<std::string> &lines) const override;

    void emit_fatal_proximate(const Error &err) const override;

    void emit_fatal_step(const Error &err) const override;

    void emit_fatal_root(const Error &err) const override;
};

} // namespace porytiles2
