#pragma once

#include <memory>

#include "porytiles2/domain/model/porymap_tileset_component.hpp"
#include "porytiles2/domain/model/porytiles_tileset_component.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Service interface for compiling a primary PorytilesTilesetComponent into a PorymapTilesetComponent.
 */
class PrimaryTilesetCompiler {
  public:
    virtual ~PrimaryTilesetCompiler() = default;

    virtual Result<std::unique_ptr<PorymapTilesetComponent>> compile(const PorytilesTilesetComponent &tileset) = 0;

    /**
     * @brief Compiles the given PorytilesTilesetComponent into a PorymapTilesetComponent using a contextual
     * PorymapTilesetComponent as the base for the incremental compilation.
     *
     * @details
     * TODO: explain the PorymapTilesetComponent contextual base
     *
     * @param tileset The PorytilesTilesetComponent to compile
     * @param context The PorymapTilesetComponent to use as the contextual base
     * @returns The compiled PorymapTilesetComponent
     */
    virtual Result<std::unique_ptr<PorymapTilesetComponent>>
    compile_incremental(const PorytilesTilesetComponent &tileset, const PorymapTilesetComponent &context) = 0;
};

} // namespace porytiles2
