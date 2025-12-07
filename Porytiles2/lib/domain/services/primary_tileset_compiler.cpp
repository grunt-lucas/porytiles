#include "porytiles2/domain/services/primary_tileset_compiler.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <ranges>
#include <unordered_set>
#include <vector>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/algorithms/palette_matchers.hpp"
#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/config/artifact_edit_mode.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/canonical_shape_tile.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tiles_png_workspace.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/packing/services/best_fusion_strategy.hpp"
#include "porytiles2/domain/packing/services/palette_packer.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/metatile_decompiler.hpp"
#include "porytiles2/domain/services/metatile_validator.hpp"
#include "porytiles2/utilities/functional/transform.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_validators.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"
#include "porytiles2/xcut/di/components.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Task encapsulating the compilation operation for primary tilesets.
 *
 * @details
 * Breaks the monolithic compilation logic into discrete phases:
 * - process_porytiles_input() - metatileize, validate, decompose Porytiles layers
 * - process_porymap_input() - triple-layerize, decompile, decompose Porymap data
 * - setup_working_data() - initialize palettes, workspace, and output Porymap component
 * - match_tiles() - main loop matching Porytiles tiles to Porymap tiles/palettes
 * - assemble_output() - finalize output with dual-layer conversion, attributes, exports
 */
class CompilerTask {
  public:
    CompilerTask(
        const Tileset &tileset,
        const TextFormatter &format,
        const UserDiagnostics &diag,
        const TilePrinter &tile_printer,
        const PalettePrinter &pal_printer,
        const DomainConfig &config,
        const ArtifactEditMode tiles_edit_mode,
        const ArtifactEditMode pals_edit_mode)
        : tileset_{tileset}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer},
          config_{config}, tiles_edit_mode_{tiles_edit_mode}, pals_edit_mode_{pals_edit_mode},
          extrinsic_transparency_{}, num_pals_in_primary_{}, num_pals_total_{}, num_metatiles_in_primary_{},
          num_tiles_in_primary_{}, num_tiles_per_metatile_{}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> run();

  private:
    // Subtasks and helpers
    [[nodiscard]] ChainableResult<void> process_porytiles_input();
    [[nodiscard]] ChainableResult<void> process_porymap_input();
    [[nodiscard]] ChainableResult<void> setup_working_data();
    [[nodiscard]] ChainableResult<ColorIndexMap<Rgba32>>
    build_color_index_map(const std::vector<PaletteHint> &hints, std::size_t color_count_limit) const;
    [[nodiscard]] ChainableResult<void> match_tiles_pals_patch_or_locked();
    [[nodiscard]] ChainableResult<void> match_tiles_pals_optimized();
    void emit_no_matching_tile_error(std::size_t tile_index);
    void emit_no_matching_pal_error(std::size_t tile_index, const std::vector<PaletteMatchResult<Rgba32>> &matches);
    void emit_tile_limit_error(std::size_t tile_index, std::size_t tile_limit);
    [[nodiscard]] std::unique_ptr<Tileset> assemble_output();

    // Dependencies (injected in ctor)
    const Tileset &tileset_;
    const TextFormatter &format_;
    const UserDiagnostics &diag_;
    const TilePrinter &tile_printer_;
    const PalettePrinter &pal_printer_;
    const DomainConfig &config_;
    const ArtifactEditMode tiles_edit_mode_;
    const ArtifactEditMode pals_edit_mode_;

    // Config values (populated in run())
    ConfigValue<Rgba32> extrinsic_transparency_;
    ConfigValue<std::size_t> num_pals_in_primary_;
    ConfigValue<std::size_t> num_pals_total_;
    ConfigValue<std::size_t> num_metatiles_in_primary_;
    ConfigValue<std::size_t> num_tiles_in_primary_;
    ConfigValue<std::size_t> num_tiles_per_metatile_;
    ConfigValue<bool> pal_hints_enabled_;
    ConfigValue<std::vector<PaletteHint>> pal_hints_;

    // Intermediate state - Porytiles
    std::vector<Metatile<Rgba32>> porytiles_metatiles_{};
    std::vector<PixelTile<Rgba32>> porytiles_pixel_rgba_{};
    std::vector<CanonicalPixelTile<Rgba32>> porytiles_canonical_pixel_rgba_{};

    // Intermediate state - Porymap
    std::vector<TilemapEntry> porymap_tilemap_entries_{};
    std::vector<Metatile<Rgba32>> porymap_metatiles_{};
    std::vector<PixelTile<Rgba32>> porymap_pixel_rgba_{};
    std::vector<CanonicalPixelTile<Rgba32>> porymap_canonical_pixel_rgba_{};
    std::vector<Palette<Rgba32, pal::max_size>> porymap_pals_{};

    // Working data
    std::unique_ptr<PorymapTilesetComponent> new_porymap_component_{};
    std::unique_ptr<TilesPngWorkspace> tiles_workspace_{};
};

