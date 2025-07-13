#include "porytiles2/infra/orchestration/pipeline.hpp"

#include <queue>

#include "porytiles2/infra/orchestration/operation.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

Pipeline::Pipeline(const std::vector<std::shared_ptr<Operation>> &ops) {
    // 1) Map each operandt key to the producer op that generates it
    for (auto &op : ops) {
        for (const auto &output_operandt : op->declare_outputs()) {
            const auto &out_key = output_operandt.key();
            if (producers_.contains(out_key)) {
                panic("duplicate producers for key: " + out_key);
            }
            producers_.insert({out_key, op.get()});
        }
    }

    // 2) Build adjacency and compute in-degrees
    for (auto &op : ops) {
        adj_.try_emplace(op.get(), std::vector<Operation *>{});
    }
    for (auto &op : ops) {
        const auto inputs = op->declare_inputs();
        int deps = 0;
        for (const auto &input_operandt : inputs) {
            if (const auto &in_key = input_operandt.key(); producers_.contains(in_key)) {
                auto *producer_op = producers_.at(in_key);
                adj_.at(producer_op).push_back(op.get());
                deps++;
            } else {
                panic(fmt::format("operation '{}' depends on non-existent operandt: '{}'", op->name(), in_key));
            }
        }
        in_degree_.insert({op.get(), deps});
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

Result<void> Pipeline::run() const {
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
        auto result = op->execute(inputs);
        if (!result.has_value()) {
            return std::unexpected{fmt::format("operation '{}' failed: {}", op->name(), result.error())};
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
