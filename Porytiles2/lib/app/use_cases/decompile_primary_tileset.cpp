#include "porytiles2/app/use_cases/decompile_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> DecompilePrimaryTileset::decompile(const std::string &tileset_name) const
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
            FormattableError{format_->format("Failed to load tileset '{}'.", FormatParam{tileset_name, Style::bold})},
            maybe_tileset};
    }
    const auto tileset = std::move(maybe_tileset.value());

    if (!tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name)) {
        diag_->warning(
            "missing-checksums",
            format_->format("no cached checksums found for tileset '{}'", FormatParam{tileset_name, Style::bold}));
    }

    // 3. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `tileset.cache.json`.
    // If any differ, bail with the message "uncompiled changes present in Porytiles asset X."

    // Only perform the checksum checks if: 1) cached checksums exist and 2) the user is requesting checksum validation
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums.value()) {
        if (!tileset->porytiles_component().is_empty()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                porytiles_keys,
                tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name),
                "Failed to get Porytiles artifact keys.",
                void);
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, porytiles_keys);
            if (!mismatched_keys.empty()) {
                // TODO: better message here
                std::vector<std::string> err_msg{};
                err_msg.reserve(mismatched_keys.size());
                err_msg.emplace_back("Uncompiled changes present in Porytiles assets:");
                for (const auto &key : mismatched_keys) {
                    err_msg.emplace_back(format_->format("  {}", FormatParam{key.key(), Style::bold}));
                }
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // 4. If all `PorymapTilesetComponent` checksums match those cached in `tileset.cache.json`, bail with the
        // message "nothing to do."
        PT_TRY_ASSIGN_CHAIN_ERR(
            porymap_keys,
            tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name),
            "Failed to get Porymap artifact keys.",
            void);
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porymap_keys)) {
            // TODO: better message here
            diag_->warning("nothing-to-do", "Skipping decompile, no changes found, TODO: give better message here");
            return {};
        }
    }

    // 5. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.
    auto maybe_decompiled_tileset = decompiler_->decompile(*tileset);
    if (!maybe_decompiled_tileset.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Import job failed for '{}'.", FormatParam{tileset_name, Style::bold}},
            maybe_decompiled_tileset};
    }
    const auto decompiled_tileset = std::move(maybe_decompiled_tileset.value());

    // 6. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*decompiled_tileset); !save_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Tileset save job failed for '{}'.", FormatParam{tileset_name, Style::bold}}, save_result};
    }

    // 7. Handle animation code wiring
    if (!decompiled_tileset->porytiles_component().anims().empty()) {
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