ChainableResult<std::unique_ptr<Tileset>> CompilerTask::run()
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_REF(config_, extrinsic_transparency, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_pals_in_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_pals_total, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_metatiles_in_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_in_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_per_metatile, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, pal_hints_enabled, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, pal_hints, tileset_.name(), std::unique_ptr<Tileset>);

    extrinsic_transparency_ = extrinsic_transparency;
    num_pals_in_primary_ = num_pals_in_primary;
    num_pals_total_ = num_pals_total;
    num_metatiles_in_primary_ = num_metatiles_in_primary;
    num_tiles_in_primary_ = num_tiles_in_primary;
    num_tiles_per_metatile_ = num_tiles_per_metatile;
    pal_hints_enabled_ = pal_hints_enabled;
    pal_hints_ = pal_hints;

    // Execute subtasks
    PT_TRY_CALL_PASS_ERR(process_porytiles_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(process_porymap_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(setup_working_data(), std::unique_ptr<Tileset>);

    if (tiles_edit_mode_ == ArtifactEditMode::optimize && pals_edit_mode_ == ArtifactEditMode::optimize) {
        PT_TRY_CALL_PASS_ERR(match_tiles_pals_optimized(), std::unique_ptr<Tileset>);
    }
    else if (tiles_edit_mode_ != ArtifactEditMode::optimize && pals_edit_mode_ != ArtifactEditMode::optimize) {
        PT_TRY_CALL_PASS_ERR(match_tiles_pals_patch_or_locked(), std::unique_ptr<Tileset>);
    }
    else {
        panic("TODO: impl this case");
    }

    return assemble_output();
}

ChainableResult<void> CompilerTask::process_porytiles_input()
{
    LayerImageMetatileizer<Rgba32> metatileizer{};
    MetatileValidator validator{&format_, &diag_, &tile_printer_, &pal_printer_, &config_, tileset_.name()};

    // Read Porytiles layer images into metatile vector
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatileizer.metatileize(
            tileset_.porytiles_component().bottom(),
            tileset_.porytiles_component().middle(),
            tileset_.porytiles_component().top()),
        "failed to metatileize input layer images for " + tileset_.name(),
        void);
    porytiles_metatiles_ = std::move(metatiles);

    // Run validation on Porytiles metatiles
    PT_TRY_CALL_CHAIN_ERR(
        validator.validate_primary(porytiles_metatiles_),
        "encountered error(s) while validating Porytiles metatiles",
        void);

    // Decompose Porytiles metatiles and generate canonical versions
    porytiles_pixel_rgba_ = metatile::decompose(porytiles_metatiles_);
    porytiles_canonical_pixel_rgba_ = transform<CanonicalPixelTile<Rgba32>>(porytiles_pixel_rgba_);

    return {};
}

ChainableResult<void> CompilerTask::process_porymap_input()
{
    LayerModeConverter layer_mode_converter{&format_, &diag_, &tile_printer_, extrinsic_transparency_};
    MetatileDecompiler metatile_decompiler{&format_, &diag_, &tile_printer_, extrinsic_transparency_};

    // Decompile Porymap tilemap entries and decompose into tile vector
    PT_TRY_ASSIGN_CHAIN_ERR(
        tilemap_entries,
        layer_mode_converter.triple_layerize(tileset_.porymap_component()),
        "failed to triple-layerize Porymap component for tileset " + tileset_.name(),
        void);
    porymap_tilemap_entries_ = std::move(tilemap_entries);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            porymap_tilemap_entries_, tileset_.porymap_component().tiles_png(), tileset_.porymap_component().pals()),
        "failed to decompile Porymap component for tileset " + tileset_.name(),
        void);
    porymap_metatiles_ = std::move(metatiles);

    /*
     * We don't need to run any validation (including size validation) on porymap_metatiles here. We're going to
     * overwrite them anyway. We only need to check the size of the final tilemap entry vector. Patch builds don't need
     * to preserve tilemap entries since those cannot be referenced by other tilesets. We can just write a new entry
     * vector every time.
     */

    // Decompose Porymap metatiles and generate canonical versions
    porymap_pixel_rgba_ = metatile::decompose(porymap_metatiles_);
    porymap_canonical_pixel_rgba_ = transform<CanonicalPixelTile<Rgba32>>(porymap_pixel_rgba_);

    return {};
}

