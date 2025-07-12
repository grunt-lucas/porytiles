#include "porytiles2/infra/orchestration/pipeline.hpp"

#include <queue>

#include "porytiles2/infra/orchestration/operation.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

Pipeline::Pipeline(const std::vector<std::shared_ptr<Operation>> &ops) {
  // 1) Map each artifact key to the producer op that generates it
  for (auto &op : ops) {
    for (const auto &output_artifact : op->declare_outputs()) {
      const auto &out_key = output_artifact.key();
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
    for (const auto &input_artifact : inputs) {
      if (const auto &in_key = input_artifact.key(); producers_.contains(in_key)) {
        auto *producer_op = producers_.at(in_key);
        adj_.at(producer_op).push_back(op.get());
        deps++;
      } else {
        panic(fmt::format("operation '{}' depends on non-existent artifact: '{}'", op->name(),
                          in_key));
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

Result<void> Pipeline::run() {
  ArtifactBundle artifacts{};
  for (auto *op : sorted_) {
    // Gather inputs for the operation
    ArtifactBundle inputs{};
    for (auto &input_artifact : op->declare_inputs()) {
      const auto &key = input_artifact.key();
      const auto val = artifacts.try_get_any(key);
      if (!val.has_value()) {
        panic(fmt::format("operation '{}' missing input artifact: {}", op->name(), key));
      }
      inputs.put(key, val.value());
    }

    // Execute the operation
    auto result = op->execute(inputs);
    if (!result.has_value()) {
      return std::unexpected{fmt::format("operation '{}' failed: {}", op->name(), result.error())};
    }

    // Merge outputs
    for (auto outputs_map = result.value(); const auto &[key, value] : outputs_map) {
      if (artifacts.contains(key)) {
        panic("duplicate output artifact: " + key);
      }
      artifacts.put(key, value);
    }
  }
  leftover_artifacts_ = artifacts;
  return {};
}

} // namespace porytiles2
