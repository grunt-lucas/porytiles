#include "porytiles2/app/use_cases/create_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/templates/result.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"
#include "porytiles2/xcut/result/text_formatter.hpp"

namespace porytiles2 {

Result<void> CreatePrimaryTileset::create(const std::string &tileset_name) const
{
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

    // 3. Initialize a blank `PorymapTilesetComponent`, to be filled later.
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();

    // 4. Initialize a `Tileset` aggregate with the components.
    Tileset tileset{tileset_name, std::move(porytiles_component), std::move(porymap_component)};

    // 5. Compile the `Tileset`, generating a new modified `Tileset`.
    auto maybe_new_tileset = compiler_->compile(tileset);
    if (!maybe_new_tileset.has_value()) {
        return std::unexpected{maybe_new_tileset.error().details(TextFormatter{false})};
    }
    const auto new_tileset = std::move(maybe_new_tileset.value());

    // 6. Update the source and header files.
    // TODO: this should use some kind of capable C source modification utility
    // if (const auto header_update_result = file_modifier_->append_tileset_declarations(tileset_name);
    //     !header_update_result.has_value()) {
    //     return std::unexpected{header_update_result.error()};
    // }

    // 7. Persist the new `Tileset` (which also caches the checksums).
    if (const auto save_result = tileset_repo_->save(*new_tileset); !save_result.has_value()) {
        return std::unexpected{save_result.error().details(TextFormatter{false})};
    }

    return {};
}

} // namespace porytiles2
