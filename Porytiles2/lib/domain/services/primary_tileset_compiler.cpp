#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <array>
#include <memory>
#include <ranges>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/rgba_pal.hpp"
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

    /*
     * TODO: here, we need to check if dual-layer output is enabled. If so, throw an error if any metatile has
     * non-transparent content on all three layers. Otherwise, use the present layers to attempt to infer the layer type
     * automatically and save it off to a vector.
     */

    // TODO: remove these, just here to test config/diagnostic stuff
    PT_TRY_ASSIGN_CHAIN_ERR(
        num_tiles_primary_config,
        config_->num_tiles_primary(tileset.name()),
        "failed to get num_tiles_primary config",
        std::unique_ptr<Tileset>);
    diag_->note(
        std::vector{
            format_->format(
                "{} {}",
                FormatParam{num_tiles_primary_config.name() + ":", Style::bold},
                FormatParam{num_tiles_primary_config, Style::bold}),
            format_->format("({})", num_tiles_primary_config.source()),
            std::string{"foo"},
            std::string{"bar"}});
    PT_TRY_ASSIGN_CHAIN_ERR(
        num_tiles_secondary_config,
        config_->num_tiles_secondary(tileset.name()),
        "failed to get num_tiles_secondary config",
        std::unique_ptr<Tileset>);
    diag_->note(
        std::vector{
            format_->format(
                "{} {}",
                FormatParam{num_tiles_secondary_config.name() + ":", Style::bold},
                FormatParam{num_tiles_secondary_config, Style::bold}),
            format_->format("({})", num_tiles_secondary_config.source()),
            std::string{"foo"},
            std::string{"bar"}});
    diag_->warn("test-warning", std::vector{std::string{"foo"}, std::string{"bar"}, std::string{"baz"}});
    diag_->warn_note("test-warning", std::vector{std::string{"foo"}, std::string{"bar"}, std::string{"baz"}});
    diag_->err(std::vector{std::string{"foo"}, std::string{"bar"}, std::string{"baz"}});

    // Leaf step to throw error if there are too many metatiles.
    PT_TRY_ASSIGN_CHAIN_ERR(
        num_metatiles_primary_config,
        config_->num_metatiles_primary(tileset.name()),
        "failed to get num_metatiles_primary config",
        std::unique_ptr<Tileset>);
    if (metatiles.size() > num_metatiles_primary_config.value()) {
        return FormattableError{
            "too many input metatiles: found '{}' > '{}' (num_metatiles_primary)",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{num_metatiles_primary_config, Style::bold}};
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
    PT_TRY_ASSIGN_CHAIN_ERR(
        extrinsic_transparency_config,
        config_->extrinsic_transparency(tileset.name()),
        "failed to get extrinsic_transparency config",
        std::unique_ptr<Tileset>);
    PT_TRY_CALL_CHAIN_ERR(
        validator.validate_unique_color_count(tiles, extrinsic_transparency_config.value()),
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
    // concept, so they should be handled like any other file type (e.g. pal files, override files, etc). If we do that,
    // then here, instead of making a new PorymapComponent, we can invoke the copy ctor. And then we should add explicit
    // "reset" functions for the tilemap entries, tiles.png, pals, etc to clear the old values.
    auto new_porymap_component = std::make_unique<PorymapTilesetComponent>();

    Image<IndexPixel> tiles_png{128, 128};
    RgbaPal pal{rgba_red};
    pal.set(extrinsic_transparency_config.value(), 0);

    new_porymap_component->tiles_png(tiles_png);
    for (int i = 0; i < pal::num_pals; i++) {
        new_porymap_component->set_pal(pal, i);
    }

    /*
     * TODO: here, we need to check if dual-layer output is enabled. If so, check the layer type and skip the empty
     * layer.
     */
    new_porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, false, false});
    new_porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, true, true});

    // TODO: write attributes for real, for now just write back what we read
    for (const auto &attr : tileset.porymap_component().metatile_attributes()) {
        new_porymap_component->push_back_attribute(attr);
    }

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
