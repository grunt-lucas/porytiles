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

    // 4. Compute checksums for the `Tileset`.

    // 5. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If
    // any differ, bail with the message "uncompiled changes present in Porytiles asset X."

    // 6. If all `PorymapTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."

    // 7. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.

    // 8. Perform an incremental compilation.

    // 9. Persist the `Tileset` (which also caches the checksums).

    return {};
}

} // namespace porytiles2
