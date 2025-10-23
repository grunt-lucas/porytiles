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
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/pack_set_generator.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/tile_validator.hpp"
#include "porytiles2/utilities/unwrap_config.hpp"
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
    LayerModeConverter layer_converter{format_, diag_, tile_printer_};

    /*
     * TODO: here, we need to check if dual-layer output is enabled. If so, throw an error if any metatile has
     * non-transparent content on all three layers. Otherwise, use the present layers to attempt to infer the layer type
     * automatically and save it off to a vector.
     */

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
     * TODO: our first step in any compilation operation should validate the LayerMode. First, we need to check the
     * num_tiles_per_metatile configuration setting. If it's set to 8, the user is requesting dual-layer compilation. If
     * it's 12, triple. Any other value will have been caught earlier by config validation. If it's 8, then we need to
     * check the input metatiles and throw an error for all metatiles that have non-transparent content on all three
     * layers. While doing this, we can also compute the inferred layer type and save it into a vector for later.
     *
     * If it's 12, then we're good, just move on. No need to validate or do anything special for the inferred layer type
     * vector. Just set it to LayerType::normal and move on.
     *
     * Since we'll be overwriting the output tilemap entries and attributes as part of the compilation operation, no
     * need to validate them via LayerModeConverter::detect_layer_mode at this point. (Let's really think through this.
     * Would we want to warn the user somewhere if the Porymap component metatiles are corrupt? Obviously in the
     * decompilation operations this is an error condition.)
     */

    // TODO: remove, here for testing
    PT_TRY_CALL_CHAIN_ERR(
        layer_converter.detect_layer_mode(
            tileset.porymap_component().metatiles_bin(), tileset.porymap_component().metatile_attributes()),
        "layer mode detection failed",
        std::unique_ptr<Tileset>);

    // TODO: remove these, just here to test config/diagnostic stuff
    PT_UNWRAP_SCOPED_CONFIG(config_, num_tiles_primary, tileset.name(), std::unique_ptr<Tileset>);
    diag_->note(
        std::vector{
            format_->format(
                "{} {}",
                FormatParam{num_tiles_primary.name() + ":", Style::bold},
                FormatParam{num_tiles_primary, Style::bold}),
            format_->format("({})", num_tiles_primary.source()),
            std::string{"foo"},
            std::string{"bar"}});
    PT_UNWRAP_SCOPED_CONFIG(config_, num_tiles_secondary, tileset.name(), std::unique_ptr<Tileset>);
    diag_->note(
        std::vector{
            format_->format(
                "{} {}",
                FormatParam{num_tiles_secondary.name() + ":", Style::bold},
                FormatParam{num_tiles_secondary, Style::bold}),
            format_->format("({})", num_tiles_secondary.source()),
            std::string{"foo"},
            std::string{"bar"}});
    diag_->warn("test-warning", std::vector{std::string{"foo"}, std::string{"bar"}, std::string{"baz"}});
    diag_->warn_note("test-warning", std::vector{std::string{"foo"}, std::string{"bar"}, std::string{"baz"}});
    diag_->err(std::vector{std::string{"foo"}, std::string{"bar"}, std::string{"baz"}});

    // Leaf step to throw error if there are too many metatiles.
    PT_UNWRAP_SCOPED_CONFIG(config_, num_metatiles_primary, tileset.name(), std::unique_ptr<Tileset>);
    if (metatiles.size() > num_metatiles_primary.value()) {
        return FormattableError{
            "too many input metatiles: found '{}' > '{}' (num_metatiles_primary)",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{num_metatiles_primary, Style::bold}};
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
