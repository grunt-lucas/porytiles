#pragma once

#include <expected>
#include <vector>

#include "porytiles2/domain/orchestration/operand_bundle.hpp"
#include "porytiles2/domain/orchestration/operand_declaration.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"

namespace porytiles2 {

/**
 * @brief Abstract base class for operations in a processing pipeline.
 *
 * @details
 * Operation represents a single processing unit in a pipeline that takes input operands and produces output operands.
 * Each operation must declare its expected inputs and outputs, and implement the execute method to perform its specific
 * processing logic. The apply method validates inputs against declared requirements before execution.
 */
class Operation {
  public:
    virtual ~Operation() = default;

    /**
     * @brief Declares the input operands required by this operation.
     *
     * @return Vector of OperandDeclaration objects describing required inputs
     */
    [[nodiscard]] virtual std::vector<OperandDeclaration> declare_inputs() const = 0;

    /**
     * @brief Declares the output operands produced by this operation.
     *
     * @return Vector of OperandDeclaration objects describing produced outputs
     */
    [[nodiscard]] virtual std::vector<OperandDeclaration> declare_outputs() const = 0;

    /**
     * @brief Applies this operation to the given input operands.
     *
     * @details
     * Validates that the provided inputs satisfy the declared input requirements before delegating to the execute
     * method. Panics if inputs are invalid.
     *
     * @param inputs Bundle of input operands to process
     * @return ChainableResult containing the output operand bundle or an error
     */
    [[nodiscard]] virtual ChainableResult<OperandBundle> apply(const OperandBundle &inputs)
    {
        const auto declared_inputs = declare_inputs();
        if (!inputs.satisfies_declarations(declared_inputs)) {
            panic(fmt::format("op '{}' declared inputs were not satisfied", name()));
        }
        return execute(inputs);
    }

    /**
     * @brief Gets the name of this operation.
     *
     * @return Const reference to the operation name
     */
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Sets the name of this operation.
     *
     * @param name The new name for this operation
     */
    void set_name(const std::string &name)
    {
        name_ = name;
    }

  protected:
    /**
     * @brief Executes the operation's processing logic.
     *
     * @details
     * Subclasses must implement this method to define their specific processing behavior. This method is called by
     * apply after input validation has passed.
     *
     * @param inputs Validated bundle of input operands
     * @return ChainableResult containing the output operand bundle or an error
     */
    [[nodiscard]] virtual ChainableResult<OperandBundle> execute(const OperandBundle &inputs) = 0;

  private:
    std::string name_;
};

} // namespace porytiles2
