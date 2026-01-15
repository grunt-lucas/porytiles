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
    if (!tileset_metadata_provider_->exists(tileset_name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}};
    }
    if (porytiles_tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{"tileset '{}' is already Porytiles-managed", FormatParam{tileset_name, Style::bold}};
    }

    // Step 2: Call the importer service to bring in the PorymapTilesetComponent from vanilla assets
    auto imported_porymap_component_result = importer_->import_porymap_component_from_vanilla(tileset_name);
    if (!imported_porymap_component_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"import job failed for '{}'", FormatParam{tileset_name, Style::bold}},
            imported_porymap_component_result};
    }
    auto imported_porymap_component = std::move(imported_porymap_component_result.value());
    auto blank_porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto tileset = std::make_unique<Tileset>(
        tileset_name, std::move(blank_porytiles_component), std::move(imported_porymap_component));

    // Step 3: Decompile the PorymapTilesetComponent to produce a matching PorytilesTilesetComponent
    auto decompiled_tileset_result = decompiler_->decompile(*tileset);
    if (!decompiled_tileset_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"decompile job failed for '{}'", FormatParam{tileset_name, Style::bold}},
            decompiled_tileset_result};
    }
    const auto decompiled_tileset = std::move(decompiled_tileset_result.value());

    // Step 4: Save to deterministic paths
    if (const auto save_result = tileset_repo_->save(*decompiled_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"tileset save job failed for '{}'", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    /*
     * Step 5: Confirmed save succeeded, now call PorytilesTilesetManager::persist_managed_state to persist "managed"
     * state (which in the Project-based impls writes to tileset-manifest.json and updates various project C files).
     * This should never fail for a reasonable cause, so we don't need to worry about rolling back or weird broken
     * state. If it does fail for extraordinary reasons, we present a helpful message to users so they can manually
     * recover.
     */
    if (const auto persist_state_result = porytiles_tileset_manager_->persist_managed_state(tileset_name);
        !persist_state_result.has_value()) {
        // TODO: add more details to this error message
        return ChainableResult<void>{
            FormattableError{
                "failed to persist Porytiles-managed state for '{}'", FormatParam{tileset_name, Style::bold}},
            persist_state_result};
    }

    return {};
}

} // namespace porytiles2
