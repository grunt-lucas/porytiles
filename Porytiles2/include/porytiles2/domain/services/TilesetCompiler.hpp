#pragma once

#include <memory>

#include "porytiles2/domain/model/aggregates/components/PorymapTilesetComponent.hpp"
#include "porytiles2/domain/model/aggregates/components/PorytilesTilesetComponent.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Service interface for compiling a PorytilesTilesetComponent into a
 * PorymapTilesetComponent.
 *
 * @details
 * Service interface for compiling a PorytilesTilesetComponent into a PorymapTilesetComponent.
 */
class TilesetCompiler {
public:
  virtual ~TilesetCompiler() = default;

  virtual Result<std::unique_ptr<PorymapTilesetComponent>>
  compile_primary(const PorytilesTilesetComponent &tileset) = 0;

  virtual Result<std::unique_ptr<PorymapTilesetComponent>>
  compile_secondary(const PorytilesTilesetComponent &tileset,
                    const PorymapTilesetComponent &primary_tileset) = 0;
};

} // namespace porytiles
