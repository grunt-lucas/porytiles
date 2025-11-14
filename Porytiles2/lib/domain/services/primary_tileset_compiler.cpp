#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <array>
#include <memory>
#include <ranges>
#include <vector>

#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/canonical_shape_tile.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/metatile_decompiler.hpp"
#include "porytiles2/domain/services/pack_set_generator.hpp"
#include "porytiles2/domain/services/tile_validator.hpp"
#include "porytiles2/utilities/functional/transform.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_validators.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

#include <unordered_set>

namespace {

using namespace porytiles2;

ChainableResult<void> validate_porytiles_metatiles(
    const DomainConfig *config,
    const std::string &tileset_name,
    const TileValidator &validator,
    const std::vector<Metatile<Rgba32>> &metatiles)
{
    // Unwrap configs we need
    PT_UNWRAP_TILESET_CONFIG(config, extrinsic_transparency, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG(config, num_metatiles_primary, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG(config, num_pals_primary, tileset_name, void);

    // Throw error if there are too many metatiles.
    if (metatiles.size() > num_metatiles_primary.value()) {
        return FormattableError{
            "too many input metatiles: found '{}' > '{}' (num_metatiles_primary)",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{num_metatiles_primary, Style::bold}};
    }

    // Run alpha channel validation
    PT_TRY_CALL_CHAIN_ERR(validator.validate_alpha_channels(metatiles), "alpha channel validation failed", void);

    // Run tile color count validation
    PT_TRY_CALL_CHAIN_ERR(
        validator.validate_tile_color_count(metatiles, extrinsic_transparency),
        "unique tile color count validation failed",
        void);

    // Run precision loss warning generation
    PT_TRY_CALL_CHAIN_ERR(
        validator.generate_precision_loss_warnings(metatiles), "precision loss validation failed", void);

    // Run global color count validation
    std::size_t color_count_limit = num_pals_primary.value() * (pal::max_size - 1);
    PT_TRY_CALL_CHAIN_ERR(
        validator.validate_global_color_count(metatiles, extrinsic_transparency, color_count_limit),
        "unique global color count validation failed",
        void);

    return {};
}

} // namespace

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile(const Tileset &tileset)
{
    // Initialize all the compilation services
    LayerImageMetatileizer<Rgba32> metatileizer{};
    TileValidator validator{format_, diag_, tile_printer_};
    LayerModeConverter layer_converter{format_, diag_, tile_printer_};

    // Grab configuration values we'll need
    PT_UNWRAP_TILESET_CONFIG(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);

    // Convert layer images into vector<RgbaMetatile>
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatileizer.metatileize(
            tileset.porytiles_component().bottom(),
            tileset.porytiles_component().middle(),
            tileset.porytiles_component().top()),
        "failed to metatileize input layer images for " + tileset.name(),
        std::unique_ptr<Tileset>);

    // Run validation on Porytiles metatiles
    PT_TRY_CALL_CHAIN_ERR(
        validate_porytiles_metatiles(config_, tileset.name(), validator, metatiles),
        "encountered error while validating Porytiles metatiles",
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
     * need to validate them via detect_layer_mode at this point. (Let's really think through this. Would we want to
     * warn the user somewhere if the Porymap component metatiles are corrupt? Obviously in the decompilation operations
     * this is an error condition.)
     */
    // TODO: remove, here for testing
    // PT_TRY_CALL_CHAIN_ERR(
    //     tileset.porymap_component().detect_layer_mode(), "layer mode detection failed", std::unique_ptr<Tileset>);

    // Create color index map from vector<RgbaTile>
    // TODO: impl

    // Throw error if ColorIndexMap has too many unique colors

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
    Palette pal{rgba_red};
    pal.set(extrinsic_transparency.value(), 0);

    new_porymap_component->tiles_png(tiles_png);
    for (unsigned int i = 0; i < pal::num_pals; i++) {
        new_porymap_component->set_pal(pal, i);
    }

    /*
     * TODO: here, we need to check if dual-layer output is enabled. If so, check the layer type and skip the empty
     * layer.
     */
    new_porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, false, false});
    new_porymap_component->push_back_tilemap_entry(TilemapEntry{1, 1, true, true});

    // TODO: write attributes for real, for now just write back what we read
    for (const auto &attr : tileset.porymap_component().metatile_attributes_bin()) {
        new_porymap_component->push_back_attribute(attr);
    }

    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(new_porytiles_component), std::move(new_porymap_component));

    return new_tileset;
}

