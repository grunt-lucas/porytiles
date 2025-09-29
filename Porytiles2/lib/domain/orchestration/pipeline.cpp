#include "porytiles2/domain/orchestration/pipeline.hpp"

#include <queue>

#include "porytiles2/domain/orchestration/operation.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

Pipeline::Pipeline(const std::vector<Operation *> &ops)
{
    // 1) Map each operand key to the producer op that generates it
    for (auto &op : ops) {
        for (const auto &output_operand : op->declare_outputs()) {
            const auto &out_key = output_operand.key();
            if (producers_.contains(out_key)) {
                panic(fmt::format("duplicate producers for key: {}", out_key));
            }
            producers_.insert({out_key, op});
        }
    }

    // 2) Build adjacency and compute in-degrees
    for (auto &op : ops) {
        adj_.try_emplace(op, std::vector<Operation *>{});
    }
    for (auto &op : ops) {
        const auto inputs = op->declare_inputs();
        int deps = 0;
        for (const auto &input_operand : inputs) {
            if (const auto &in_key = input_operand.key(); producers_.contains(in_key)) {
                auto *producer_op = producers_.at(in_key);
                adj_.at(producer_op).push_back(op);
                deps++;
            }
            else {
                panic(fmt::format("operation '{}' depends on non-existent operand: '{}'", op->name(), in_key));
            }
        }
        in_degree_.insert({op, deps});
    }

    // 3) Kahn's algorithm
    std::queue<Operation *> q;
    for (const auto &[op, degree] : in_degree_) {
        if (degree == 0) {
            q.push(op);
        }
    }
    while (!q.empty()) {
        auto *op = q.front();
        q.pop();
        sorted_.push_back(op);
        for (auto *neighbor : adj_.at(op)) {
            if (--in_degree_[neighbor] == 0) {
                q.push(neighbor);
            }
        }
    }
    if (sorted_.size() != ops.size()) {
        panic("cycle detected in pipeline dependencies");
    }
}

ChainableResult<void> Pipeline::run() const
{
    OperandBundle operand_pool{};
    for (auto *op : sorted_) {
        // Gather inputs for the operation
        OperandBundle inputs{};
        for (auto &input_operand : op->declare_inputs()) {
            const auto &key = input_operand.key();
            const auto val = operand_pool.get(key);
            if (!val.has_value()) {
                panic(fmt::format("operation '{}' missing input operand: {}", op->name(), key));
            }
            inputs.put(key, val.value());
        }

        // Execute the operation
        auto result = op->apply(inputs);
        if (!result.has_value()) {
            return ChainableResult<void>::chain_together(
                FormattableError{fmt::format("operation '{}' failed", op->name())}, result);
        }

        // Merge outputs
        auto output_bundle = result.value();
        for (const auto &[key, value] : output_bundle) {
            if (operand_pool.contains(key)) {
                panic(fmt::format("op '{}' output operand '{}' already present in operand pool", op->name(), key));
            }
            operand_pool.put(key, value);
        }
    }
    return {};
}

} // namespace porytiles2
