#include "porytiles2/infra/services/project_primary_tileset_importer.hpp"

#include <format>

#include "porytiles2/infra/algorithms/porymap_artifact_parsers.hpp"
#include "porytiles2/infra/services/project_vanilla_anim_importer.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<PorymapTilesetComponent>>
ProjectPrimaryTilesetImporter::import_porymap_component_from_vanilla(const std::string &tileset_name) const
{
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();

    // Step 1: Get artifact paths from metadata provider
    // This resolves INCBIN variable names to actual file paths by parsing graphics.h, metatiles.h, etc.
    PT_TRY_ASSIGN_CHAIN_ERR(
        artifact_paths,
        metadata_provider_->artifact_paths_for(tileset_name),
        format_->format("failed to get artifact paths for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::unique_ptr<PorymapTilesetComponent>);

    // Step 2: Parse metatiles.bin
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatile_entries,
        parse_metatiles_bin(project_root_ / artifact_paths.metatiles_path()),
        "failed to parse metatiles.bin",
        std::unique_ptr<PorymapTilesetComponent>);

    for (auto &entry : metatile_entries) {
        porymap_component->push_back_tilemap_entry(std::move(entry));
    }

    // Step 3: Parse metatile_attributes.bin
    PT_TRY_ASSIGN_CHAIN_ERR(
        attributes,
        parse_emerald_metatile_attributes(project_root_ / artifact_paths.metatile_attributes_path()),
        "failed to parse metatile_attributes.bin",
        std::unique_ptr<PorymapTilesetComponent>);

    for (auto &attr : attributes) {
        porymap_component->push_back_attribute(std::move(attr));
    }

    // Step 4: Load tiles.png (strip all extensions like .4bpp.smol, then add .png)
    // TODO: we should put this extension stripping logic into a utilities header
    auto tiles_dir = artifact_paths.tiles_path().parent_path();
    auto tiles_filename = artifact_paths.tiles_path().filename();
    while (!tiles_filename.extension().empty()) {
        tiles_filename = tiles_filename.stem();
    }
    auto tiles_png_path = tiles_dir / (tiles_filename.string() + ".png");

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_image,
        load_indexed_png(project_root_ / tiles_png_path, *png_loader_),
        "failed to load tiles.png",
        std::unique_ptr<PorymapTilesetComponent>);

    porymap_component->tiles_png(*tiles_image);

    // Step 5: Load palettes from the discovered palette paths
    const auto &palette_paths = artifact_paths.palette_paths();
    for (std::size_t i = 0; i < palette_paths.size() && i < pal::num_pals; ++i) {
        // Convert .gbapal path to .pal by looking in the palettes directory
        auto pal_dir = artifact_paths.palettes_dir();
        auto pal_path = project_root_ / pal_dir / pal_filename(i);

        PT_TRY_ASSIGN_CHAIN_ERR(
            palette,
            load_porymap_palette(pal_path, *pal_loader_),
            format_->format("failed to load palette {}", FormatParam{i}),
            std::unique_ptr<PorymapTilesetComponent>);

        porymap_component->set_pal(i, std::move(palette));
    }

    /*
     * TODO: somehow, when we run through this codepath, we're losing the frame dimensions of the original vanilla
     * animations. We need to figure out where that's happening and fix it. I suspect anim_importer.import_animations
     * isn't preserving them.
     */

    // Step 6: Import animations using ProjectVanillaAnimImporter
    // Animation import failure is non-fatal - tileset may not have animations
    ProjectVanillaAnimImporter anim_importer{project_root_, format_, diag_};

    auto anims_result = anim_importer.import_animations(tileset_name);
    if (anims_result.has_value()) {
        for (auto &anim : anims_result.value() | std::views::values) {
            porymap_component->add_anim(std::move(anim));
        }
    }
    // Note: The returned animations will have frames populated but NO key frames.
    // Key frame extraction is done later by AnimationDecompiler in the domain layer.

    return porymap_component;
}

} // namespace porytiles2
