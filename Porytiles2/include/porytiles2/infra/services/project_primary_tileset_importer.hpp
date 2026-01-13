#pragma once

#include "porytiles2/domain/services/primary_tileset_importer.hpp"

namespace porytiles2 {

/**
 * @brief Infra-layer implementation of PrimaryTilesetImporter for pokeemerald project structure.
 *
 * @details
 * This class provides the pokeemerald-specific implementation for importing vanilla tilesets. It reads tileset
 * artifacts from the standard pokeemerald project directory structure:
 *
 * - `src/data/tilesets/headers.h` - Tileset struct declarations with INCBIN variable references
 * - `src/data/tilesets/graphics.h` - INCBIN declarations for tiles and palettes
 * - `src/data/tilesets/metatiles.h` - INCBIN declarations for metatiles and attributes
 * - `src/tileset_anims.c` - Animation callback implementations and frame INCBIN declarations
 * - `data/tilesets/{primary,secondary}/{name}/` - Actual PNG/binary assets
 *
 * The import process parses C header files to discover asset locations (which may be scattered across different paths
 * due to INCBIN flexibility), then reads and populates a PorymapTilesetComponent with all the data needed for
 * decompilation.
 *
 * @note This class handles the "discovery chaos" of vanilla pokeemerald projects where asset paths are not
 * standardized. After import, Porytiles places all assets at deterministic paths.
 *
 * @see PrimaryTilesetImporter for the domain-layer base class and import workflow
 * @see ProjectVanillaAnimImporter for the animation import helper used by this class
 * @see project_structure_refactoring_plan.md Section 7 for the full import workflow specification
 */
class ProjectPrimaryTilesetImporter : public PrimaryTilesetImporter {
  public:
    /**
     * @brief Constructs a ProjectPrimaryTilesetImporter with required dependencies.
     *
     * @param config Domain configuration containing tileset parameters and paths
     * @param format Text formatter for error message styling
     * @param diag User diagnostics for warnings and informational messages
     * @param tile_printer Tile visualization service for diagnostic output
     * @param pal_printer Palette visualization service for diagnostic output
     */
    explicit ProjectPrimaryTilesetImporter(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : config_{config}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    /**
     * @brief Reads vanilla Porymap artifacts from a pokeemerald project into a PorymapTilesetComponent.
     *
     * @details
     * This implementation:
     *
     * 1. Parses `src/data/tilesets/headers.h` to get tileset metadata (isSecondary, INCBIN variable names)
     * 2. Parses `src/data/tilesets/graphics.h` and `metatiles.h` to resolve INCBIN paths to actual files
     * 3. Reads `tiles.png`, palettes, `metatiles.bin`, and `metatile_attributes.bin` from discovered paths
     * 4. Uses ProjectVanillaAnimImporter to read animation frames from `tileset_anims.c` INCBIN declarations
     *
     * @param tileset_name The name of the tileset to import (e.g., "gTileset_General")
     * @pre Tileset entry must exist in src/data/tilesets/headers.h
     * @pre All INCBIN-referenced files must exist at their declared paths
     * @return A ChainableResult containing a unique_ptr to the populated PorymapTilesetComponent
     */
    [[nodiscard]] ChainableResult<std::unique_ptr<PorymapTilesetComponent>>
    import_porymap_component_from_vanilla(const std::string &tileset_name) const override;

  private:
    const DomainConfig *config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles2
