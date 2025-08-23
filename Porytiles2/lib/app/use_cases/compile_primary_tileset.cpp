#include "porytiles2/app/use_cases/compile_primary_tileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> CompilePrimaryTileset::compile(const std::string &tileset_name) const
{
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
    if (tileset->porytiles_component().is_empty()) {
        return std::unexpected{"PorytilesTilesetComponent was empty"};
    }

    // 4. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If any
    // differ, bail with the message "unimported changes present in Porymap asset X."
    if (!tileset->porymap_component().is_empty()) {
        const auto porymap_keys = tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name);
        const auto mismatched_keys =
            tileset_repo_->checksum_provider().find_unsynced_artifacts(tileset_name, porymap_keys);
        if (!mismatched_keys.empty()) {
            return std::unexpected{"unimported changes present in Porymap assets: TODO keys here"};
        }
    }

    // 5. If all `PorytilesTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."
    const auto porytiles_keys = tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name);
    if (tileset_repo_->checksum_provider().all_checksums_match(tileset_name, porytiles_keys)) {
        // TODO: display a nothing_to_do message to the user
        return {};
    }

    // 6. Compile the `PorytilesTilesetComponent`, generating a new `PorymapTilesetComponent`.
    const auto &porytiles_component = tileset->porytiles_component();
    auto maybe_porymap_component = compiler_->compile(porytiles_component);
    if (!maybe_porymap_component.has_value()) {
        return std::unexpected{maybe_porymap_component.error()};
    }
    auto porymap_component = std::move(maybe_porymap_component.value());
    // TODO: The resulting PorymapTilesetComponent may be incomplete. E.g., the user may have specified PLA
    // files; they will be present on disk. We don't want to clobber them when saving the newly compiled
    // component. So we'll need to pull them from the original component and inject them into this one before
    // persisting. One way around this would be to add PLA files to the Porytiles component. Compilation can simply copy
    // them over. We'll also have to handle this on the import side. That is, when importing a tileset that contains PLA
    // files, we need to make sure to copy them into the new Porytiles component.
    tileset->porymap_component(std::move(porymap_component));

    // 7. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*tileset); !save_result.has_value()) {
        return std::unexpected{save_result.error()};
    }

    return {};
}

} // namespace porytiles2
