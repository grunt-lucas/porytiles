#pragma once

#include <expected>
#include <memory>
#include <unordered_map>
#include <vector>

#include "porytiles2/domain/orchestration/operation.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Manages and executes a collection of operations in dependency order.
 *
 * @details
 * Pipeline builds a dependency graph from a collection of Operation objects based on their input and output
 * declarations. It topologically sorts the operations to determine the correct execution order, ensuring that each
 * operation's inputs are produced before it executes. The pipeline tracks which operations produce each operand and
 * manages the flow of data between operations.
 */
class Pipeline {
  public:
    /**
     * @brief Constructs a pipeline from a collection of operations.
     *
     * @details
     * Analyzes the operations' input and output declarations to build a dependency graph, then topologically sorts the
     * operations to determine execution order. The constructor validates that all required inputs can be satisfied by
     * the outputs of other operations in the pipeline.
     *
     * @param ops Vector of pointers to Operation objects
     */
    explicit Pipeline(const std::vector<Operation *> &ops);

    /**
     * @brief Executes all operations in the pipeline in dependency order.
     *
     * @details
     * Runs each operation in the topologically sorted order, passing outputs from earlier operations as inputs to later
     * operations as specified by their declarations. Propagates any errors that occur during execution.
     *
     * @return Result<void> indicating success or containing an error
     */
    [[nodiscard]] Result<void> run() const;

  private:
    std::unordered_map<std::string, Operation *> producers_;
    std::unordered_map<Operation *, std::vector<Operation *>> adj_;
    std::unordered_map<Operation *, int> in_degree_;
    std::vector<Operation *> sorted_;
};

} // namespace porytiles2
