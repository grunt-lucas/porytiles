#include "porytiles2/app/use_cases/create_primary_tileset.hpp"

#include <memory>
#include <string>

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

    // 4. Initialize a new `Tileset` aggregate with the components.
    Tileset tileset{std::move(porytiles_component), std::move(porymap_component)};

    // 5. Update the source and header files.
    // TODO : this should use HeaderFileParser for more sophisticated error handling
    if (const auto header_update_result = file_modifier_->append_tileset_declarations(tileset_name);
        !header_update_result.has_value()) {
        return std::unexpected{header_update_result.error()};
    }

    // 6. Persist the `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(tileset); !save_result.has_value()) {
        return std::unexpected{save_result.error()};
    }

    return {};
}

} // namespace porytiles2
