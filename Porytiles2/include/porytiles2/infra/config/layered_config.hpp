#pragma once

#include <optional>
#include <vector>

#include "porytiles2/domain/config/config.hpp"
#include "porytiles2/domain/config/valueobj/incremental_build_mode.hpp"

namespace porytiles2 {

template <typename T>
struct LayeredValue {
    std::vector<T> values_;
    std::vector<std::string> source_data_;
};

/**
 * @brief Represents a layered Config possibly built from multiple sources.
 */
class LayeredConfig final : public Config {
  public:
    [[nodiscard]] TilesetSettings tileset_settings(const std::string &tileset_name) const override;

    [[nodiscard]] IncrementalBuildMode incremental_build_mode() const override;

  private:
    TilesetSettings tileset_settings_;
    LayeredValue<IncrementalBuildMode> incremental_build_mode_;
};

} // namespace porytiles2
