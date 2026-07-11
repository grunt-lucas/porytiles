#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Domain service interface for importing vanilla (non-Porytiles-managed) tilesets into a
/// PorymapTilesetComponent.
class PrimaryTilesetImporter {
  public:
    virtual ~PrimaryTilesetImporter() = default;

    /// @brief Imports vanilla Porymap assets for the given tileset into a PorymapTilesetComponent.
    ///
    /// @details
    /// This is the Template Method hook that infra-layer implementations override to provide backing-store-specific
    /// asset loading. The method reads existing Porymap tileset artifacts from a "vanilla" tileset that is not yet
    /// managed by Porytiles (the "first-time import" case).
    ///
    /// For vanilla tilesets, asset locations are scattered (determined by INCBIN declarations in C headers). This
    /// method handles that "discovery chaos" by parsing the relevant C files to find actual file paths.
    ///
    /// The returned PorymapTilesetComponent should contain:
    /// - tile data (the indexed-color tileset image, tiles.png)
    /// - palette data (00.pal through 15.pal)
    /// - raw metatile TilemapEntry (metatiles.bin)
    /// - metatile behavior/terrain attributes (metatile_attributes.bin)
    /// - animations (anim/ folder contents)
    ///
    /// @param tileset_name The name of the tileset to import (e.g., "gTileset_General")
    /// @pre Tileset entry must exist in src/data/tilesets/headers.h
    /// @pre Animation arrays in tileset_anims.c must follow gTilesetAnims_{TilesetName}_{AnimName} naming convention
    /// @return A ChainableResult containing a unique_ptr to the populated PorymapTilesetComponent on success
    /// @post The returned component has tiles_png, pals, metatile_entries, and metatile_attributes populated
    /// @post Animation frames are populated as Animation<IndexPixel> (key frames NOT extracted by this method)
    ///
    /// @see ProjectPrimaryTilesetImporter for the pokeemerald project-based implementation
    /// @see ProjectVanillaAnimImporter for animation import details
    [[nodiscard]] virtual ChainableResult<std::unique_ptr<PorymapTilesetComponent>>
    import_porymap_component_from_vanilla(const std::string &tileset_name) const = 0;
};

} // namespace porytiles
