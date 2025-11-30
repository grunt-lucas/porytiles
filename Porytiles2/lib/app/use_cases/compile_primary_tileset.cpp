#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <memory>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const
{
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return ChainableResult<void>{
            FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format("failed to load tileset '{}'", FormatParam{tileset_name, Style::bold})},
            maybe_tileset};
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. If `PorytilesTilesetComponent` is empty, bail with error.
    if (tileset->porytiles_component().is_empty()) {
        // TODO: better user-facing message
        return ChainableResult<void>{FormattableError{"PorytilesTilesetComponent was empty"}};
    }

    if (!tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name)) {
        diag_->warn(
            "missing-checksums",
            format_->format("no cached checksums found for tileset '{}'", FormatParam{tileset_name, Style::bold}));
    }

    // Only perform the checksum checks if: 1) cached checksums exist and 2) the user is requesting checksum validation
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums.value()) {
        // 4. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If
        // any differ, bail with the message "unimported changes present in Porymap asset X."
        if (!tileset->porymap_component().is_empty()) {
            const auto porymap_keys = tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name);
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, porymap_keys);
            if (!mismatched_keys.empty()) {
                // TODO: better message here
                std::vector<std::string> err_msg{};
                err_msg.reserve(mismatched_keys.size());
                err_msg.emplace_back("unimported changes present in Porymap assets:");
                for (const auto &key : mismatched_keys) {
                    err_msg.emplace_back("  " + key.key());
                }
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // 5. If all `PorytilesTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with
        // the message "nothing to do."
        const auto porytiles_keys = tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name);
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porytiles_keys)) {
            // TODO: better message here
            diag_->warn("nothing-to-do", "Skipping compilation, no changes found, TODO: give better message here");
            return {};
        }
    }

    // 6. Compile the `Tileset`, generating a new modified `Tileset`.
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, patch_build_enabled, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, patch_tiles_mode, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, patch_pal_mode, tileset_name, void);
    auto maybe_compiled_tileset =
        patch_build_enabled.value()
            ? compiler_->compile_patch(*tileset, patch_tiles_mode.value(), patch_pal_mode.value())
            : compiler_->compile(*tileset);
    if (!maybe_compiled_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"compilation job failed for '{}'", FormatParam{tileset_name, Style::bold}},
            maybe_compiled_tileset};
    }
    const auto new_tileset = std::move(maybe_compiled_tileset.value());

    // 7. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*new_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"tileset save job failed for '{}'", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    return {};
}

} // namespace porytiles2
