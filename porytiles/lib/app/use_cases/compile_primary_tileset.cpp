#include "porytiles/app/use_cases/compile_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace porytiles {

ChainableResult<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const
{
    // 1. Precondition: tileset must exist. Check if it is Porytiles-managed. If not, abort with error.
    assert_or_panic(metadata_provider_->exists(tileset_name), "precondition violated: tileset must exist");
    if (!tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{
            "Tileset '{}' exists but is not Porytiles-managed.", FormatParam{tileset_name, Style::bold}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset,
        tileset_repo_->load(tileset_name),
        void,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam(tileset_name, Style::bold)));

    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (!tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag_->formatter().format(
            "No cached checksums found for tileset '{}'.", FormatParam{tileset_name, Style::bold}));
        err_msg.emplace_back(diag_->formatter().format(
            "Expected to find file '{}'.",
            FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
        err_msg.emplace_back("Checksum verification requested via configuration.");
        err_msg.append_range(format_config_note_with_separator(diag_->formatter(), verify_checksums));

        return ChainableResult<void>{FormattableError{err_msg}};
    }

    // Only perform the checksum checks if: 1) cached checksums exist and 2) the user is requesting checksum validation
    if (tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums.value()) {
        // 3. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `tileset.cache.json`. If
        // any differ, bail with the message "unimported changes present in Porymap asset X."
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
                err_msg.append_range(format_config_note_with_separator(diag_->formatter(), verify_checksums));
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // 4. If all `PorytilesTilesetComponent` checksums match those cached in `tileset.cache.json`, bail with
        // the message "nothing to do."
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

    // 5. Compile the `Tileset`, generating a new modified `Tileset`.
    PT_TRY_ASSIGN_CHAIN_ERR(
        new_tileset,
        compiler_->compile(*tileset, /*is_secondary=*/false),
        void,
        "Compilation job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 6. Persist the `Tileset` (which also caches the checksums).
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*new_tileset),
        void,
        "Tileset save job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 7. Handle animation code wiring
    if (!new_tileset->porymap_component().anims().empty()) {
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
