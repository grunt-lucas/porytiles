#include "porytiles2/app/use_cases/import_primary_tileset.hpp"

#include <map>
#include <memory>
#include <string>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

/*
 * A first-pass attempt that's not quite right.
 */
// ChainableResult<void> ImportPrimaryTileset::import_old_attempt(const std::string &tileset_name) const
// {
//     // Step 1: Validate tileset exists
//     if (!tileset_metadata_provider_->exists(tileset_name)) {
//         return FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}};
//     }
//
//     // Step 2: Validate tileset isn't already Porytiles-managed
//     if (porytiles_tileset_manager_->is_porytiles_managed(tileset_name)) {
//         return FormattableError{"tileset '{}' is already Porytiles-managed", FormatParam{tileset_name, Style::bold}};
//     }
//
//     // Step 3: CREATE a new empty Tileset (NOT load!)
//     // For first-time imports, the Tileset doesn't exist at deterministic paths yet.
//     // TilesetRepo::load() only works for already Porytiles-managed tilesets.
//     auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
//     auto porymap_component = std::make_unique<PorymapTilesetComponent>();
//     auto tileset =
//         std::make_unique<Tileset>(tileset_name, std::move(porytiles_component), std::move(porymap_component));
//
//     // Step 4: Get extrinsic transparency color from config
//     PT_UNWRAP_TILESET_CONFIG_PTR(domain_config_, extrinsic_transparency, tileset_name, void);
//
//     // Step 5: Import animations from vanilla tileset_anims.c
//     auto animations_result =
//         vanilla_animation_importer_->import_animations(tileset_name, extrinsic_transparency.value());
//     if (!animations_result.has_value()) {
//         return ChainableResult<void>{FormattableError{"failed to import animations"}, animations_result};
//     }
//     auto animations = std::move(animations_result).value();
//     const auto animation_count = animations.size();
//
//     for (auto &anim : animations | std::views::values) {
//         tileset->porytiles_component().add_anim(std::move(anim));
//     }
//
//     // TODO (Phase 5/6): Read vanilla Porymap artifacts (metatiles.bin, tiles.png, palettes)
//     // TODO (Phase 5/6): Decompile metatiles → layer PNGs (bottom.png, middle.png, top.png)
//     // TODO (Phase 5): Modify C headers (headers.h, graphics.h, metatiles.h, tileset_anims.c)
//     // TODO (Phase 3): Write original_artifacts.json
//
//     // Step 6: Save to deterministic paths
//     // This writes anim.yaml and animation frame PNGs to Porytiles-managed locations
//     const auto save_result = tileset_repo_->save(*tileset);
//     if (!save_result.has_value()) {
//         return ChainableResult<void>{FormattableError{"failed to save tileset"}, save_result};
//     }
//
//     diag_->remark(
//         "import-tileset",
//         "imported " + std::to_string(animation_count) + " animations for " + tileset_name +
//             " (note: full import requires Phase 5/6 components)");
//
//     return {};
// }

ChainableResult<void> ImportPrimaryTileset::import(const std::string &tileset_name) const
{
    // Step 1: Validate tileset exists
    if (!tileset_metadata_provider_->exists(tileset_name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}};
    }

    // Step 2: Validate tileset isn't already Porytiles-managed
    if (porytiles_tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{"tileset '{}' is already Porytiles-managed", FormatParam{tileset_name, Style::bold}};
    }

    // Step 3: Call the importer service to bring in the tileset from vanilla assets
    auto maybe_imported_tileset = importer_->import(tileset_name);
    if (!maybe_imported_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"import job failed for '{}'", FormatParam{tileset_name, Style::bold}},
            maybe_imported_tileset};
    }
    const auto imported_tileset = std::move(maybe_imported_tileset.value());

    // Step 4: Save to deterministic paths
    if (const auto save_result = tileset_repo_->save(*imported_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"tileset save job failed for '{}'", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    return {};
}

} // namespace porytiles2
