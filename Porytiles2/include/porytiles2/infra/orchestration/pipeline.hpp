#pragma once

#include <expected>
#include <memory>
#include <unordered_map>
#include <vector>

#include "porytiles2/infra/orchestration/operation.hpp"

namespace porytiles {

class Pipeline {
  public:
    explicit Pipeline(const std::vector<std::shared_ptr<Operation>> &ops);

    [[nodiscard]] std::expected<AnyMap, std::string> Run() const;

  private:
    std::unordered_map<std::string, Operation *> producers_;
    std::unordered_map<Operation *, std::vector<Operation *>> adj_;
    std::unordered_map<Operation *, int> in_degree_;
    std::vector<Operation *> sorted_;
};

} // namespace porytiles
