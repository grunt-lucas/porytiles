#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include "porytiles2/domain/algorithms/palette_matchers.hpp"

#include <array>
#include <memory>
#include <ranges>
#include <vector>

#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/config/patch_mode.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/canonical_shape_tile.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tiles_png_workspace.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/metatile_decompiler.hpp"
#include "porytiles2/domain/services/metatile_validator.hpp"
#include "porytiles2/domain/services/pack_set_generator.hpp"
#include "porytiles2/utilities/functional/transform.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_validators.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

#include <iostream>
#include <unordered_set>

namespace {

using namespace porytiles2;

} // namespace

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile(const Tileset &tileset)
{
    // Initialize all the compilation services
    LayerImageMetatileizer<Rgba32> metatileizer{};
    MetatileValidator validator{format_, diag_, tile_printer_, pal_printer_, config_, tileset.name()};
    LayerModeConverter layer_converter{format_, diag_, tile_printer_};

    // Grab configuration values we'll need
    PT_UNWRAP_TILESET_CONFIG(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_metatiles_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_tiles_per_metatile, tileset.name(), std::unique_ptr<Tileset>);

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
        validator.validate_primary(metatiles),
        "encountered error(s) while validating Porytiles metatiles",
        std::unique_ptr<Tileset>);

    /*
     * TODO: compute the inferred layer type and save it into a vector for later. Above, we already validated that all
     * metatiles satisfy the layer type constraint.
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
    auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile);

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
PrimaryTilesetCompiler::compile_patch(const Tileset &tileset, PatchTilesMode tiles_mode, PatchPalMode pal_mode)
{
    // Initialize all the compilation services
    LayerImageMetatileizer<Rgba32> metatileizer{};
    MetatileValidator validator{format_, diag_, tile_printer_, pal_printer_, config_, tileset.name()};
    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_};
    MetatileDecompiler metatile_decompiler{format_, diag_, tile_printer_};

    // Grab configuration values we'll need
    PT_UNWRAP_TILESET_CONFIG(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_pals_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_pals_total, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_metatiles_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_tiles_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG(config_, num_tiles_per_metatile, tileset.name(), std::unique_ptr<Tileset>);

    // Read Porytiles layer images into metatile vector
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
        validator.validate_primary(porytiles_metatiles),
        "encountered error(s) while validating Porytiles metatiles",
        std::unique_ptr<Tileset>);

    // Decompose Porytiles metatiles and generate canonical versions
    std::vector<PixelTile<Rgba32>> porytiles_pixel_rgba = metatile::decompose(porytiles_metatiles);
    std::vector<CanonicalPixelTile<Rgba32>> porytiles_canonical_pixel_rgba =
        transform<CanonicalPixelTile<Rgba32>>(porytiles_pixel_rgba);

    // Decompile Porymap tilemap entries and decompose into tile vector
    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_tilemap_entries,
        layer_mode_converter.triple_layerize(tileset.porymap_component()),
        "failed to triple-layerize Porymap component for tileset " + tileset.name(),
        std::unique_ptr<Tileset>);
    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_metatiles,
        metatile_decompiler.decompile_metatiles(
            porymap_tilemap_entries, tileset.porymap_component().tiles_png(), tileset.porymap_component().pals()),
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
    // ColorIndexMap color_index_map{porytiles_pixel_rgba, extrinsic_transparency.value()};

    /*
     * Create canonical ShapeTile vectors from porytiles input. Create canonical ShapeTile vectors from porytiles input.
     * We don't actually need this for tiles:fixed pals:fixed builds. We don't actually need this for tiles:fixed
     * pals:fixed builds. But if we were going to do pal assignment, we'd need
     * std::vector<CanonicalShapeTile<ColorIndex>>. If pals weren't fixed, here we'd want to do bin packing to get new
     * colors into the pals with the Porymap pals used as overrides in the packing process.
     */
    // std::vector<CanonicalShapeTile<ColorIndex>> porytiles_canonical_color_index_shapes =
    //     transform(porytiles_pixel_rgba, [&color_index_map, &extrinsic_transparency](const PixelTile<Rgba32> &tile) {
    //         return CanonicalShapeTile{from_pixel_tile(tile, color_index_map, extrinsic_transparency.value())};
    //     });
    // std::vector<CanonicalShapeTile<Rgba32>> porytiles_canonical_rgba_shapes = transform(
    //     porytiles_canonical_color_index_shapes, [&color_index_map](const CanonicalShapeTile<ColorIndex> &tile) {
    //         return CanonicalShapeTile{shape_tile_to_pixel_colors(tile, color_index_map)};
    //     });

    // TODO: import command should have already normalized transparency in slot 0 to extrinsic_transparency
    // TODO: PaletteValidator: throw error if pal isn't size 16
    // TODO: PaletteValidator: throw warning if slot 0 doesn't match current extrinsic_transparency
    std::vector<Palette<Rgba32>> porymap_pals{};
    porymap_pals.reserve(num_pals_primary.value());
    for (unsigned int i = 0; i < num_pals_primary.value(); i++) {
        auto pal_copy = tileset.porymap_component().pals()[i];
        pal_copy.set(extrinsic_transparency, 0);
        porymap_pals.push_back(pal_copy);
    }

    // TODO: The resulting PorymapTilesetComponent may be incomplete. E.g., the user may have specified PLA
    // files; they will be present on disk. We don't want to clobber them when saving the newly compiled
    // component. So we'll need to pull them from the original component and inject them into this one before
    // returning. We should probably add PLA file handling to the Tileset repository aggregate root. That way. all
    // this is handled automatically via the save/load abstraction mechanisms. PLA files are a first-class domain
    // concept, so they should be handled like any other file type (e.g. pal files, override files, etc). If we do that,
    // then here, instead of making a new PorymapComponent, we can invoke the copy ctor. And then we should add explicit
    // "reset" functions for the tilemap entries, tiles.png, pals, etc to clear the old values.
    auto new_porymap_component = std::make_unique<PorymapTilesetComponent>();

    // Precondition: all these decomposed tile vectors have the same size
    assert_or_panic(
        porytiles_pixel_rgba.size() == porymap_pixel_rgba.size(),
        "porytiles_pixel_rgba.size() != porymap_pixel_rgba.size()");
    assert_or_panic(
        porytiles_pixel_rgba.size() == porytiles_canonical_pixel_rgba.size(),
        "porytiles_pixel_rgba.size() != porytiles_canonical_pixel_rgba.size()");
    assert_or_panic(
        porymap_pixel_rgba.size() == porymap_canonical_pixel_rgba.size(),
        "porymap_pixel_rgba.size() != porymap_canonical_pixel_rgba.size()");
    assert_or_panic(
        porymap_tilemap_entries.size() == porymap_pixel_rgba.size(),
        "porymap_tilemap_entries.size() == porymap_pixel_rgba.size()");

    TilesPngWorkspace tiles_workspace{tileset.porymap_component().tiles_png(), num_tiles_primary};
    for (std::size_t i = 0; i < porytiles_pixel_rgba.size(); i++) {
        const auto &porytiles_tile = porytiles_pixel_rgba[i];
        const auto &porymap_tile = porymap_pixel_rgba[i];
        const auto &canonical_porytiles_tile = porytiles_canonical_pixel_rgba[i];
        const auto &canonical_porymap_tile = porymap_canonical_pixel_rgba[i];
        const auto &porymap_tilemap_entry = porymap_tilemap_entries[i];

        // CASE: Porytiles component tile exactly matches Porymap component, emit original metatile entry
        if (porytiles_tile.equals_ignoring_transparency(porymap_tile, extrinsic_transparency)) {
            new_porymap_component->push_back_tilemap_entry(porymap_tilemap_entry);
        }
        // CASE: Porytiles component tile matches Porymap component under flip transformation
        else if (canonical_porytiles_tile.equals_ignoring_transparency(
                     canonical_porymap_tile, extrinsic_transparency)) {
            auto [metatile_index, layer, subtile] = metatile::from_tile_index(i);
            std::vector<std::string> pt_note{};
            pt_note.emplace_back(format_->format(
                "TODO: FLIP-ISO MATCH: {} {} {}", FormatParam{i}, FormatParam{layer}, FormatParam{subtile}));
            diag_->note("debug-porytiles", pt_note);
        }
        // CASE: New tile, compute which pal to use, compute (or create) tile to use
        else {
            // TODO: for tiles:fixed pals:fixed, here we need to try computing which existing tile+pal we can use.
            auto [metatile_index, layer, subtile] = metatile::from_tile_index(i);
            std::vector<std::string> pt_note{};
            pt_note.emplace_back(format_->format(
                "TODO: HANDLE NEW TILE CASE: {} {} {}", FormatParam{i}, FormatParam{layer}, FormatParam{subtile}));
            pt_note.emplace_back(format_->format("{} {} {}", FormatParam{i}, FormatParam{layer}, FormatParam{subtile}));
            pt_note.emplace_back(format_->format("{} {} {}", FormatParam{i}, FormatParam{layer}, FormatParam{subtile}));
            // std::ranges::copy(
            //     tile_printer_->print_metatile(porytiles_metatiles.at(metatile_index), layer, subtile),
            //     std::back_inserter(pt_note));
            // std::ranges::copy(
            //     tile_printer_->print_metatile(porymap_metatiles.at(metatile_index), layer, subtile),
            //     std::back_inserter(pm_note));
            std::ranges::copy(tile_printer_->print_tile(canonical_porytiles_tile), std::back_inserter(pt_note));
            pt_note.emplace_back();
            std::ranges::copy(tile_printer_->print_tile(canonical_porymap_tile), std::back_inserter(pt_note));
            diag_->note("debug-porytiles", pt_note);
        }
    }

    // No changes here, this is a compilation operation and there should be no writebacks into the input assets.
    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset.porytiles_component());

    /*
     * If user is requesting dual-layer, use the input Porytiles-format metatiles to infer the LayerType for each
     * metatile and remove the relevant tilemap entries. Here, we assume that the Porytiles metatiles have already been
     * validated in an earlier step as dual-layer compatible.
     */
    const auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile);
    if (configured_layer_mode == LayerMode::dual) {
        const auto &dual_layerized =
            layer_mode_converter.dual_layerize(new_porymap_component->metatiles_bin(), porytiles_metatiles);
        new_porymap_component->metatiles_bin(dual_layerized);
    }

    // TODO: write attributes for real, for now just write back what we read
    for (const auto &attr : tileset.porymap_component().metatile_attributes_bin()) {
        new_porymap_component->push_back_attribute(attr);
    }

    // Export tiles in original form; could use ExportTrimMode::trim_trailing_transparent to remove padding
    new_porymap_component->tiles_png(tiles_workspace.export_image(ExportFlipMode::original));

    // Copy primary palettes from our processed porymap_pals vector
    for (unsigned int i = 0; i < num_pals_primary.value(); i++) {
        new_porymap_component->set_pal(porymap_pals[i], i);
    }

    /*
     * Copy remaining secondary palettes from the original component. The "secondary" pals in a primary tileset's folder
     * won't be actually loaded by the game engine. Porymap also doesn't show them -- it will grab pals from the
     * relevant secondary set folder. However, we copy them here for consistency. If for some reason the user had edited
     * them, we don't want to clobber their edits. Porytiles should be surgical where possible.
     */
    for (unsigned int i = num_pals_primary.value(); i < num_pals_total.value(); i++) {
        new_porymap_component->set_pal(tileset.porymap_component().pals()[i], i);
    }

    /*
     * Copy junk pals. 13.pal, 14.pal, 15.pal exist in the tileset but are reserved by the game engine for
     * overworld/shop UI. Here we just copy them over as-is. Again, if for some reason the user had edited them, let's
     * not clobber anything unnecessarily.
     */
    for (unsigned int i = num_pals_total.value(); i < pal::num_pals; i++) {
        new_porymap_component->set_pal(tileset.porymap_component().pals()[i], i);
    }

    // Create the full Tileset and return
    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(new_porytiles_component), std::move(new_porymap_component));

    return new_tileset;
}

} // namespace porytiles2
