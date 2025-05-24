#pragma once

#include <gsl/pointers>

#include "../diagnostics/diagnostic_engine.hpp"
#include "../templates/any_map.hpp"
#include "../templates/result.hpp"

namespace porytiles {

class Operation {
    gsl::not_null<DiagEngine *> diag;

  public:
    virtual ~Operation() = default;

    explicit Operation(const gsl::not_null<DiagEngine *> diag) : diag{diag} {}

    virtual Result<AnyMap, BinaryStatus> execute(const AnyMap &inputs) const = 0;
};

} // namespace porytiles
