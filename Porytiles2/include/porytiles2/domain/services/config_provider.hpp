#pragma once

#include "porytiles2/domain/model/valueobj/config/tileset_settings.hpp"

namespace porytiles2 {

class ConfigProvider {
  public:
    virtual ~ConfigProvider() = default;

    virtual TilesetSettings tileset_settings() const = 0;
};

} // namespace porytiles2
