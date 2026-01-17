#include "porytiles2/app/use_cases/create_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> CreatePrimaryTileset::create(const std::string &name) const
{
    // Step 1: Error if tileset already exists
    if (metadata_provider_->exists(name)) {
        return FormattableError{
            "cannot create tileset '{}': a tileset with this name already exists", FormatParam{name, Style::bold}};
    }

    // Step 2: Create blank PorytilesTilesetComponent via PrimaryTilesetCreator
    auto porytiles_component_result = creator_->create_porytiles_component(name);
    if (!porytiles_component_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to create PorytilesTilesetComponent for '{}'", FormatParam{name, Style::bold}},
            porytiles_component_result};
    }
    auto porytiles_component = std::move(porytiles_component_result.value());

    // Step 3: Create blank PorymapTilesetComponent and wrap in Tileset
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset = std::make_unique<Tileset>(name, std::move(porytiles_component), std::move(porymap_component));

    // Step 4: Compile (generates minimal valid Porymap assets from the empty Porytiles component)
    auto compile_result = compiler_->compile(*tileset);
    if (!compile_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"compilation failed for '{}'", FormatParam{name, Style::bold}}, compile_result};
    }
    auto compiled_tileset = std::move(compile_result.value());

    // Step 5: Persist managed state for the new tileset
    // This creates the headers.h entry and tileset-manifest.json BEFORE saving assets
    auto persist_result = tileset_manager_->persist_managed_new(name);
    if (!persist_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to persist managed state for '{}'", FormatParam{name, Style::bold}},
            persist_result};
    }

    // Step 6: Save via TilesetRepo (writes all Porymap artifacts)
    auto save_result = tileset_repo_->save(*compiled_tileset);
    if (!save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to save tileset '{}'", FormatParam{name, Style::bold}}, save_result};
    }

    return {};
}

} // namespace porytiles2