ChainableResult<std::unique_ptr<Tileset>>
PrimaryTilesetCompiler::compile_patch_tiles_fixed_pals_fixed(const Tileset &tileset)
{
    // Initialize all the compilation services
    LayerImageMetatileizer<Rgba32> metatileizer{};
    TileValidator validator{format_, diag_, tile_printer_};
    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_};
    MetatileDecompiler metatile_decompiler{format_, diag_, tile_printer_};

    // Grab configuration values we'll need
    PT_UNWRAP_TILESET_CONFIG(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_pals_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_pals_total, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_metatiles_primary, tileset.name(), std::unique_ptr<Tileset>);

    // Read Porytiles layer images and decompose into tile vectors
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_metatiles,
        metatileizer.metatileize(
            tileset.porytiles_component().bottom(),
            tileset.porytiles_component().middle(),
            tileset.porytiles_component().top()),
        "failed to metatileize input layer images for " + tileset.name(),
        std::unique_ptr<Tileset>);

    // Run validation on Porytiles metatiles
    PT_TRY_CALL_CHAIN_ERR(
        validate_porytiles_metatiles(config_, tileset.name(), validator, porytiles_metatiles),
        "encountered error while validating Porytiles metatiles",
        std::unique_ptr<Tileset>);

    // Decompose Porytiles metatiles and generate canonical versions
    std::vector<PixelTile<Rgba32>> porytiles_pixel_rgba = metatile::decompose(porytiles_metatiles);
    std::vector<CanonicalPixelTile<Rgba32>> porytiles_canonical_pixel_rgba =
        transform<CanonicalPixelTile<Rgba32>>(porytiles_pixel_rgba);

    // Decompile Porymap tilemap entries and decompose into tile vector
    PT_TRY_ASSIGN_CHAIN_ERR(
        tilemap_entries,
        layer_mode_converter.triple_layerize(tileset.porymap_component()),
        "failed to triple-layerize Porymap component for tileset " + tileset.name(),
        std::unique_ptr<Tileset>);
    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_metatiles,
        metatile_decompiler.decompile_metatiles(
            tilemap_entries, tileset.porymap_component().tiles_png(), tileset.porymap_component().pals()),
        "failed to decompile Porymap component for tileset " + tileset.name(),
        std::unique_ptr<Tileset>);
    /*
     * We don't need to check porymap_metatiles size here. We're going to overwrite it anyway. We only need to check the
     * size of the final tilemap entry vector. Patch builds don't need to preserve tilemap entries since those cannot be
     * referenced by other tilesets.
     */

    // Decompose Porymap metatiles and generate canonical versions
    std::vector<PixelTile<Rgba32>> porymap_pixel_rgba = metatile::decompose(porymap_metatiles);
    std::vector<CanonicalPixelTile<Rgba32>> porymap_canonical_pixel_rgba =
        transform<CanonicalPixelTile<Rgba32>>(porymap_pixel_rgba);

    /*
     * Create ColorIndexMap from porytiles_tiles. We don't actually need a ColorIndexMap for a pals:fixed patch build.
     */
    ColorIndexMap color_index_map{porytiles_pixel_rgba, extrinsic_transparency.value()};
    std::size_t color_count = color_index_map.size();
    std::size_t color_count_limit = num_pals_primary.value() * (pal::max_size - 1);

    // Global color limit handling
    /*
     * TODO: we could handle this in a separate service-based step, kinda like the TileValidator service. It would be
     * nice to give users very detailed information about their global color count when they go over. Example, we could
     * print out a list of colors with their pixel counts, the first location of colors that went over the limit, etc.
     * This will really help users narrow down issues when they exceed color count.
     */
    if (color_count > color_count_limit) {
        // Emit error
        diag_->err(
            "color-limit-exceeded",
            format_->format(
                "too many unique colors ({}) in Porytiles component for tileset '{}'",
                FormatParam{color_count, Style::bold},
                FormatParam{tileset.name(), Style::bold}));

        // Construct note text
        std::vector<std::string> note_text;
        note_text.push_back(format_->format(
            "unique color count limit is '{}' due to configuration", FormatParam{color_count_limit, Style::bold}));
        note_text.emplace_back("");
        std::ranges::copy(num_pals_primary.prettify(*format_), std::back_inserter(note_text));
        note_text.emplace_back("");
        note_text.push_back(format_->format(
            "Color limit definition: {} * {}: {} * {}: {}",
            FormatParam{num_pals_primary.name(), Style::bold},
            FormatParam{"nontransparent_colors_per_pal", Style::bold},
            FormatParam{num_pals_primary.value(), Style::bold},
            FormatParam{(pal::max_size - 1), Style::bold},
            FormatParam{color_count_limit, Style::bold}));

        // Emit note
        diag_->note("color-limit-exceeded", note_text);
    }

    // Create canonical ShapeTile vectors from porytiles input
    // We don't actually need this for tiles:fixed pals:fixed builds.
    // But if we were going to do pal assignment, we'd need std::vector<CanonicalShapeTile<ColorIndex>>.
    // If pals weren't fixed, here we'd want to do bin packing to get new colors into the pals with the Porymap pals
    // used as overrides in the packing process.
    //
    // std::vector<CanonicalShapeTile<ColorIndex>> porytiles_canonical_color_index_shapes =
    //     transform(porytiles_pixel_rgba, [&color_index_map, &extrinsic_transparency](const PixelTile<Rgba32> &tile) {
    //         return CanonicalShapeTile{from_pixel_tile(tile, color_index_map, extrinsic_transparency.value())};
    //     });
    // std::vector<CanonicalShapeTile<Rgba32>> porytiles_canonical_rgba_shapes = transform(
    //     porytiles_canonical_color_index_shapes, [&color_index_map](const CanonicalShapeTile<ColorIndex> &tile) {
    //         return CanonicalShapeTile{shape_tile_to_pixel_colors(tile, color_index_map)};
    //     });

    // TODO: Copy in the Porymap pals then normalize transparency
    std::vector<unsigned int> pal_indexes;
    std::vector<Palette<Rgba32>> porymap_pals{};
    porymap_pals.reserve(num_pals_primary.value());
    for (unsigned int i = 0; i < num_pals_primary.value(); i++) {
        porymap_pals.push_back(tileset.porymap_component().pals()[i]);
    }

    panic("TODO: finish implementation");
}

} // namespace porytiles2
