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

    // 3. If `PorytilesTilesetComponent` is not empty (i.e., a `porytiles` folder exists), and the oldest Porytiles
    // asset is newer than the newest Porymap asset, bail with the message "uncompiled changes in Porytiles asset X."

    // 4. Import the Porymap assets into the `PorymapTilesetComponent` and compute checksums for each.

    // 5. If `PorytilesTilesetComponent` is not empty and all checksums match, bail with the message "nothing to do."

    // 6. Perform a complete decompilation.

    // 7. Fill in the `PorytilesTilesetComponent` with the decompiled assets.

    // 8. Perform an incremental compilation and re-store checksums.

    // 9. Persist the `Tileset` aggregate.

    return {};
}

} // namespace porytiles2
