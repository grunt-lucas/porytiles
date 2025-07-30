#include "porytiles2/app/use_cases/import_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> ImportPrimaryTileset::import(const std::string &tileset_name) const {
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return std::unexpected{fmt::format("tileset {} does not exist", tileset_name)};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        return std::unexpected{maybe_tileset.error()};
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. If `PorymapTilesetComponent` is empty, bail with error.
    if (tileset->porymap_component() == nullptr) {
        return std::unexpected{"PorymapTilesetComponent was empty"};
    }

    // 4. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If
    // any differ, bail with the message "uncompiled changes present in Porytiles asset X."
    if (tileset->porytiles_component() != nullptr) {
        const auto porytiles_keys = tileset_repo_->metadata_provider().get_porytiles_artifact_keys(tileset_name);
        const auto mismatched_keys =
            tileset_repo_->metadata_provider().find_unsynced_artifacts(tileset_name, porytiles_keys);
        if (!mismatched_keys.empty()) {
            return std::unexpected{"uncompiled changes present in Porytiles assets: TODO keys here"};
        }
    }

    // 5. If all `PorymapTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."
    const auto porymap_keys = tileset_repo_->metadata_provider().get_porymap_artifact_keys(tileset_name);
    if (tileset_repo_->metadata_provider().all_checksums_match(tileset_name, porymap_keys)) {
        // TODO: display a nothing_to_do message to the user
        return {};
    }

    // 6. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.

    // 7. Perform an incremental compilation.

    // 8. Persist the `Tileset` (which also caches the checksums).

    return {};
}

} // namespace porytiles2
