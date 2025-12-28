#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> CompilePrimaryTileset::compile(const std::string &name) const
{
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(name)) {
        return ChainableResult<void>{FormattableError{"tileset '{}' does not exist", FormatParam{name, Style::bold}}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(name);
    if (!maybe_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format("failed to load tileset '{}'", FormatParam{name, Style::bold})},
            maybe_tileset};
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. If `PorytilesTilesetComponent` is empty, bail with error.
    if (tileset->porytiles_component().is_empty()) {
        /*
         * TODO: check artifact existence via repo class instead of looking at domain types. In fact, TilesetRepo should
         * fail to load a tileset that is missing "essential" artifacts, e.g. metatiles.bin, metatile_attributes.bin,
         * and the 16 palettes. The Porytiles workflow commands prevent users from ever getting into a situation where
         * these artifacts are missing. This domain-layer check here should simply confirm that there are actually
         * layer images with content on which to operate. If not, then the compile will basically wipe everything, We
         * should think about this more.
         */
        return ChainableResult<void>{FormattableError{"no Porytiles assets found, try importing the tileset first"}};
    }

    if (!tileset_repo_->checksum_provider().cached_checksums_exist(name)) {
        diag_->warning(
            "missing-checksums",
            format_->format("no cached checksums found for tileset '{}'", FormatParam{name, Style::bold}));
    }

    // Only perform the checksum checks if: 1) cached checksums exist and 2) the user is requesting checksum validation
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, name, void);
    if (tileset_repo_->checksum_provider().cached_checksums_exist(name) && verify_checksums.value()) {
        // 4. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If
        // any differ, bail with the message "unimported changes present in Porymap asset X."
        if (!tileset->porymap_component().is_empty()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                porymap_keys,
                tileset_repo_->key_provider().get_porymap_artifact_keys(name),
                "failed to get Porymap artifact keys",
                void);
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(name, porymap_keys);
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
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_keys,
            tileset_repo_->key_provider().get_porytiles_artifact_keys(name),
            "failed to get Porytiles artifact keys",
            void);
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(name, porytiles_keys)) {
            // TODO: better message here
            diag_->warning("nothing-to-do", "Skipping compilation, no changes found, TODO: give better message here");
            return {};
        }
    }

    // 6. Compile the `Tileset`, generating a new modified `Tileset`.
    auto maybe_compiled_tileset = compiler_->compile(*tileset);
    if (!maybe_compiled_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"compilation job failed for '{}'", FormatParam{name, Style::bold}},
            maybe_compiled_tileset};
    }
    const auto new_tileset = std::move(maybe_compiled_tileset.value());

    // 7. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*new_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"tileset save job failed for '{}'", FormatParam{name, Style::bold}}, save_result};
    }

    return {};
}

} // namespace porytiles2
