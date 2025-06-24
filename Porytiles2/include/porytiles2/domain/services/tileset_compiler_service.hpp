#pragma once

#include <memory>

#include <porytiles2/domain/aggregates/porymap_tileset.hpp>
#include <porytiles2/domain/aggregates/porytiles_tileset.hpp>
#include <porytiles2/templates/result.hpp>

namespace porytiles {

/**
 * @brief A domain service that compiles a PorytilesTileset into a PorymapTileset.
 */
class TilesetCompilerService {
  public:
    virtual ~TilesetCompilerService() = default;

    /**
     * @brief Compiles a primary PorytilesTileset into an equivalent PorymapTileset.
     *
     * @param porytilesTileset The PorytilesTileset to compile.
     * @return A pointer to the resulting PorymapTileset or a description of the compilation error.
     */
    virtual Result<std::unique_ptr<PorymapTileset>> CompilePrimary(const PorytilesTileset &porytilesTileset) = 0;
};

} // namespace porytiles
