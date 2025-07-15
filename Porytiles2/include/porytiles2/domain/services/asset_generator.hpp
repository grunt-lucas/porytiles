#pragma once

#include <memory>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

class AssetGenerator {
  public:
    virtual ~AssetGenerator() = default;

    [[nodiscard]] virtual Result<std::unique_ptr<PorytilesTilesetComponent>> generate() = 0;
};

} // namespace porytiles2