ChainableResult<void> CompilerTask::setup_working_data()
{
    std::vector<PaletteHint> hints = pal_hints_enabled_.value() ? pal_hints_.value() : std::vector<PaletteHint>{};
    /*
     * TODO: PaletteValidator: throw error if pal isn't size 16
     */
    /*
     * TODO: PaletteValidator: throw error if any non-slot-0 pal slot contains the extrinsic transparency color
     */
    /*
     * TODO: PaletteValidator: throw warning if slot 0 doesn't match current extrinsic_transparency. This is not a
     * hard failure condition, since some advanced users may be using slot 0 for a .pla blend color. But we should
     * at least generate a warning in case folks are confused about what slot 0 is for.
     */
    /*
     * TODO: PaletteValidator: disallow duplicate colors or extrinsic transparency in palette hints
     */
    /*
     * TODO: PaletteValidator: disallow extrinsic transparency in Porytiles palette overrides
     */

    if (pals_edit_mode_ == ArtifactEditMode::locked) {
        // Collect primary palettes from existing Porymap component
        porymap_pals_.reserve(num_pals_in_primary_);
        for (unsigned int i = 0; i < num_pals_in_primary_; i++) {
            porymap_pals_.push_back(tileset_.porymap_component().pals()[i]);
        }
    }
    else if (pals_edit_mode_ == ArtifactEditMode::patch) {
        panic("TODO: implement handling for pals ArtifactEditMode::patch");
    }
    else if (pals_edit_mode_ == ArtifactEditMode::optimize) {
        /*
         * Create ColorIndexMap from the Porytiles tiles, Porytiles pals, and palette hints. This validates that we
         * don't exceed the global color count limit.
         */
        const std::size_t color_count_limit = num_pals_in_primary_.value() * (pal::max_size - 1);
        PT_TRY_ASSIGN_CHAIN_ERR(
            color_index_map,
            build_color_index_map(hints, color_count_limit),
            "failed to build color index map for tileset " + tileset_.name(),
            void);

        /*
         * TODO: we should have warnings get generated here if any colors in the hints/overrides did not appear in the
         * layer PNGs.
         */

        /*
         * TODO: create canonical ShapeTile vectors here once we implement 'compile.tiles.sharing:' config option
         */
        // std::vector<CanonicalShapeTile<ColorIndex>> porytiles_canonical_color_index_shapes =
        //     transform(porytiles_pixel_rgba, [&color_index_map, &extrinsic_transparency](const PixelTile<Rgba32>
        //     &tile) {
        //         return CanonicalShapeTile{from_pixel_tile(tile, color_index_map, extrinsic_transparency.value())};
        //     });
        // std::vector<CanonicalShapeTile<Rgba32>> porytiles_canonical_rgba_shapes = transform(
        //     porytiles_canonical_color_index_shapes, [&color_index_map](const CanonicalShapeTile<ColorIndex> &tile) {
        //         return CanonicalShapeTile{shape_tile_to_pixel_colors(tile, color_index_map)};
        //     });

        BestFusionStrategy packing_strategy{&format_, &diag_};
        PalettePacker pal_packer{&packing_strategy, &format_, &diag_};
        std::bitset<pal::num_pals> available_pals{0};
        for (unsigned int i = 0; i < num_pals_in_primary_; i++) {
            // TODO: support out-of-band primary palettes
            available_pals.set(i, true);
        }
        PackingParams packing_params{};
        packing_params.tiles_ = porytiles_pixel_rgba_;
        packing_params.color_map_ = color_index_map;
        packing_params.extrinsic_transparency_ = extrinsic_transparency_.value();
        packing_params.prefilled_pals_ = tileset_.porytiles_component().pals();
        packing_params.hints_ = hints;
        packing_params.available_pals_ = available_pals;

        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_packing,
            pal_packer.pack_tiles(packing_params),
            "failed to pack palettes for tileset " + tileset_.name(),
            void);

        for (unsigned int i = 0; i < pal::num_pals; i++) {
            const auto &maybe_packed_pal = pal_packing.pals_.at(i);
            if (maybe_packed_pal.has_value()) {
                // Copy over the packed palette
                porymap_pals_.push_back(maybe_packed_pal.value());
            }
            else if (tileset_.porytiles_component().pal_at(i).has_value()) {
                /*
                 * TODO: out-of-band override: resolve all wildcards to some default and copy it over
                 */
                panic("TODO: implement copy for out-of-band porytiles override pal");
            }
            else {
                /*
                 * Copy remaining secondary palettes from the original component. The "secondary" pals in a primary
                 * tileset's folder won't be actually loaded by the game engine. Porymap also doesn't show them -- it
                 * will grab pals from the relevant secondary set folder. However, we copy them here for consistency. If
                 * for some reason the user had edited them, we don't want to clobber their edits. Porytiles should be
                 * surgical where possible.
                 *
                 * Copy junk pals. 13.pal, 14.pal, 15.pal exist in the tileset but are reserved by the game engine for
                 * overworld/shop UI. Here we just copy them over as-is. Again, if for some reason the user had edited
                 * them, let's not clobber anything unnecessarily.
                 */
                porymap_pals_.push_back(tileset_.porymap_component().pal_at(i));
            }
        }
    }
    else {
        panic("unexpected pals ArtifactEditMode");
    }

    // Create new Porymap component for output
    // TODO: The resulting PorymapTilesetComponent may be incomplete. E.g., the user may have specified PLA
    // files; they will be present on disk. We don't want to clobber them when saving the newly compiled
    // component. So we'll need to pull them from the original component and inject them into this one before
    // returning. We should probably add PLA file handling to the Tileset repository aggregate root. That way. all
    // this is handled automatically via the save/load abstraction mechanisms. PLA files are a first-class domain
    // concept, so they should be handled like any other file type (e.g. pal files, override files, etc). If we do that,
    // then here, instead of making a new PorymapComponent, we could invoke the copy ctor. And then we should add
    // explicit "reset" functions for the tilemap entries, tiles.png, pals, etc to clear the old values?
    new_porymap_component_ = std::make_unique<PorymapTilesetComponent>();

    // Create tiles workspace
    tiles_workspace_ = [](ArtifactEditMode tiles_edit_mode, const Tileset &tileset, std::size_t num_tiles_in_primary) {
        if (tiles_edit_mode == ArtifactEditMode::locked) {
            /*
             * TODO: in tiles locked mode, since we're not adding tiles, the capacity should just be the size of
             * the original tiles.png. Later on, when we export, we'll set "include_trailing_transparent" so if user
             * originally had transparency at the end (for whatever reason), we won't clobber it.
             */
            return std::make_unique<TilesPngWorkspace>(tileset.porymap_component().tiles_png(), num_tiles_in_primary);
        }
        if (tiles_edit_mode == ArtifactEditMode::patch) {
            return std::make_unique<TilesPngWorkspace>(tileset.porymap_component().tiles_png(), num_tiles_in_primary);
        }
        if (tiles_edit_mode == ArtifactEditMode::optimize) {
            return std::make_unique<TilesPngWorkspace>(num_tiles_in_primary);
        }
        panic("unexpected tiles_edit_mode");
    }(tiles_edit_mode_, tileset_, num_tiles_in_primary_.value());

    return {};
}

