#pragma once

#include <memory>

#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

class AssetGenerator {
  public:
    virtual ~AssetGenerator() = default;

    [[nodiscard]] virtual ChainableResult<std::unique_ptr<PorytilesTilesetComponent>> generate() = 0;
};

} // namespace porytiles2
