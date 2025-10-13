#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <array>
#include <memory>
#include <ranges>
#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/color_index_map_builder.hpp"
#include "porytiles2/domain/services/pack_set_generator.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/tile_validator.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

#include <unordered_set>

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile(const Tileset &tileset)
{
    // Initialize all the compilation services
    RgbaLayerImageMetatileizer metatileizer{};
    ColorIndexMapBuilder color_index_map_builder{};
    TileValidator validator{format_, diag_, tile_printer_};

    // Convert layer images into vector<RgbaMetatile>
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatileizer.metatileize(
            tileset.porytiles_component().bottom(),
            tileset.porytiles_component().middle(),
            tileset.porytiles_component().top()),
        "failed to metatileize input layer images",
        std::unique_ptr<Tileset>);

    diag_->note(format_->format(
        "{} {} ({})",
        FormatParam{config_->num_metatiles_primary(tileset.name()).name() + ":", Style::bold},
        FormatParam{config_->num_metatiles_primary(tileset.name())},
        FormatParam{config_->num_metatiles_primary(tileset.name()).source(), Style::bold}));
    diag_->note(format_->format(
        "{} {} ({})",
        FormatParam{config_->num_tiles_secondary(tileset.name()).name() + ":", Style::bold},
        FormatParam{config_->num_tiles_secondary(tileset.name())},
        FormatParam{config_->num_tiles_secondary(tileset.name()).source(), Style::bold}));
    diag_->note(format_->format(
        "{} {} ({})",
        FormatParam{config_->num_metatiles_secondary(tileset.name()).name() + ":", Style::bold},
        FormatParam{config_->num_metatiles_secondary(tileset.name())},
        FormatParam{config_->num_metatiles_secondary(tileset.name()).source(), Style::bold}));
    diag_->note(format_->format(
        "{} {} ({})",
        FormatParam{config_->num_pals_secondary(tileset.name()).name() + ":", Style::bold},
        FormatParam{config_->num_pals_secondary(tileset.name())},
        FormatParam{config_->num_pals_secondary(tileset.name()).source(), Style::bold}));

    // Leaf step to throw error if there are too many metatiles.
    if (metatiles.size() > config_->num_metatiles_primary(tileset.name())) {
        return FormattableError{
            "too many input metatiles: found '{}' > '{}' (num_metatiles_primary)",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{config_->num_metatiles_primary(tileset.name()), Style::bold}};
    }

    // Decompose vector<RgbaMetatile> into vector<RgbaTile>
    std::vector<RgbaTile> tiles{};
    tiles.reserve(metatiles.size() * metatile::tiles_per_metatile);
    for (const auto &metatile : metatiles) {
        const auto decomposed = metatile.decompose();
        for (const auto &pixel_tile : decomposed) {
            tiles.emplace_back(pixel_tile);
        }
    }

    // Leaf step to throw errors if:
    // - any tiles contain an invalid alpha value
    // - any tiles have more than 15+1 colors
    // - generate precision loss warnings if some colors collapse to the same 5-bit color
    PT_TRY_CALL_CHAIN_ERR(validator.validate_alpha_channels(tiles), "tile validation error", std::unique_ptr<Tileset>);
    PT_TRY_CALL_CHAIN_ERR(
        validator.validate_unique_color_count(tiles, config_->extrinsic_transparency(tileset.name())),
        "tile validation error",
        std::unique_ptr<Tileset>);
    PT_TRY_CALL_CHAIN_ERR(
        validator.generate_precision_loss_warnings(tiles), "tile validation error", std::unique_ptr<Tileset>);

    // Create color index map from vector<RgbaTile>
    // TODO: impl

    // Create PackSets for the bin packing step
    // const auto &color_index_map = color_index_map_builder.build_map(norm_tiles, rgba_magenta);
    // ColorSetBuilder color_set_builder{text_formatter_};
    // PackSetGenerator assignable_tile_generator{&color_set_builder};
    // std::vector<PackSet> assignable_tiles = assignable_tile_generator.generate(norm_tiles, color_index_map);

    // TODO: set up these components correctly, for now we just use some dummy values
    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset.porytiles_component());

    // TODO: The resulting PorymapTilesetComponent may be incomplete. E.g., the user may have specified PLA
    // files; they will be present on disk. We don't want to clobber them when saving the newly compiled
    // component. So we'll need to pull them from the original component and inject them into this one before
    // returning. We should probably add PLA file handling to the Tileset repository aggregate root. That way. all
    // this is handled automatically via the save/load abstraction mechanisms. PLA files are a first-class domain
    // concept, so they should be handled like any other file type (e.g. pal files, override files, etc).
    auto new_porymap_component = std::make_unique<PorymapTilesetComponent>();
    new_porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, false, false});
    new_porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, true, true});

    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(new_porytiles_component), std::move(new_porymap_component));

    return new_tileset;
}

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile_incremental(const Tileset &tileset)
{
    // TODO: implement for real
    // Pipeline pipeline{};
    panic("TODO: implement");
}

} // namespace porytiles2
