#pragma once

#include <memory>
#include <optional>

#include "porytiles2/domain/config/valueobj/incremental_build_mode.hpp"

namespace porytiles2 {

class Config {
  public:
    Config() = default;

    [[nodiscard]] virtual std::unique_ptr<Config> merge(const Config &other) const = 0;

  private:
    std::optional<IncrementalBuildMode> incremental_build_mode_;
};

} // namespace porytiles2
