#pragma once

#include <expected>

#include "gsl/pointers"

#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/infra/orchestration/artifact_bundle.hpp"
#include "porytiles2/infra/orchestration/artifact_declaration.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

class Operation {
  public:
    virtual ~Operation() = default;

    explicit Operation(const gsl::not_null<DiagEngine *> diag) : diag_{diag} {}

    /// @brief Declares the input artifacts required by this operation.
    [[nodiscard]] virtual std::vector<ArtifactDeclaration> declare_inputs() const = 0;

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] virtual std::vector<ArtifactDeclaration> declare_outputs() const = 0;

    [[nodiscard]] virtual Result<ArtifactBundle> execute(const ArtifactBundle &inputs) = 0;

    [[nodiscard]] const DiagEngine &diag() const {
        return *diag_;
    }

    [[nodiscard]] const std::string &name() const {
        return name_;
    }

    void set_name(const std::string &name) {
        name_ = name;
    }

  private:
    DiagEngine *diag_;
    std::string name_;
};

} // namespace porytiles2
