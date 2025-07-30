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
    if (tileset->porytiles_component() == nullptr) {
        return std::unexpected{"PorytilesTilesetComponent was empty"};
    }

    // 4. Compute current checksums and fetch cached checksums for the `Tileset`.
    auto checksums = tileset_repo_->metadata_provider().compute_artifact_checksums(tileset_name);
    auto cached_checksums = tileset_repo_->metadata_provider().load_cached_checksums(tileset_name);

    // 5. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If any
    // differ, bail with the message "unimported changes present in Porymap asset X."
    if (tileset->porymap_component() != nullptr) {
        const auto porymap_keys = tileset_repo_->metadata_provider().get_porymap_artifact_keys(tileset_name);
        for (const auto &key : porymap_keys) {
            auto checksum_for_key = checksums.contains(key) ? checksums.at(key) : "";
            auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
            // TODO: more specific error message if one of the above is actually empty
            if (checksum_for_key != cached_checksum_for_key) {
                return std::unexpected{fmt::format("unimported changes present in Porymap asset {}", key)};
            }
        }
    }

    // 6. If all `PorytilesTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the
    // message "nothing to do."
    const auto porytiles_keys = tileset_repo_->metadata_provider().get_porytiles_artifact_keys(tileset_name);
    auto has_checksum_changed = [&checksums, &cached_checksums](const auto &key) {
        auto checksum_for_key = checksums.contains(key) ? checksums.at(key) : "";
        auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
        return checksum_for_key != cached_checksum_for_key;
    };
    if (std::ranges::none_of(porytiles_keys, has_checksum_changed)) {
        // TODO: display a nothing_to_do message to the user
        return {};
    }

    // 7. Compile the `PorytilesTilesetComponent`, generating a new `PorymapTilesetComponent`.
    const auto porytiles_component = tileset->porytiles_component();
    auto maybe_porymap_component = compiler_->compile(*porytiles_component);
    if (!maybe_porymap_component.has_value()) {
        return std::unexpected{maybe_porymap_component.error()};
    }
    auto porymap_component = std::move(maybe_porymap_component.value());
    tileset->porymap_component(std::move(porymap_component));

    // 8. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*tileset); !save_result.has_value()) {
        return std::unexpected{save_result.error()};
    }

    return {};
}

} // namespace porytiles2
