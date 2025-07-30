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

    // 4. Compute current checksums and fetch cached checksums for the `Tileset`.
    auto checksums = tileset_repo_->metadata_provider().compute_artifact_checksums(tileset_name);
    auto cached_checksums = tileset_repo_->metadata_provider().load_cached_checksums(tileset_name);

    // 5. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If
    // any differ, bail with the message "uncompiled changes present in Porytiles asset X."
    if (tileset->porytiles_component() != nullptr) {
        const auto porytiles_keys = tileset_repo_->metadata_provider().get_porytiles_artifact_keys(tileset_name);
        for (const auto &key : porytiles_keys) {
            auto checksum_for_key = checksums.contains(key) ? checksums.at(key) : "";
            auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
            if (checksum_for_key != cached_checksum_for_key) {
                return std::unexpected{fmt::format("uncompiled changes present in Porytiles asset {}", key)};
            }
        }
    }

    // 6. If all `PorymapTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."

    // 7. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.

    // 8. Perform an incremental compilation.

    // 9. Persist the `Tileset` (which also caches the checksums).

    return {};
}

} // namespace porytiles2
