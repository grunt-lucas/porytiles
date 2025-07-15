#include "porytiles2/app/create_primary_tileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> CreatePrimaryTileset::create(const std::string &tileset_name) const {
    // 1. Check if the primary tileset already exists. If so, abort with an error message.
    if (tileset_repo_->exists(tileset_name)) {
        return std::unexpected{"tileset already exists"};
    }

    // 2. Initialize a `PorytilesTilesetComponent` with default assets.
    auto maybe_porytiles_component = asset_generator_->generate();
    if (!maybe_porytiles_component.has_value()) {
        return std::unexpected{maybe_porytiles_component.error()};
    }
    auto porytiles_component = std::move(maybe_porytiles_component.value());

    // 3. Compile the `PorytilesTilesetComponent` to generate an initial `PorymapTilesetComponent`.
    auto maybe_porymap_component = compiler_->compile(*porytiles_component);
    if (!maybe_porymap_component.has_value()) {
        return std::unexpected{maybe_porymap_component.error()};
    }
    auto porymap_component = std::move(maybe_porymap_component.value());

    // 4. Initialize a new `Tileset` aggregate with the components, also generating the initial artifact checksums.
    Tileset tileset{std::move(porytiles_component), std::move(porymap_component)};

    // 5. Persist the `Tileset` (including updating the right source/header files and generating checksums).
    auto new_checksums = metadata_provider_->compute_porymap_checksums(tileset);
    if (auto store_checksum_result = metadata_provider_->store_checksums(tileset_name, new_checksums);
        !store_checksum_result.has_value()) {
        return std::unexpected{store_checksum_result.error()};
    }
    if (const auto save_result = tileset_repo_->save(tileset); !save_result.has_value()) {
        return std::unexpected{save_result.error()};
    }
    /*
     * TODO : use HeaderFileParser, CSourceGenerator, and CSourceFileModifier to update graphics.h, headers.h, and
     * metatiles.h
     */

    return {};
}

} // namespace porytiles2
