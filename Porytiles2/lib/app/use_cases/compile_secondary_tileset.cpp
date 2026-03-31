#include "porytiles2/app/use_cases/compile_secondary_tileset.hpp"

#include <memory>
#include <ranges>
#include <set>
#include <string>

#include "porytiles2/app/config/primary_pairing_mode.hpp"
#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/tileset_compiler.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

namespace {

[[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> resolve_partner_primary(
    const std::string &tileset_name,
    PrimaryPairingMode pairing_mode,
    const std::vector<std::string> &partners,
    const TilesetRepo *tileset_repo,
    const TilesetMetadataProvider *metadata_provider,
    const LayoutMetadataProvider *layout_metadata_provider,
    const PorytilesTilesetManager *tileset_manager,
    const UserDiagnostics *diag)
{
    std::string partner_name{};

    switch (pairing_mode) {
    case PrimaryPairingMode::off: {
        if (!partners.empty()) {
            diag->warning(
                "pairing-partners-ignored",
                "Primary pairing mode is 'off' but '{}' has non-empty pairing partners list. Partners will be ignored.",
                FormatParam{tileset_name, Style::bold});
        }
        return std::unique_ptr<Tileset>{nullptr};
    }

    case PrimaryPairingMode::manual: {
        if (partners.empty()) {
            return FormattableError{
                "Primary pairing mode is 'manual' but no partner primaries configured for '{}'.",
                FormatParam{tileset_name, Style::bold}};
        }
        partner_name = partners.at(0);
        break;
    }

    case PrimaryPairingMode::automatic: {
        if (!partners.empty()) {
            diag->warning(
                "pairing-partners-ignored",
                "Primary pairing mode is 'automatic' but '{}' has non-empty pairing partners list. Partners will be "
                "ignored.",
                FormatParam{tileset_name, Style::bold});
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            layout_names,
            layout_metadata_provider->layout_names(),
            std::unique_ptr<Tileset>,
            "Failed to enumerate layouts for automatic primary pairing.");

        std::set<std::string> found_primaries{};
        for (const auto &layout_name : layout_names) {
            auto secondary_result = layout_metadata_provider->secondary_tileset(layout_name);
            if (!secondary_result.has_value()) {
                continue;
            }
            if (secondary_result.value() == tileset_name) {
                auto primary_result = layout_metadata_provider->primary_tileset(layout_name);
                if (primary_result.has_value()) {
                    found_primaries.insert(primary_result.value());
                }
            }
        }

        if (found_primaries.empty()) {
            return FormattableError{
                "Automatic primary pairing found no layouts using secondary tileset '{}'.",
                FormatParam{tileset_name, Style::bold}};
        }

        if (found_primaries.size() > 1) {
            // TODO: eventually support multiple distinct primaries
            diag->warning(
                "multiple-partner-primaries",
                "Multiple distinct partner primaries found for '{}'. Using first found: '{}'.",
                FormatParam{tileset_name, Style::bold},
                FormatParam{*found_primaries.begin(), Style::bold});
        }

        partner_name = *found_primaries.begin();
        break;
    }
    }

    if (!metadata_provider->exists(partner_name)) {
        return FormattableError{
            "Resolved partner primary '{}' for secondary '{}' does not exist.",
            FormatParam{partner_name, Style::bold},
            FormatParam{tileset_name, Style::bold}};
    }
    if (!tileset_manager->is_porytiles_managed(partner_name)) {
        return FormattableError{
            "Secondary compilation requires a Porytiles-managed primary, but '{}' is not Porytiles-managed.",
            FormatParam{partner_name, Style::bold}};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        primary_tileset,
        tileset_repo->load(partner_name),
        std::unique_ptr<Tileset>,
        "Failed to load partner primary '{}' for secondary '{}'.",
        FormatParam{partner_name, Style::bold},
        FormatParam{tileset_name, Style::bold});

    return primary_tileset;
}

} // namespace

ChainableResult<void> CompileSecondaryTileset::compile(const std::string &tileset_name) const
{
    // 1. Precondition: tileset must exist. Check if it is Porytiles-managed.
    assert_or_panic(metadata_provider_->exists(tileset_name), "precondition violated: tileset must exist");
    if (!tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{
            "Tileset '{}' exists but is not Porytiles-managed.", FormatParam{tileset_name, Style::bold}};
    }

    // 2. Read primary pairing configuration.
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, primary_pairing_mode, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, primary_pairing_partners, tileset_name, void);

    // 3. Resolve the partner primary tileset.
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

    // 4. Load the secondary tileset.
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset,
        tileset_repo_->load(tileset_name),
        void,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam(tileset_name, Style::bold)));

    // 5. Checksum verification.
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (!tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag_->formatter().format(
            "No cached checksums found for tileset '{}'.", FormatParam{tileset_name, Style::bold}));
        err_msg.emplace_back(diag_->formatter().format(
            "Expected to find file '{}'.",
            FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
        err_msg.emplace_back("Checksum verification requested via configuration.");
        std::ranges::copy(
            format_config_note_with_separator(diag_->formatter(), verify_checksums), std::back_inserter(err_msg));

        return ChainableResult<void>{FormattableError{err_msg}};
    }

    if (tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums.value()) {
        // Check for unimported Porymap asset changes.
        if (!tileset->porymap_component().is_empty()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                porymap_keys,
                tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name),
                void,
                "Failed to get Porymap artifact keys.");
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, porymap_keys);
            if (!mismatched_keys.empty()) {
                std::vector<std::string> err_msg{};
                err_msg.emplace_back("Changes present in Porymap assets:");
                for (const auto &key : mismatched_keys) {
                    err_msg.emplace_back(diag_->formatter().format("  {}", FormatParam{key.key(), Style::bold}));
                }
                err_msg.emplace_back("");
                err_msg.emplace_back("Compiling now would clobber your Porymap asset changes.");
                err_msg.emplace_back("To resolve:");
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Run '{} {}' to synchronize assets.",
                    FormatParam{"decompile-tileset", Style::bold},
                    FormatParam{tileset_name, Style::bold}));
                err_msg.emplace_back(diag_->formatter().format(
                    "  - {} disable checksum verification to allow the clobber.", FormatParam{"OR", Style::bold}));
                err_msg.emplace_back(diag_->formatter().format(
                    "  - {} delete '{}' cache file.",
                    FormatParam{"OR", Style::bold},
                    FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
                std::ranges::copy(
                    format_config_note_with_separator(diag_->formatter(), verify_checksums),
                    std::back_inserter(err_msg));
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // Check if Porytiles source assets are unchanged (nothing to do).
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_keys,
            tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name),
            void,
            "Failed to get Porytiles artifact keys.");
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porytiles_keys)) {
            diag_->warning(
                "nothing-to-do",
                "Skipping compilation for '{}', no changes found.",
                FormatParam{tileset_name, Style::bold});
            return {};
        }
    }

    // 6. Compile the secondary tileset.
    PT_TRY_ASSIGN_CHAIN_ERR(
        new_tileset,
        compiler_->compile(*tileset, /*is_secondary=*/true, paired_primary.get()),
        void,
        "Compilation job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 7. Persist the compiled tileset (which also caches the checksums).
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*new_tileset),
        void,
        "Tileset save job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 8. Handle animation code wiring.
    if (!new_tileset->porymap_component().anims().empty()) {
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
