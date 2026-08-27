#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles/utilities/result/error.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Silent diagnostics implementation that suppresses all output.
///
/// @details
/// NullUserDiagnostics implements all UserDiagnostics methods as no-ops, producing no output
/// whatsoever. This is useful in contexts where diagnostic output would be inappropriate,
/// such as shell completion scripts that must only output completion candidates.
///
/// Shell completion routines call helper commands that parse project files and produce
/// completions. Any stderr output (warnings, errors, etc.) would corrupt the completion
/// results and break the shell's tab-completion functionality.
class NullUserDiagnostics final : public UserDiagnostics {
  public:
    explicit NullUserDiagnostics(gsl::not_null<const TextFormatter *> format) : UserDiagnostics{format} {}

    void remark(const std::string & /*tag*/, const std::vector<std::string> & /*lines*/) const override
    {
        // Silent: no output
    }

    void warning(const std::string & /*tag*/, const std::vector<std::string> & /*lines*/) const override
    {
        // Silent: no output
    }

    void error(const std::string & /*tag*/, const std::vector<std::string> & /*lines*/) const override
    {
        // Silent: no output
    }

    void remark_note(const std::string & /*tag*/, const std::vector<std::string> & /*lines*/) const override
    {
        // Silent: no output
    }

    void warning_note(const std::string & /*tag*/, const std::vector<std::string> & /*lines*/) const override
    {
        // Silent: no output
    }

    void error_note(const std::string & /*tag*/, const std::vector<std::string> & /*lines*/) const override
    {
        // Silent: no output
    }

    void emit_fatal_proximate(const Error & /*err*/) const override
    {
        // Silent: no output
    }

    void emit_fatal_step(const Error & /*err*/) const override
    {
        // Silent: no output
    }

    void emit_fatal_root(const Error & /*err*/) const override
    {
        // Silent: no output
    }
};

} // namespace porytiles
