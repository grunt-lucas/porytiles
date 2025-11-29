#include "porytiles2/app/use_cases/import_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> ImportPrimaryTileset::import(const std::string &tileset_name) const
{
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return FormattableError{fmt::format("tileset {} does not exist", tileset_name)};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        // TODO: hook up ChainableError here
        return FormattableError{"failed to load tileset"};
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. If `PorymapTilesetComponent` is empty, bail with error.
    if (tileset->porymap_component().is_empty()) {
        return FormattableError{"PorymapTilesetComponent was empty"};
    }

    /*
     * TODO: if Porytiles component is empty and no porytiles config files exist, create a porytiles.yaml that sets
     * tileset.compile.patch.enabled:true
     */

    // Only perform the checksum checks if: 1) cached checksums exist and 2) the user is requesting checksum validation
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums.value()) {
        // 4. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`.
        // If any differ, bail with the message "uncompiled changes present in Porytiles asset X."
        if (!tileset->porytiles_component().is_empty()) {
            const auto porytiles_keys = tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name);
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, porytiles_keys);
            if (!mismatched_keys.empty()) {
                // TODO: better message here?
                std::vector<std::string> err_msg{};
                err_msg.reserve(mismatched_keys.size());
                err_msg.emplace_back("uncompiled changes present in Porytiles assets:");
                for (const auto &key : mismatched_keys) {
                    err_msg.emplace_back("  " + key.key());
                }
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // 5. If all `PorymapTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
        // message "nothing to do."
        const auto porymap_keys = tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name);
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porymap_keys)) {
            // TODO: better message here
            diag_->warn("nothing-to-do", "Skipping import, no changes found, TODO: give better message here");
            return {};
        }
    }

    // 6. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.
    auto maybe_imported_tileset = importer_->import(*tileset);
    if (!maybe_imported_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"import job failed for '{}'", FormatParam{tileset_name, Style::bold}},
            maybe_imported_tileset};
    }
    const auto imported_tileset = std::move(maybe_imported_tileset.value());

    // 7. Perform a patch build.
    auto maybe_recompiled_tileset =
        compiler_->compile_patch(*imported_tileset, PatchTilesMode::fixed, PatchPalMode::fixed);
    if (!maybe_recompiled_tileset.has_value()) {
        // return ChainableResult<void>{
        //     FormattableError{"patch compilation job failed for '{}'", FormatParam{tileset_name, Style::bold}},
        //     maybe_new_tileset};
        panic("patch re-compilation after import failed: this should never happen right?");
    }
    const auto new_tileset = std::move(maybe_recompiled_tileset.value());

    // 8. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*new_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"tileset save job failed for '{}'", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    return {};
}

} // namespace porytiles2
