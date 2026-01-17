#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Domain service for creating new primary tilesets from scratch.
 *
 * @details
 * This service creates a blank PorytilesTilesetComponent with empty/transparent layers and no animations. The
 * resulting component is ready to be compiled by PrimaryTilesetCompiler to produce minimal valid Porymap assets.
 *
 * Unlike import workflows that read existing assets, this creates a tileset from nothing. The generated component
 * will have empty layer images (0x0 dimensions), no metatile attributes, no palettes, and no animations.
 *
 * This is a concrete class (not a virtual interface) since all creation logic uses pure domain objects without any
 * I/O dependencies.
 */
class PrimaryTilesetCreator {
  public:
    PrimaryTilesetCreator() = default;

    /**
     * @brief Creates a new blank PorytilesTilesetComponent.
     *
     * @details
     * The resulting component has:
     * - Empty layer images (bottom, middle, top) with 0x0 dimensions
     * - No metatile attributes
     * - No palettes
     * - No animations
     *
     * @param tileset_name The name of the tileset being created (for error messages)
     * @return A new PorytilesTilesetComponent ready for compilation
     */
    [[nodiscard]] ChainableResult<std::unique_ptr<PorytilesTilesetComponent>>
    create_porytiles_component(const std::string &tileset_name) const;
};

} // namespace porytiles2
