#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Domain service for importing vanilla (non-Porytiles-managed) tilesets into Porytiles-managed state.
 *
 * @details
 * This class orchestrates the one-time import workflow that converts a vanilla pokeemerald tileset into a
 * Porytiles-managed tileset.
 *
 * The class uses the Template Method pattern: `import()` provides the algorithm skeleton while
 * `import_porymap_component_from_vanilla()` is a pure virtual hook for infra-layer implementations.
 *
 * @note This service handles the "discovery chaos" of vanilla imports where assets are at scattered INCBIN paths.
 *       After import completes, all artifact locations become deterministic.
 *
 * @see ProjectPrimaryTilesetImporter for the pokeemerald project-based implementation
 */
class PrimaryTilesetImporter {
  public:
    virtual ~PrimaryTilesetImporter() = default;

    explicit PrimaryTilesetImporter(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    /**
     * @brief Imports a vanilla (non-Porytiles-managed) tileset into a Porytiles Tileset aggregate root.
     *
     * @details
     * This is the main entry point for the import workflow. The method delegates to
     * `import_porymap_component_from_vanilla()` to read vanilla Porymap artifacts from the backing store before
     * completing the operation within the domain layer.
     *
     * This design keeps domain logic (decompilation, layer conversion) in the domain layer while delegating I/O
     * operations (reading PNGs, parsing binary files) to infra-layer implementations.
     *
     * @param tileset_name The name of the tileset to import (e.g., "gTileset_General")
     * @pre Tileset entry must exist in src/data/tilesets/headers.h
     * @pre Tileset must not already be Porytiles-managed (original_artifacts.json must not exist)
     * @return A ChainableResult containing a unique_ptr to the fully-populated Tileset on success
     * @post The returned Tileset has both PorymapTilesetComponent and PorytilesTilesetComponent populated
     *
     * @see import_porymap_component_from_vanilla() for the implementation-specific asset loading hook
     */
    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> import(const std::string &tileset_name) const;

    /**
     * @brief Imports vanilla Porymap assets for the given tileset into a PorymapTilesetComponent.
     *
     * @details
     * This is the Template Method hook that infra-layer implementations override to provide backing-store-specific
     * asset loading. The method reads existing Porymap tileset artifacts from a "vanilla" tileset that is not yet
     * managed by Porytiles (the "first-time import" case).
     *
     * For vanilla tilesets, asset locations are scattered (determined by INCBIN declarations in C headers). This method
     * handles that "discovery chaos" by parsing the relevant C files to find actual file paths.
     *
     * The returned PorymapTilesetComponent should contain:
     * - tiles.png (the indexed-color tileset image)
     * - Palettes (00.pal through 15.pal)
     * - metatiles.bin (raw metatile tilemap entries)
     * - metatile_attributes.bin (metatile behavior/terrain attributes)
     * - Animations (if the tileset has any, via ProjectVanillaAnimImporter)
     *
     * @param tileset_name The name of the tileset to import (e.g., "gTileset_General")
     * @pre Tileset entry must exist in src/data/tilesets/headers.h
     * @pre Animation arrays in tileset_anims.c must follow gTilesetAnims_{TilesetName}_{AnimName} naming convention
     * @return A ChainableResult containing a unique_ptr to the populated PorymapTilesetComponent on success
     * @post The returned component has tiles_png, pals, metatile_entries, and metatile_attributes populated
     * @post Animation frames are populated as Animation<IndexPixel> (key frames NOT extracted by this method)
     *
     * @see ProjectPrimaryTilesetImporter for the pokeemerald project-based implementation
     * @see ProjectVanillaAnimImporter for animation import details
     */
    [[nodiscard]] virtual ChainableResult<std::unique_ptr<PorymapTilesetComponent>>
    import_porymap_component_from_vanilla(const std::string &tileset_name) const = 0;

  private:
    const DomainConfig *config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles2
