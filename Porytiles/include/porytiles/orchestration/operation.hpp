#pragma once

#include <typeindex>

#include <gsl/pointers>

#include "../diagnostics/diagnostic_engine.hpp"
#include "../templates/any_map.hpp"
#include "../templates/result.hpp"

namespace porytiles {

class ArtifactMetadata {
  public:
    ArtifactMetadata(std::string k, const std::type_index t) : key_{std::move(k)}, expected_type_{t} {}

    [[nodiscard]] const std::string &key() const {
        return key_;
    }

    [[nodiscard]] const std::type_index &expected_type() const {
        return expected_type_;
    }

  private:
    std::string key_;
    std::type_index expected_type_;
};

class Operation {
  public:
    virtual ~Operation() = default;

    explicit Operation(const gsl::not_null<DiagEngine *> diag) : diag_{diag} {}

    /// @brief Declares the input artifacts required by this operation.
    [[nodiscard]] virtual std::vector<ArtifactMetadata> DeclareInputs() const = 0;

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] virtual std::vector<ArtifactMetadata> DeclareOutputs() const = 0;

    [[nodiscard]] virtual Result<AnyMap, BinaryStatus> Execute(const AnyMap &inputs) const = 0;

  protected:
    gsl::not_null<DiagEngine *> diag_;
};

} // namespace porytiles