ChainableResult<ColorIndexMap<Rgba32>>
CompilerTask::build_color_index_map(const std::vector<PaletteHint> &hints, std::size_t color_count_limit) const
{
    constexpr auto tag = "global-color-count-violation";

    // Create ColorIndexMap from the Porytiles tiles
    ColorIndexMap color_index_map{porytiles_pixel_rgba_, extrinsic_transparency_.value()};

    // Check color count after initial tile colors are added
    if (color_index_map.size() > color_count_limit) {
        panic("color_index_map.size() > count_limit - this should have already been validated by MetatileValidator");
    }

    // Add override palettes and validate after each
    for (std::size_t pal_index = 0; pal_index < tileset_.porytiles_component().pals().size(); ++pal_index) {
        const auto &maybe_override_pal = tileset_.porytiles_component().pals().at(pal_index);
        if (!maybe_override_pal.has_value()) {
            continue;
        }
        color_index_map.add_pal(maybe_override_pal.value(), extrinsic_transparency_.value());
        if (color_index_map.size() > color_count_limit) {
            diag_.err(
                tag,
                format_.format(
                    "found '{}' global unique colors after adding override palette '{}', limit is '{}'",
                    FormatParam{color_index_map.size(), Style::bold},
                    FormatParam{pad_two_digits(pal_index) + ".pal", Style::bold},
                    FormatParam{color_count_limit, Style::bold}));
            diag_.note(tag, global_color_limit_definition(format_, color_count_limit, num_pals_in_primary_));

            return FormattableError{
                "{}: found '{}' unique colors after adding override palette '{}', limit is '{}'",
                FormatParam{tag, Style::bold},
                FormatParam{color_index_map.size(), Style::bold},
                FormatParam{pad_two_digits(pal_index) + ".pal", Style::bold},
                FormatParam{color_count_limit, Style::bold}};
        }
    }

    // Add palette hints and validate after each
    for (const auto &hint : hints) {
        color_index_map.add_pal(hint.pal(), extrinsic_transparency_.value());
        if (color_index_map.size() > color_count_limit) {
            diag_.err(
                tag,
                format_.format(
                    "found '{}' global unique colors after adding palette hint '{}', limit is '{}'",
                    FormatParam{color_index_map.size(), Style::bold},
                    FormatParam{hint.name(), Style::bold},
                    FormatParam{color_count_limit, Style::bold}));
            diag_.note(tag, global_color_limit_definition(format_, color_count_limit, num_pals_in_primary_));

            return FormattableError{
                "{}: found '{}' unique colors after adding palette hint '{}', limit is '{}'",
                FormatParam{tag, Style::bold},
                FormatParam{color_index_map.size(), Style::bold},
                FormatParam{hint.name(), Style::bold},
                FormatParam{color_count_limit, Style::bold}};
        }
    }

    return color_index_map;
}

