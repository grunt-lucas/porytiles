#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const {

    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return std::unexpected{"tileset does not exist"};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        return std::unexpected{maybe_tileset.error()};
    }
    const auto tileset = std::move(maybe_tileset.value());

    // 3. Compute checksums for each asset in `PorymapTilesetComponent`.
    auto current_checksums = metadata_provider_->compute_porymap_checksums(*tileset);
    auto stored_checksums = metadata_provider_->load_stored_checksums(tileset_name);

    // 4. Compare with cached checksums, if any differ, bail
    for (const auto &[artifact, current_sum] : current_checksums) {
        if (stored_checksums.contains(artifact) && stored_checksums[artifact] != current_sum) {
            // TODO : instead of bailing early, collect all errors and bail at the end
            return std::unexpected{"unimported changes present in Porymap asset " + artifact};
        }
    }
    // 5. If all match, continue.

    // 6. If the newest Porymap asset "modified" timestamp is newer than the newest Porytiles asset "modified"
    // timestamp, bail with "nothing to do."
    if (metadata_provider_->are_porymap_assets_newer(tileset->name())) {
        // TODO : display this message to the user
        // nothing to do - Porymap assets are newer than Porytiles assets
        return {};
    }

    // 7. Otherwise, compile the `PorytilesTilesetComponent`, generating a new `PorymapTilesetComponent`.
    const auto porytiles_component = tileset->porytiles_component();
    auto maybe_porymap_component = compiler_->compile(porytiles_component);
    if (!maybe_porymap_component.has_value()) {
        return std::unexpected{maybe_porymap_component.error()};
    }
    auto porymap_component = std::move(maybe_porymap_component.value());
    tileset->porymap_component(std::move(porymap_component));

    // 8. Compute new artifact checksums.
    auto new_checksums = metadata_provider_->compute_porymap_checksums(*tileset);
    auto store_checksum_result = metadata_provider_->store_checksums(tileset_name, new_checksums);
    if (!store_checksum_result.has_value()) {
        return std::unexpected{store_checksum_result.error()};
    }

    // 9. Persist the `Tileset` aggregate.
    if (const auto save_result = tileset_repo_->save(*tileset); !save_result.has_value()) {
        return std::unexpected{save_result.error()};
    }

    return {};
}

} // namespace porytiles2
