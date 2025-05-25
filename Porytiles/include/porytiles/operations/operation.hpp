#pragma once

#include <typeindex>

#include <gsl/pointers>

#include "../diagnostics/diagnostic_engine.hpp"
#include "../templates/any_map.hpp"
#include "../templates/result.hpp"

namespace porytiles {

class Artifact {
  public:
    Artifact(std::string k, const std::type_index t) : key_{std::move(k)}, expected_type_{t} {}

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

    /// @brief Declares the input dependencies required by this operation.
    ///
    /// @details
    /// Subclasses must implement this method to define their input contract.
    /// An empty vector indicates
    /// that the operation has no specific input dependencies.
    [[nodiscard]] virtual std::vector<Artifact> DeclareDependencies() const = 0;

    [[nodiscard]] virtual Result<AnyMap, BinaryStatus> Run(const AnyMap &inputs) const = 0;

    [[nodiscard]] Result<AnyMap, BinaryStatus> Execute(const AnyMap &inputs) const;

  protected:
    gsl::not_null<DiagEngine *> diag_;
};

} // namespace porytiles
