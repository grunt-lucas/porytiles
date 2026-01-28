#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <memory>
#include <ranges>
#include <string>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const
{
    // 1. Check if the primary tileset exists and is Porytiles-managed. If not, abort with error.
    if (!metadata_provider_->exists(tileset_name)) {
        return FormattableError{"Tileset '{}' does not exist.", FormatParam{tileset_name, Style::bold}};
    }
    if (!tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{
            "Tileset '{}' exists but is not Porytiles-managed.", FormatParam{tileset_name, Style::bold}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset_name, Style::bold})},
            maybe_tileset};
    }
    const auto tileset = std::move(maybe_tileset.value());

    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (!tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag_->formatter().format(
            "No cached checksums found for tileset '{}'.", FormatParam{tileset_name, Style::bold}));
        err_msg.emplace_back(diag_->formatter().format(
            "Expected to find file '{}'.",
            FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
        err_msg.emplace_back("Checksum verification requested via configuration.");
        err_msg.emplace_back("");
        err_msg.emplace_back("--------");
        err_msg.emplace_back("");
        std::ranges::copy(format_config_note(diag_->formatter(), verify_checksums), std::back_inserter(err_msg));

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
                "Failed to get Porymap artifact keys.",
                void);
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
                err_msg.emplace_back("");
                err_msg.emplace_back("--------");
                err_msg.emplace_back("");
                std::ranges::copy(
                    format_config_note(diag_->formatter(), verify_checksums), std::back_inserter(err_msg));
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // 4. If all `PorytilesTilesetComponent` checksums match those cached in `tileset.cache.json`, bail with
        // the message "nothing to do."
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_keys,
            tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name),
            "Failed to get Porytiles artifact keys.",
            void);
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porytiles_keys)) {
            diag_->warning(
                "nothing-to-do",
                "Skipping compilation for '{}', no changes found.",
                FormatParam{tileset_name, Style::bold});
            return {};
        }
    }

    // 5. Compile the `Tileset`, generating a new modified `Tileset`.
    auto maybe_compiled_tileset = compiler_->compile(*tileset);
    if (!maybe_compiled_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Compilation job failed for '{}'.", FormatParam{tileset_name, Style::bold}},
            maybe_compiled_tileset};
    }
    const auto new_tileset = std::move(maybe_compiled_tileset.value());

    // 6. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*new_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Tileset save job failed for '{}'.", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    // 7. Handle animation code wiring
    if (!new_tileset->porymap_component().anims().empty()) {
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
