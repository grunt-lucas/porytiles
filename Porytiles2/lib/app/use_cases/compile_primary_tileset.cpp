#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const {
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

    // 3. If `PorytilesTilesetComponent` is empty, bail with error.

    // 4. Compute checksums for the `Tileset`.

    // 5. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If any
    // differ, bail with the message "unimported changes present in Porymap asset X."

    // 6. If all `PorytilesTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."

    // 7. Compile the `PorytilesTilesetComponent`, generating a new `PorymapTilesetComponent`.

    // 8. Persist the `Tileset` (which also caches the checksums).

    return {};
}

} // namespace porytiles2