ChainableResult<void> CompilerTask::match_tiles_pals_patch_or_locked()
{
    // Preconditions
    assert_or_panic(
        tiles_edit_mode_ != ArtifactEditMode::optimize, "tiles.png edit mode cannot be 'optimize' in this method");
    assert_or_panic(
        pals_edit_mode_ != ArtifactEditMode::optimize, "pals edit mode cannot be 'optimize' in this method");
    /*
     * TODO: this is wrong. E.g. user could be asking for a tiles:locked build but have added new metatiles. We need to
     * account for adding or removing metatiles, since that is still allowed during a tiles/pals locked build.
     */
    assert_or_panic(
        porytiles_pixel_rgba_.size() == porymap_pixel_rgba_.size(),
        "porytiles_pixel_rgba_.size() != porymap_pixel_rgba_.size()");
    assert_or_panic(
        porytiles_pixel_rgba_.size() == porytiles_canonical_pixel_rgba_.size(),
        "porytiles_pixel_rgba_.size() != porytiles_canonical_pixel_rgba_.size()");
    assert_or_panic(
        porymap_pixel_rgba_.size() == porymap_canonical_pixel_rgba_.size(),
        "porymap_pixel_rgba_.size() != porymap_canonical_pixel_rgba_.size()");
    assert_or_panic(
        porymap_tilemap_entries_.size() == porymap_pixel_rgba_.size(),
        "porymap_tilemap_entries_.size() != porymap_pixel_rgba_.size()");

    bool matched_all_tiles = true;
    for (std::size_t i = 0; i < porytiles_pixel_rgba_.size(); i++) {
        const auto &porytiles_tile = porytiles_pixel_rgba_[i];
        const auto &porymap_tile = porymap_pixel_rgba_[i];
        const auto &canonical_porytiles_tile = porytiles_canonical_pixel_rgba_[i];
        const auto &canonical_porymap_tile = porymap_canonical_pixel_rgba_[i];
        const auto &porymap_tilemap_entry = porymap_tilemap_entries_[i];

        // CASE 1: Porytiles component tile exactly matches Porymap component
        if (porytiles_tile.equals_ignoring_transparency(porymap_tile, extrinsic_transparency_)) {
            new_porymap_component_->push_back_tilemap_entry(porymap_tilemap_entry);
        }

        // CASE 2: Porytiles component tile matches Porymap component under flip transformation
        else if (canonical_porytiles_tile.equals_ignoring_transparency(
                     canonical_porymap_tile, extrinsic_transparency_)) {
            // XOR flip bits to compute transformation from Porytiles orientation to Porymap orientation
            const bool pt_to_pm_hflip = canonical_porytiles_tile.h_flip() ^ canonical_porymap_tile.h_flip();
            const bool pt_to_pm_vflip = canonical_porytiles_tile.v_flip() ^ canonical_porymap_tile.v_flip();
            TilemapEntry new_entry{
                porymap_tilemap_entry.tile_index(),
                porymap_tilemap_entry.pal_index(),
                static_cast<bool>(porymap_tilemap_entry.h_flip() ^ pt_to_pm_hflip),
                static_cast<bool>(porymap_tilemap_entry.v_flip() ^ pt_to_pm_vflip)};
            new_porymap_component_->push_back_tilemap_entry(new_entry);
        }

        // CASE 3: New tile, compute which pal to use, compute (or create) tile to use
        else {
            // TODO: top_n matches should be configurable
            // TODO: what if multiple pals match?
            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(porytiles_tile, porymap_pals_, extrinsic_transparency_.value(), 1);

            // CASE 3a: found covering pal
            if (matches.at(0).is_covered) {
                const auto pal_index = matches.at(0).pal_index;
                const auto &matched_pal = porymap_pals_.at(pal_index);
                const auto index_tile =
                    index_tile_from_color_tile(porytiles_tile, matched_pal, extrinsic_transparency_.value());
                CanonicalPixelTile canonical_index_tile{index_tile};
                const auto maybe_tile_index = tiles_workspace_->first_occurrence_of(canonical_index_tile);

                // CASE 3a-i: tile already present
                if (maybe_tile_index.has_value()) {
                    const auto tile_index = maybe_tile_index.value();
                    const auto workspace_tile = tiles_workspace_->tile_at(tile_index);
                    const bool pt_to_pm_hflip = canonical_index_tile.h_flip() ^ workspace_tile.h_flip();
                    const bool pt_to_pm_vflip = canonical_index_tile.v_flip() ^ workspace_tile.v_flip();
                    const TilemapEntry new_entry{tile_index, pal_index, pt_to_pm_hflip, pt_to_pm_vflip};
                    new_porymap_component_->push_back_tilemap_entry(new_entry);
                }

                // CASE 3a-ii: tile not found and tiles.png is locked
                else if (!maybe_tile_index.has_value() && tiles_edit_mode_ == ArtifactEditMode::locked) {
                    matched_all_tiles = false;
                    emit_no_matching_tile_error(i);
                }

                // CASE 3a-iii: tile not found and tiles.png is not locked (i.e. it's patch)
                else {
                    if (tiles_workspace_->at_capacity()) {
                        matched_all_tiles = false;
                        emit_tile_limit_error(i, tiles_workspace_->capacity());
                        break;
                    }
                    const std::size_t inserted_index = tiles_workspace_->insert_tile(canonical_index_tile);
                    const auto workspace_tile = tiles_workspace_->tile_at(inserted_index);
                    const TilemapEntry new_entry{
                        inserted_index,
                        pal_index,
                        static_cast<bool>(canonical_index_tile.h_flip() ^ workspace_tile.h_flip()),
                        static_cast<bool>(canonical_index_tile.v_flip() ^ workspace_tile.v_flip())};
                    new_porymap_component_->push_back_tilemap_entry(new_entry);
                }
            }

            // CASE 3b: no covering pal
            else {
                matched_all_tiles = false;
                emit_no_matching_pal_error(i, matches);
            }
        }
    }

    if (!matched_all_tiles) {
        return ChainableResult<void>{FormattableError{"failed to match all Porytiles tiles"}};
    }

    return {};
}

