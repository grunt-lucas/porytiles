#pragma once

#include <memory>

#include "porytiles2/domain/model/aggregates/components/PorymapTilesetComponent.hpp"
#include "porytiles2/domain/model/aggregates/components/PorytilesTilesetComponent.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief A domain service that provides functionality to compile a
 * PorytilesTilesetComponent to a PorymapTilesetComponent.
 */
class TilesetCompilerService {
public:
  virtual ~TilesetCompilerService() = default;

  virtual Result<std::unique_ptr<PorymapTilesetComponent>>
  CompilePrimary(const PorytilesTilesetComponent &tileset) = 0;

  virtual Result<std::unique_ptr<PorymapTilesetComponent>>
  CompileSecondary(const PorytilesTilesetComponent &tileset,
                   const PorymapTilesetComponent &primary_tileset) = 0;
};

} // namespace porytiles
