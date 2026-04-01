#include "porytiles2/app/use_cases/create_secondary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/app/use_cases/secondary_tileset_helpers.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> CreateSecondaryTileset::create(const std::string &tileset_name) const
{
    // 1. Error if tileset already exists
    if (metadata_provider_->exists(tileset_name)) {
        return FormattableError{
            std::vector<std::string>{"Cannot create tileset '{}'.", "A tileset with this name already exists."},
            std::vector<std::vector<FormatParam>>{std::vector{FormatParam{tileset_name, Style::bold}}}};
    }

    // 2. Resolve partner primary tileset
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, primary_pairing_mode, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, primary_pairing_partners, tileset_name, void);

    PT_TRY_ASSIGN_CHAIN_ERR(
        paired_primary,
        resolve_partner_primary(
            tileset_name,
            primary_pairing_mode.value(),
            primary_pairing_partners.value(),
            tileset_repo_,
            metadata_provider_,
            layout_metadata_provider_,
            tileset_manager_,
            diag_),
        void,
        "Failed to resolve partner primary for secondary '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 3. Create a default secondary PorytilesTilesetComponent via TilesetCreator
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_component,
        creator_->create_sample_secondary_porytiles_component(tileset_name),
        void,
        "Failed to create Porytiles source assets for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 4. Create blank PorymapTilesetComponent and wrap in Tileset
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset =
        std::make_unique<Tileset>(tileset_name, std::move(porytiles_component), std::move(porymap_component));

    // 5. Compile (generates minimal valid Porymap assets from the minimal Porytiles component)
    PT_TRY_ASSIGN_CHAIN_ERR(
        compiled_tileset,
        compiler_->compile(*tileset, /*is_secondary=*/true, paired_primary.get()),
        void,
        "Compilation failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 6. Persist managed state for the new secondary tileset
    PT_TRY_CALL_CHAIN_ERR(
        tileset_manager_->persist_managed_new(tileset_name, /*is_secondary=*/true),
        void,
        "Failed to persist managed state for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 7. Save via TilesetRepo (writes all Porymap artifacts)
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*compiled_tileset),
        void,
        "Failed to save tileset '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 8. Handle animation code wiring
    if (!compiled_tileset->porymap_component().anims().empty()) {
        PT_TRY_CALL_CHAIN_ERR(
            tileset_manager_->wire_anim_code(tileset_name, /*is_secondary=*/true),
            void,
            "Failed to wire animation code for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }
    else {
        PT_TRY_CALL_CHAIN_ERR(
            tileset_manager_->remove_wired_anim_code(tileset_name, /*is_secondary=*/true),
            void,
            "Failed to remove wired animation code for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }

    return {};
}

} // namespace porytiles2