ChainableResult<void> CompilerTask::match_tiles_pals_optimized()
{
    // Preconditions
    assert_or_panic(
        tiles_edit_mode_ == ArtifactEditMode::optimize, "tiles.png edit mode must be 'optimize' in this method");
    assert_or_panic(pals_edit_mode_ == ArtifactEditMode::optimize, "pals edit mode must be 'optimize' in this method");

    bool matched_all_tiles = true;
    for (std::size_t i = 0; i < porytiles_pixel_rgba_.size(); i++) {
        const auto &porytiles_tile = porytiles_pixel_rgba_[i];

        // TODO: what if multiple pals match?
        std::vector<PaletteMatchResult<Rgba32>> matches =
            match_or_best(porytiles_tile, porymap_pals_, extrinsic_transparency_.value(), 1);

        // CASE 1: found covering pal
        if (matches.at(0).is_covered) {
            const auto pal_index = matches.at(0).pal_index;
            const auto &matched_pal = porymap_pals_.at(pal_index);
            const auto index_tile =
                index_tile_from_color_tile(porytiles_tile, matched_pal, extrinsic_transparency_.value());
            CanonicalPixelTile canonical_index_tile{index_tile};
            const auto maybe_tile_index = tiles_workspace_->first_occurrence_of(canonical_index_tile);

            // CASE 1a: tile already present
            if (maybe_tile_index.has_value()) {
                const auto tile_index = maybe_tile_index.value();
                const auto workspace_tile = tiles_workspace_->tile_at(tile_index);
                const bool pt_to_pm_hflip = canonical_index_tile.h_flip() ^ workspace_tile.h_flip();
                const bool pt_to_pm_vflip = canonical_index_tile.v_flip() ^ workspace_tile.v_flip();
                const TilemapEntry new_entry{tile_index, pal_index, pt_to_pm_hflip, pt_to_pm_vflip};
                new_porymap_component_->push_back_tilemap_entry(new_entry);
            }
            // CASE 1b: tile not found
            else {
                if (tiles_workspace_->at_capacity()) {
                    matched_all_tiles = false;
                    emit_tile_limit_error(i, tiles_workspace_->capacity());
                    break;
                }
                const std::size_t inserted_index = tiles_workspace_->insert_tile(canonical_index_tile);
                const auto workspace_tile = tiles_workspace_->tile_at(inserted_index);
                const TilemapEntry new_entry{
                    inserted_index,
                    pal_index,
                    static_cast<bool>(canonical_index_tile.h_flip() ^ workspace_tile.h_flip()),
                    static_cast<bool>(canonical_index_tile.v_flip() ^ workspace_tile.v_flip())};
                new_porymap_component_->push_back_tilemap_entry(new_entry);
            }
        }

        // CASE 2: no covering pal
        else {
            panic("this should have failed earlier after packing step");
        }
    }

    if (!matched_all_tiles) {
        return ChainableResult<void>{FormattableError{"failed to match all Porytiles tiles"}};
    }

    return {};
}

