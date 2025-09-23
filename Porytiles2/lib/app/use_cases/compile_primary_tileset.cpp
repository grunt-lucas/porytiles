#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

ChainableResult<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const
{
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return ChainableResult<void>{BasicError{"tileset '{}' does not exist", std::vector{tileset_name}}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        // TODO: hook up ChainableError here
        return ChainableResult<void>::chain_together(
            BasicError{fmt::format("failed to load tileset '{}'", tileset_name)}, maybe_tileset);
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. If `PorytilesTilesetComponent` is empty, bail with error.
    if (tileset->porytiles_component().is_empty()) {
        return ChainableResult<void>{BasicError{"PorytilesTilesetComponent was empty"}};
    }

    // 4. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If any
    // differ, bail with the message "unimported changes present in Porymap asset X."
    if (!tileset->porymap_component().is_empty()) {
        const auto porymap_keys = tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name);
        const auto mismatched_keys =
            tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, porymap_keys);
        if (!mismatched_keys.empty()) {
            return ChainableResult<void>{BasicError{"unimported changes present in Porymap assets: TODO keys here"}};
        }
    }

    // 5. If all `PorytilesTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."
    const auto porytiles_keys = tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name);
    if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porytiles_keys)) {
        // TODO: display a nothing_to_do message to the user
        return {};
    }

    // 6. Compile the `Tileset`, generating a new modified `Tileset`.
    auto maybe_new_tileset = compiler_->compile(*tileset);
    if (!maybe_new_tileset.has_value()) {
        return ChainableResult<void>{BasicError{maybe_new_tileset.error()}};
    }
    const auto new_tileset = std::move(maybe_new_tileset.value());

    // 7. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*new_tileset); !save_result.has_value()) {
        return ChainableResult<void>{BasicError{save_result.error()}};
    }

    return {};
}

} // namespace porytiles2
