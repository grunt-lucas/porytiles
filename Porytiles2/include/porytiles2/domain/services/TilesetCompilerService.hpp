#pragma once

#include <memory>

#include "porytiles2/domain/aggregates/PorymapTileset.hpp"
#include "porytiles2/domain/aggregates/PorytilesTileset.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief A domain service that provides functionality to compile
 * PorytilesTileset to PorymapTileset.
 */
class TilesetCompilerService {
public:
  virtual ~TilesetCompilerService() = default;

  /**
   * @brief Compiles a primary PorytilesTileset to a PorymapTileset.
   *
   * @param tileset The PorytilesTileset to compile.
   * @return A pointer to the resulting PorymapTileset or a description of the
   * compilation error.
   */
  virtual Result<std::unique_ptr<PorymapTileset>>
  CompilePrimary(const PorytilesTileset &tileset) = 0;

  /**
   * @brief Compiles a secondary PorytilesTileset to a PorymapTileset, using a
   * given paired primary PorymapTileset.
   *
   * @param tileset The PorytilesTileset to compile.
   * @param primary_tileset The paired primary PorymapTileset.
   * @return A pointer to the resulting PorymapTileset or a description of the
   * compilation error.
   */
  virtual Result<std::unique_ptr<PorymapTileset>>
  CompileSecondary(const PorytilesTileset &tileset,
                   const PorymapTileset &primary_tileset) = 0;
};

} // namespace porytiles