void CompilerTask::emit_no_matching_tile_error(std::size_t tile_index)
{
    constexpr auto tag = "no-matching-tiles";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> no_match_err{};
    no_match_err.emplace_back(format_.format(
        "{}: no matching tiles found",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
    std::ranges::copy(
        tile_printer_.print_metatile_tile_highlight(
            porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_),
        std::back_inserter(no_match_err));
    diag_.err(tag, no_match_err);

    // TODO: add note to print out matching pal and print index_pixel generated
}

void CompilerTask::emit_no_matching_pal_error(
    std::size_t tile_index, const std::vector<PaletteMatchResult<Rgba32>> &matches)
{
    constexpr auto tag = "no-matching-palettes";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> no_match_err{};
    no_match_err.emplace_back(format_.format(
        "{}: no matching palettes found",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
    std::ranges::copy(
        tile_printer_.print_metatile_tile_highlight(
            porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_),
        std::back_inserter(no_match_err));
    diag_.err(tag, no_match_err);

    // Emit a long note showing the top N closest matches
    std::vector<std::string> closest_n_note{};
    // TODO: substitute configurable top_n for N
    closest_n_note.emplace_back("closest N match(es):");
    int match_index = 0;
    for (const auto &match : matches) {
        if (match_index != 0) {
            // Add a blank line between subsequent matches
            closest_n_note.emplace_back();
        }
        closest_n_note.push_back(format_.format(
            "Palette match candidate: {}",
            FormatParam{pad_two_digits(match.pal_index) + std::string{".pal"}, Style::bold}));
        std::ranges::copy(
            pal_printer_.print_rgba_palette_covered_missing(
                porymap_pals_.at(match.pal_index), match.covered_colors, match.missing_colors),
            std::back_inserter(closest_n_note));
        closest_n_note.emplace_back();
        closest_n_note.push_back(format_.format(
            "Uncovered pixels with {}:",
            FormatParam{pad_two_digits(match.pal_index) + std::string{".pal"}, Style::bold}));
        std::ranges::copy(
            tile_printer_.print_metatile_pixel_highlights(
                porytiles_metatiles_.at(metatile_index),
                layer,
                subtile,
                match.uncovered_pixel_indices,
                extrinsic_transparency_),
            std::back_inserter(closest_n_note));
        match_index++;
    }
    diag_.note(tag, closest_n_note);
}

void CompilerTask::emit_tile_limit_error(std::size_t tile_index, std::size_t tile_limit)
{
    constexpr auto tag = "tile-limit";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> tile_limit_error{};
    tile_limit_error.emplace_back(format_.format(
        "{}: hit limit of '{}' unique tiles",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold},
        FormatParam{tile_limit, Style::bold}));
    std::ranges::copy(
        tile_printer_.print_metatile_tile_highlight(
            porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_),
        std::back_inserter(tile_limit_error));
    diag_.err(tag, tile_limit_error);

    // Construct note text
    std::vector<std::string> note_text;
    note_text.push_back(
        format_.format("tile limit is '{}' due to configuration", FormatParam{num_tiles_in_primary_, Style::bold}));
    note_text.emplace_back();
    std::ranges::copy(num_tiles_in_primary_.prettify(format_), std::back_inserter(note_text));
    diag_.note(tag, note_text);
}

std::unique_ptr<Tileset> CompilerTask::assemble_output()
{
    /*
     * TODO: we should track tile+pal use and warn the user here about any unused tiles or pal colors. This would be
     * nice for cases where users add some assets and compile with "tiles/pals:patch", but then later decide to remove
     * the assets. We could warn them these assets are unused so that they can optionally remove to free up space. We
     * could also have a compilation option "force_remove" that forcibly removes unused stuff. This is obviously less
     * safe, since for vanilla primary tilesets, seemingly unused assets may be in use from the secondaries. We can
     * solve this by eventually having code that reads all tileset pairings from layouts.json and computes which primary
     * assets are truly unused.
     */

    // No changes here, this is a compilation operation - no writebacks into input assets
    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset_.porytiles_component());

    /*
     * If user is requesting dual-layer, use the input Porytiles-format metatiles to infer the LayerType for each
     * metatile and remove the relevant tilemap entries. Here, we assume that the Porytiles metatiles have already been
     * validated in an earlier step as dual-layer compatible.
     */
    LayerModeConverter layer_mode_converter{&format_, &diag_, &tile_printer_, extrinsic_transparency_};
    const auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile_);
    if (configured_layer_mode == LayerMode::dual) {
        const auto &dual_layerized =
            layer_mode_converter.dual_layerize(new_porymap_component_->metatiles_bin(), porytiles_metatiles_);
        new_porymap_component_->metatiles_bin(dual_layerized);
    }

    // Copy metatile attributes from original
    // TODO: need to copy over metatile behavior (and other firered attrs if relevant)
    for (const auto &metatile : porytiles_metatiles_) {
        LayerType layer_type;
        if (configured_layer_mode == LayerMode::dual) {
            layer_type = metatile.infer_layer_type(extrinsic_transparency_.value());
        }
        else {
            layer_type = LayerType::normal;
        }
        MetatileAttribute new_attr{layer_type, 0};
        new_porymap_component_->push_back_attribute(new_attr);
    }

    // Export tiles in original form
    if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
        /*
         * TODO: why is using ExportFlipMode::canonical here bugged? I think it has to do with how we computed the flip
         * bits in 'match_tiles_pals_optimized'. If we're going to make this configurable, we'll need to check the
         * config value in the matcher function so we can compute the flip bits correctly.
         */
        new_porymap_component_->tiles_png(
            tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::trim_trailing_transparent));
    }
    else {
        new_porymap_component_->tiles_png(
            tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent));
    }

    // Copy palettes from our processed porymap_pals vector
    for (unsigned int i = 0; i < pal::num_pals; i++) {
        new_porymap_component_->set_pal(i, porymap_pals_[i]);
    }

    // Create the full Tileset and return
    return std::make_unique<Tileset>(
        tileset_.name(), std::move(new_porytiles_component), std::move(new_porymap_component_));
}

} // namespace

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile_todo_remove(const Tileset &tileset) const
{
    // Grab configuration values we'll need
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_metatiles_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_per_metatile, tileset.name(), std::unique_ptr<Tileset>);

    // Initialize all the compilation services
    LayerImageMetatileizer<Rgba32> metatileizer{};
    MetatileValidator validator{format_, diag_, tile_printer_, pal_printer_, config_, tileset.name()};
    LayerModeConverter layer_converter{format_, diag_, tile_printer_, extrinsic_transparency};
    // ClassicDfsStrategy packing_strategy{};
    // PalettePacker pal_packer{&packing_strategy, format_, diag_};

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

    // Decompose Porytiles metatiles and generate canonical versions
    const auto porytiles_pixel_rgba = metatile::decompose(metatiles);
    const auto porytiles_canonical_pixel_rgba = transform<CanonicalPixelTile<Rgba32>>(porytiles_pixel_rgba);

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
    ColorIndexMap<Rgba32> color_index_map{porytiles_pixel_rgba, extrinsic_transparency};

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
    Palette<Rgba32, pal::max_size> pal{rgba_red};
    pal.set(0, extrinsic_transparency.value());

    new_porymap_component->tiles_png(tiles_png);
    for (unsigned int i = 0; i < pal::num_pals; i++) {
        new_porymap_component->set_pal(i, pal);
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

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile(const Tileset &tileset) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tiles_edit_mode, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, pals_edit_mode, tileset.name(), std::unique_ptr<Tileset>);
    CompilerTask task{
        tileset,
        *format_,
        *diag_,
        *tile_printer_,
        *pal_printer_,
        *config_,
        tiles_edit_mode.value(),
        pals_edit_mode.value()};
    return task.run();
}

} // namespace porytiles2
