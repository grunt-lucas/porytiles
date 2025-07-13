#pragma once

#include <memory>

#include "porytiles2/domain/model/aggregates/components/porymap_tileset_component.hpp"
#include "porytiles2/domain/model/aggregates/components/porytiles_tileset_component.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Service interface for compiling a primary PorytilesTilesetComponent into a PorymapTilesetComponent.
 */
class PrimaryTilesetCompiler {
  public:
    virtual ~PrimaryTilesetCompiler() = default;

    virtual Result<std::unique_ptr<PorymapTilesetComponent>> compile(const PorytilesTilesetComponent &tileset) = 0;
};

} // namespace porytiles2
