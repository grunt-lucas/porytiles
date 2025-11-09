#include "porytiles2/app/use_cases/create_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {

ChainableResult<void> CreatePrimaryTileset::create(const std::string &tileset_name) const
{
    // 1. Check if the primary tileset already exists. If so, abort with an error message.
    if (tileset_repo_->exists(tileset_name)) {
        return FormattableError{"tileset already exists"};
    }

    // 2. Initialize a `PorytilesTilesetComponent` with default assets.
    auto maybe_porytiles_component = asset_generator_->generate();
    if (!maybe_porytiles_component.has_value()) {
        return FormattableError{maybe_porytiles_component.error()};
    }
    auto porytiles_component = std::move(maybe_porytiles_component.value());

    // 3. Initialize a blank `PorymapTilesetComponent`, to be filled later.
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();

    // 4. Initialize a `Tileset` aggregate with the components.
    Tileset tileset{tileset_name, std::move(porytiles_component), std::move(porymap_component)};

    // 5. Compile the `Tileset`, generating a new modified `Tileset`.
    auto maybe_new_tileset = compiler_->compile(tileset);
    if (!maybe_new_tileset.has_value()) {
        auto error_lines = maybe_new_tileset.error().details(PlainTextFormatter{});
        std::string joined_error;
        for (std::size_t i = 0; i < error_lines.size(); ++i) {
            if (i > 0) {
                joined_error += "\n";
            }
            joined_error += error_lines[i];
        }
        return FormattableError{joined_error};
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
        auto error_lines = save_result.error().details(PlainTextFormatter{});
        std::string joined_error;
        for (std::size_t i = 0; i < error_lines.size(); ++i) {
            if (i > 0) {
                joined_error += "\n";
            }
            joined_error += error_lines[i];
        }
        return FormattableError{joined_error};
    }

    return {};
}

} // namespace porytiles2
