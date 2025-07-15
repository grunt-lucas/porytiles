#pragma once

#include <memory>

#include "porytiles2/domain/model/aggregates/components/porymap_tileset_component.hpp"
#include "porytiles2/domain/model/aggregates/components/porytiles_tileset_component.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Service interface for decompiling a primary PorymapTilesetComponent into an equivalent
 * PorytilesTilesetComponent.
 */
class PrimaryTilesetDecompiler {
  public:
    virtual ~PrimaryTilesetDecompiler() = default;

    virtual Result<std::unique_ptr<PorytilesTilesetComponent>> decompile(const PorymapTilesetComponent &tileset) = 0;
};

} // namespace porytiles2
