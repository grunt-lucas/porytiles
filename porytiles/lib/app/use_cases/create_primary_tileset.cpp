#include "porytiles/app/use_cases/create_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/infra/config/override_config_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

ChainableResult<void> CreatePrimaryTileset::create(const std::string &tileset_name) const
{
    // 1. Error if tileset already exists
    if (metadata_provider_->exists(tileset_name)) {
        return FormattableError{
            std::vector<std::string>{"Cannot create tileset '{}'.", "A tileset with this name already exists."},
            std::vector<std::vector<FormatParam>>{std::vector{FormatParam{tileset_name, Style::bold}}}};
    }

    // 2. Create a default PorytilesTilesetComponent via PrimaryTilesetCreator
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_component,
        creator_->create_sample_primary_porytiles_component(tileset_name),
        void,
        "Failed to create Porytiles source assets for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 3. Create blank PorymapTilesetComponent and wrap in Tileset
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset =
        std::make_unique<Tileset>(tileset_name, std::move(porytiles_component), std::move(porymap_component));

    // Layer an override provider that forces the domain config values required for the sample-tileset recompile. The
    // TilesetCreator bakes rgba_magenta into every sample tile as the transparency color, so we must force
    // extrinsic_transparency to rgba_magenta regardless of the user's configured value. tiles/palettes edit mode are
    // forced to 'optimize' so the compiler generates fresh Porymap artifacts for the blank tileset. User infra
    // settings (paths, etc.) are left intact because the override only touches the three fields set below.
    auto create_override = std::make_unique<OverrideConfigProvider>(
        ConfigScopeType::tileset, tileset_name, "create-tileset internal sample compile");
    create_override->set_extrinsic_transparency(rgba_magenta);
    create_override->set_tiles_edit_mode(ArtifactEditMode::optimize);
    create_override->set_palettes_edit_mode(ArtifactEditMode::optimize);
    domain_config_->add_provider(std::move(create_override));

    // 4. Compile (generates minimal valid Porymap assets from the minimal Porytiles component)
    PT_TRY_ASSIGN_CHAIN_ERR(
        compiled_tileset,
        compiler_->compile(*tileset, /*is_secondary=*/false),
        void,
        "Compilation failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 5. Persist managed state for the new tileset
    // This creates the headers.h entry and tileset-manifest.json BEFORE saving assets
    PT_TRY_CALL_CHAIN_ERR(
        tileset_manager_->persist_managed_new(tileset_name),
        void,
        "Failed to persist managed state for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 6. Save via TilesetRepo (writes all Porymap artifacts)
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*compiled_tileset),
        void,
        "Failed to save tileset '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 7. Handle animation code wiring
    if (!compiled_tileset->porymap_component().anims().empty()) {
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
