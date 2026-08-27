#include "porytiles/app/use_cases/import_primary_tileset.hpp"

#include <map>
#include <memory>
#include <string>

#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/infra/config/override_config_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace porytiles {

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
    // A blank Porytiles component carries no prior attributes.csv state, so its layer_type pin state defaults to
    // no_csv. The decompiler below reads that as "pin every row from the bin", which is exactly what a from-scratch
    // import should be doing. Since import always clobbers porytiles_src/, there is no prior CSV to preserve.
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

    // Step 4: Re-compile the decompiled tileset in locked edit mode. This forces byte-equivalence between the imported
    // Porymap assets and the Porytiles sources we just decompiled from them, and surfaces any invalid input (e.g. a
    // palette slot containing the extrinsic transparency color) as a proper diagnostic at import time rather than
    // deferring the failure to a later compile.
    //
    // The override provider forces tiles_edit_mode and palettes_edit_mode to 'locked' without touching other user
    // config. ET is intentionally NOT overridden here: it is a property of the Porymap palettes themselves and the
    // user's configured value is the right thing to validate against.
    auto import_override = std::make_unique<OverrideConfigProvider>(
        ConfigScopeType::tileset, tileset_name, "import-tileset internal locked recompile");
    import_override->set_tiles_edit_mode(ArtifactEditMode::locked);
    import_override->set_palettes_edit_mode(ArtifactEditMode::locked);
    domain_config_->add_provider(std::move(import_override));

    PT_TRY_ASSIGN_CHAIN_ERR(
        compiled_tileset,
        compiler_->compile(*decompiled_tileset, /*is_secondary=*/false),
        void,
        "Internal locked recompile failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 5: Save the compiled tileset so Porymap artifacts round-trip byte-equivalent to the Porytiles source.
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*compiled_tileset),
        void,
        "Tileset save job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 6: Confirmed save succeeded, now call PorytilesTilesetManager::persist_existing to persist "managed"
    // state (which in the Project-based impls writes to tileset-manifest.json and updates various project C files).
    // This should never fail for a reasonable cause, so we don't need to worry about rolling back or weird broken
    // state. If it does fail for extraordinary reasons, we present a helpful message to users so they can manually
    // recover.
    PT_TRY_CALL_CHAIN_ERR(
        tileset_manager_->persist_managed_existing(tileset_name),
        void,
        "Failed to persist Porytiles-managed state for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 7: Handle animation code wiring.
    if (!compiled_tileset->porytiles_component().anims().empty()) {
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

} // namespace porytiles
