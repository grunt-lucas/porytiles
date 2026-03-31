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

ChainableResult<void> ImportPrimaryTileset::import(const std::string &tileset_name) const
{
    // Step 1: Validate tileset exists and isn't already Porytiles-managed
    if (!metadata_provider_->exists(tileset_name)) {
        return FormattableError{"Tileset '{}' does not exist.", FormatParam{tileset_name, Style::bold}};
    }
    if (tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{"Tileset '{}' is already Porytiles-managed.", FormatParam{tileset_name, Style::bold}};
    }

    // Step 2: Call the importer service to bring in the PorymapTilesetComponent from vanilla assets
    PT_TRY_ASSIGN_CHAIN_ERR(
        imported_porymap_component,
        importer_->import_porymap_component_from_vanilla(tileset_name),
        void,
        "Import job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));
    auto blank_porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto tileset = std::make_unique<Tileset>(
        tileset_name, std::move(blank_porytiles_component), std::move(imported_porymap_component));

    // Step 3: Decompile the PorymapTilesetComponent to produce a matching PorytilesTilesetComponent
    PT_TRY_ASSIGN_CHAIN_ERR(
        decompiled_tileset,
        decompiler_->decompile(*tileset),
        void,
        "Decompile job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    /*
     * TODO: before saving, we should probably perform an aritfact_edit_mode:locked compilation here. That way, Porymap
     * assets completely match their Porytiles counterparts. See note about user config override in
     * CreatePrimaryTileset.
     */

    // Step 4: Save to deterministic paths
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*decompiled_tileset),
        void,
        "Tileset save job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    /*
     * Step 5: Confirmed save succeeded, now call PorytilesTilesetManager::persist_existing to persist "managed"
     * state (which in the Project-based impls writes to tileset-manifest.json and updates various project C files).
     * This should never fail for a reasonable cause, so we don't need to worry about rolling back or weird broken
     * state. If it does fail for extraordinary reasons, we present a helpful message to users so they can manually
     * recover.
     */
    // TODO: add more details to this error message
    PT_TRY_CALL_CHAIN_ERR(
        tileset_manager_->persist_managed_existing(tileset_name),
        void,
        "Failed to persist Porytiles-managed state for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 6. Handle animation code wiring
    if (!decompiled_tileset->porytiles_component().anims().empty()) {
        // Tileset has animations - wire the generated code
        PT_TRY_CALL_CHAIN_ERR(
            tileset_manager_->wire_anim_code(tileset_name, /*is_secondary=*/false),
            void,
            "Failed to wire animation code for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }
    else {
        // Tileset has no animations - remove any stale wiring
        PT_TRY_CALL_CHAIN_ERR(
            tileset_manager_->remove_wired_anim_code(tileset_name, /*is_secondary=*/false),
            void,
            "Failed to remove wired animation code for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }

    return {};
}

} // namespace porytiles2
