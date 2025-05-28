#include "orchestration/pipeline.hpp"

#include <queue>

#include "panic/panic.hpp"

namespace porytiles {

Pipeline::Pipeline(const std::vector<std::shared_ptr<Operation>> &ops) {
    // 1) Map each artifact key to the producer op that generates it
    for (auto &op : ops) {
        for (const auto &output_artifact : op->DeclareOutputs()) {
            const auto &out_key = output_artifact.key();
            if (producers_.contains(out_key)) {
                Panic("Duplicate producer for key: " + out_key);
            }
            producers_.insert({out_key, op.get()});
        }
    }

    // 2) Build adjacency and compute in-degrees
    for (auto &op : ops) {
        const auto inputs = op->DeclareInputs();
        int deps = 0;
        for (const auto &input_artifact : inputs) {
            if (const auto &in_key = input_artifact.key(); producers_.contains(in_key)) {
                auto *producer_op = producers_.at(in_key);
                adj_.at(producer_op).push_back(op.get());
                deps++;
            } else {
                // TODO : resolve at runtime from initial inputs?
                Panic("No producer for key: " + in_key);
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
        for (auto *nbr : adj_[op]) {
            if (--in_degree_[nbr] == 0) {
                q.push(nbr);
            }
        }
    }
    if (sorted_.size() != ops.size()) {
        Panic("Cycle detected in pipeline dependencies");
    }
}

std::expected<AnyMap, std::string> Pipeline::Run() const {
    AnyMap artifacts{};
    for (auto *op : sorted_) {
        // Gather inputs for the operation
        AnyMap inputs{};
        for (auto &input_artifact : op->DeclareInputs()) {
            const auto &key = input_artifact.key();
            const auto val = artifacts.Try<std::any>(key);
            if (!val.has_value()) {
                throw std::runtime_error("Missing artifact: " + key);
            }
            inputs.Put(key, val.value());
        }

        // Execute the operation
        auto result = op->Execute(inputs);
        if (!result.has_value()) {
            return result;
        }

        // Merge outputs
        for (const auto &[key, value] : result.value()) {
            artifacts.Put(key, value);
        }
    }
    return std::expected<AnyMap, std::string>{artifacts};
}

} // namespace porytiles
