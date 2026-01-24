#include "porytiles2/app/use_cases/create_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> CreatePrimaryTileset::create(const std::string &tileset_name) const
{
    // 1. Error if tileset already exists
    if (metadata_provider_->exists(tileset_name)) {
        return FormattableError{
            std::vector<std::string>{"Cannot create tileset '{}'.", "A tileset with this name already exists."},
            std::vector<std::vector<FormatParam>>{std::vector{FormatParam{tileset_name, Style::bold}}}};
    }

    // 2. Create a default PorytilesTilesetComponent via PrimaryTilesetCreator
    auto porytiles_component_result = creator_->create_sample_porytiles_component(tileset_name);
    if (!porytiles_component_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to create Porytiles source assets for '{}'.", FormatParam{tileset_name, Style::bold}},
            porytiles_component_result};
    }
    auto porytiles_component = std::move(porytiles_component_result.value());

    // 3. Create blank PorymapTilesetComponent and wrap in Tileset
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset =
        std::make_unique<Tileset>(tileset_name, std::move(porytiles_component), std::move(porymap_component));

    // 4. Compile (generates minimal valid Porymap assets from the minimal Porytiles component)
    auto compile_result = compiler_->compile(*tileset);
    if (!compile_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Compilation failed for '{}'.", FormatParam{tileset_name, Style::bold}}, compile_result};
    }
    auto compiled_tileset = std::move(compile_result.value());

    // 5. Persist managed state for the new tileset
    // This creates the headers.h entry and tileset-manifest.json BEFORE saving assets
    auto persist_result = tileset_manager_->persist_managed_new(tileset_name);
    if (!persist_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to persist managed state for '{}'.", FormatParam{tileset_name, Style::bold}},
            persist_result};
    }

    // 6. Save via TilesetRepo (writes all Porymap artifacts)
    auto save_result = tileset_repo_->save(*compiled_tileset);
    if (!save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to save tileset '{}'.", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    // 7. Handle animation code wiring
    if (!compiled_tileset->porymap_component().anims().empty()) {
        // Tileset has animations - wire the generated code
        auto wire_result = tileset_manager_->wire_anim_code(tileset_name, /*is_secondary=*/false);
        if (!wire_result.has_value()) {
            return ChainableResult<void>{
                FormattableError{"Failed to wire animation code for '{}'.", FormatParam{tileset_name, Style::bold}},
                wire_result};
        }
    }
    else {
        // Tileset has no animations - remove any stale wiring
        auto remove_result = tileset_manager_->remove_wired_anim_code(tileset_name, /*is_secondary=*/false);
        if (!remove_result.has_value()) {
            return ChainableResult<void>{
                FormattableError{
                    "Failed to remove wired animation code for '{}'.", FormatParam{tileset_name, Style::bold}},
                remove_result};
        }
    }

    return {};
}

} // namespace porytiles2
