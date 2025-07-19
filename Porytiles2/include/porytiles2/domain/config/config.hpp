#pragma once

#include <memory>
#include <optional>

#include "porytiles2/domain/config/valueobj/incremental_build_mode.hpp"

namespace porytiles2 {

class Config final {
  public:
    Config() = default;

    [[nodiscard]] std::optional<IncrementalBuildMode> incremental_build_mode() const;

  private:
    std::optional<IncrementalBuildMode> incremental_build_mode_;
};

} // namespace porytiles2
