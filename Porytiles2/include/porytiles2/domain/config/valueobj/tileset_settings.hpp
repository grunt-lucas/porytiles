#pragma once

#include <utility>

#include "porytiles2/domain/config/valueobj/fieldmap_settings.hpp"

namespace porytiles2 {

class TilesetSettings {
  public:
    explicit TilesetSettings(FieldmapSettings fieldmap_settings) : fieldmap_settings_{std::move(fieldmap_settings)} {}

    [[nodiscard]] const FieldmapSettings &fieldmap_settings() const {
        return fieldmap_settings_;
    }

  private:
    FieldmapSettings fieldmap_settings_;
};

} // namespace porytiles2
