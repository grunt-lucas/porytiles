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
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_component,
        creator_->create_sample_porytiles_component(tileset_name),
        void,
        "Failed to create Porytiles source assets for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 3. Create blank PorymapTilesetComponent and wrap in Tileset
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset =
        std::make_unique<Tileset>(tileset_name, std::move(porytiles_component), std::move(porymap_component));

    /*
     * TODO: we need to create a way for internal code to override user configuration. E.g., for this recompilation
     * operation, we want to be able to control a lot of the configuration values like extrinsic_transparency,
     * artifact_edit_mode, etc. I have a couple of ideas:
     *
     * 1. Create a master Config interface with one method: add_provider. Domain, App, InfraConfig all inherit from this
     * interface. LazyLayeredConfig implements this add_provider method. Then, within this create function, we can
     * easily just call domain_config->add_provider(override_provider), where OverrideProvider is a new provider type
     * that will allow us to specify config values we care about, and leave alone ones we want to come from the user.
     *
     * 2. We can construct the compiler within the create function instead of injecting it. And then as the DomainConfig
     * param we provide a MockDomainConfig that's defined locally in an anonymous namespace, that forces the settings we
     * want. The downside to this is that we don't respect any user settings. We might want to respect some of the user
     * compilation settings that aren't necessarily relevant to this compilation. I.e., while we must override settings
     * like extrinsic_transparency, since our created tileset transparency might not match the user global setting, some
     * settings like tileset.paths.primary from the user should absolutely be respected.
     */
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

} // namespace porytiles2
