#include "porytiles2/infra/services/project_primary_tileset_importer.hpp"

#include <format>

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/infra/algorithms/porymap_artifact_parsers.hpp"
#include "porytiles2/infra/services/project_vanilla_anim_importer.hpp"
#include "porytiles2/utilities/filesystem_utils.hpp"
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
        std::unique_ptr<PorymapTilesetComponent>,
        format_->format("Failed to get artifact paths for tileset '{}'.", FormatParam{tileset_name, Style::bold}));

    // Step 2: Parse metatiles.bin
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatile_entries,
        parse_metatiles_bin(project_root_ / artifact_paths.metatiles_path()),
        std::unique_ptr<PorymapTilesetComponent>,
        "Failed to parse metatiles.bin.");

    for (auto &entry : metatile_entries) {
        porymap_component->push_back_tilemap_entry(std::move(entry));
    }

    // Step 3: Parse metatile_attributes.bin (dispatch on attribute size for correct format)
    PT_TRY_ASSIGN_CHAIN_ERR(
        attributes,
        metatile_attr_size_ == attr::bytes_per_attr_firered
            ? parse_firered_metatile_attributes(project_root_ / artifact_paths.metatile_attributes_path())
            : parse_emerald_metatile_attributes(project_root_ / artifact_paths.metatile_attributes_path()),
        std::unique_ptr<PorymapTilesetComponent>,
        "Failed to parse metatile_attributes.bin.");

    for (auto &attr : attributes) {
        porymap_component->push_back_attribute(std::move(attr));
    }

    // Step 4: Load tiles.png (strip all extensions like .4bpp.smol, then add .png)
    auto tiles_png_path = strip_all_extensions(artifact_paths.tiles_path());
    tiles_png_path += ".png";

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_image,
        load_indexed_png(project_root_ / tiles_png_path, *png_loader_),
        std::unique_ptr<PorymapTilesetComponent>,
        "Failed to load tiles.png.");

    porymap_component->tiles_png(*tiles_image);

    // Step 5: Load palettes from the discovered palette paths
    const auto &palette_paths = artifact_paths.palette_paths();
    for (std::size_t i = 0; i < palette_paths.size() && i < pal::num_pals; ++i) {
        // Convert .gbapal path to .pal by stripping all extensions and adding .pal
        auto pal_path = strip_all_extensions(palette_paths[i]);
        pal_path += ".pal";

        PT_TRY_ASSIGN_CHAIN_ERR(
            palette,
            load_porymap_palette(project_root_ / pal_path, *pal_loader_),
            std::unique_ptr<PorymapTilesetComponent>,
            format_->format("Failed to load palette {}.", FormatParam{pal_filename(i), Style::bold}));

        porymap_component->set_pal(i, std::move(palette));
    }

    // Step 6: Import animations using ProjectVanillaAnimImporter
    // Animation import failure is non-fatal - tileset may not have animations
    ProjectVanillaAnimImporter anim_importer{project_root_, format_, diag_};

    auto anims_result = anim_importer.import_animations(tileset_name);
    if (!anims_result.has_value()) {
        return ChainableResult<std::unique_ptr<PorymapTilesetComponent>>{
            FormattableError{"Failed to import animations for '{}'.", FormatParam{tileset_name, Style::bold}},
            anims_result};
    }
    for (auto &anim : anims_result.value() | std::views::values) {
        porymap_component->add_anim(std::move(anim));
    }

    // Note: The returned animations will have frames populated but NO key frames.
    // Key frame extraction is done later by AnimDecompiler in the domain layer.

    return porymap_component;
}

} // namespace porytiles2
