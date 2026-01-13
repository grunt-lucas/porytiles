/**
 * @file project_primary_tileset_importer.cpp
 *
 * @brief Implementation of ProjectPrimaryTilesetImporter for pokeemerald project structure.
 *
 * @details
 * This file implements the pokeemerald-specific logic for importing vanilla tilesets. The implementation is currently
 * a stub that returns an empty PorymapTilesetComponent. See the TODOs below for what needs to be implemented.
 *
 * @see project_structure_refactoring_plan.md Phase 6 for the implementation roadmap
 */

#include "porytiles2/infra/services/project_primary_tileset_importer.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<PorymapTilesetComponent>>
ProjectPrimaryTilesetImporter::import_porymap_component_from_vanilla(const std::string &tileset_name) const
{
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();

    /*
     * TODO (Phase 5/6): Read vanilla Porymap artifacts (metatiles.bin, metatile_attributes.bin, tiles.png, palettes)
     * into PorymapTilesetComponent.
     *
     * Implementation steps:
     * 1. Use ProjectTilesetMetadataProvider to get tileset metadata from headers.h (isSecondary, variable names)
     * 2. Parse graphics.h and metatiles.h to resolve INCBIN variable names to actual file paths (may also need to check
     * src/graphics.c, gTileset_General stores tiles and pals there instead of standard location)
     * 3. Read tiles.png using PngIndexedImageLoader
     * 4. Read palettes (00.pal through 15.pal) from discovered palette directory
     * 5. Read metatiles.bin and metatile_attributes.bin from discovered paths
     * 6. Populate porymap_component with all loaded data
     *
     * Since reading metatiles.bin, metatile_attributes.bin, tiles.png, and palettes are operations that need to be
     * performed here and in TilesetRepo::load, we should have some reusable code that can handle this for us. If you
     * look in ProjectTilesetArtifactReader, there's already code for reading metatiles.bin, metatile_attributes.bin,
     * etc. Let's move it somewhere shareable. Perhaps into headers in infra/algorithms?
     */

    /*
     * TODO: Use ProjectVanillaAnimImporter to read vanilla animations into PorymapTilesetComponent.
     *
     * Implementation steps:
     * 1. Create ProjectVanillaAnimImporter instance with project_root, format_, diag_
     * 2. Call import_animations(tileset_name) to get map of Animation<IndexPixel>
     * 3. Add each animation to porymap_component via add_anim()
     *
     * Note: The returned animations will have frames populated but NO key frames.
     * Key frame extraction is done later by AnimationDecompiler in the domain layer.
     */

    return porymap_component;
}

} // namespace porytiles2
