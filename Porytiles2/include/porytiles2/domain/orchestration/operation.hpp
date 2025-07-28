#pragma once

#include <expected>

#include "gsl/pointers"

#include "porytiles2/domain/orchestration/operand_bundle.hpp"
#include "porytiles2/domain/orchestration/operand_declaration.hpp"
#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

class Operation {
  public:
    virtual ~Operation() = default;

    /// @brief Declares the input operands required by this operation.
    [[nodiscard]] virtual std::vector<OperandDeclaration> declare_inputs() const = 0;

    /// @brief Declares the operands this operation will produce.
    [[nodiscard]] virtual std::vector<OperandDeclaration> declare_outputs() const = 0;

    [[nodiscard]] virtual Result<OperandBundle> apply(const OperandBundle &inputs) {
        const auto declared_inputs = declare_inputs();
        if (!inputs.satisfies_declarations(declared_inputs)) {
            panic(fmt::format("op '{}' declared inputs were not satisfied", name()));
        }
        return execute(inputs);
    }

    [[nodiscard]] const std::string &name() const {
        return name_;
    }

    void set_name(const std::string &name) {
        name_ = name;
    }

  protected:
    [[nodiscard]] virtual Result<OperandBundle> execute(const OperandBundle &inputs) = 0;

  private:
    std::string name_;
};

} // namespace porytiles2
