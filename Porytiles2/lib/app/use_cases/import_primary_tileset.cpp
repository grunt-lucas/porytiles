#include "porytiles2/app/use_cases/import_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> ImportPrimaryTileset::import(const TilesetName &name) const
{
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{name.shorthand(), Style::bold}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(name);
    if (!maybe_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                format_->format("failed to load tileset '{}'", FormatParam{name.shorthand(), Style::bold})},
            maybe_tileset};
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. If `PorymapTilesetComponent` is empty, bail with error.
    if (tileset->porymap_component().is_empty()) {
        /*
         * TODO: check artifact existence via repo class instead of looking at domain types. In fact, TilesetRepo should
         * fail to load a tileset that is missing "essential" artifacts, e.g. metatiles.bin, metatile_attributes.bin,
         * and the 16 palettes. The Porytiles workflow commands prevent users from ever getting into a situation where
         * these artifacts are missing. This domain-layer check here should simply confirm that there are actually
         * metatile tilemap entries on which to operate. If not, then the import will basically wipe everything, We
         * should think about this more.
         */
        return FormattableError{"metatiles.bin is empty, nothing to import"};
    }

    /*
     * TODO: if Porytiles component is empty and no porytiles config files exist, create a porytiles.yaml that sets
     * tileset.compile.[tiles,pals].edit_mode:locked
     */

    if (!tileset_repo_->checksum_provider().cached_checksums_exist(name)) {
        diag_->warning(
            "missing-checksums",
            format_->format("no cached checksums found for tileset '{}'", FormatParam{name.shorthand(), Style::bold}));
    }

    // Only perform the checksum checks if: 1) cached checksums exist and 2) the user is requesting checksum validation
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, name.shorthand(), void);
    if (tileset_repo_->checksum_provider().cached_checksums_exist(name) && verify_checksums.value()) {
        // 4. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`.
        // If any differ, bail with the message "uncompiled changes present in Porytiles asset X."
        if (!tileset->porytiles_component().is_empty()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                porytiles_keys,
                tileset_repo_->key_provider().get_porytiles_artifact_keys(name),
                "failed to get Porytiles artifact keys",
                void);
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(name, porytiles_keys);
            if (!mismatched_keys.empty()) {
                // TODO: better message here
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
        PT_TRY_ASSIGN_CHAIN_ERR(
            porymap_keys,
            tileset_repo_->key_provider().get_porymap_artifact_keys(name),
            "failed to get Porymap artifact keys",
            void);
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(name, porymap_keys)) {
            // TODO: better message here
            diag_->warning("nothing-to-do", "Skipping import, no changes found, TODO: give better message here");
            return {};
        }
    }

    // 6. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.
    auto maybe_imported_tileset = importer_->import(*tileset);
    if (!maybe_imported_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"import job failed for '{}'", FormatParam{name.shorthand(), Style::bold}},
            maybe_imported_tileset};
    }
    const auto imported_tileset = std::move(maybe_imported_tileset.value());

    // 7. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*imported_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"tileset save job failed for '{}'", FormatParam{name.shorthand(), Style::bold}},
            save_result};
    }

    return {};
}

} // namespace porytiles2
