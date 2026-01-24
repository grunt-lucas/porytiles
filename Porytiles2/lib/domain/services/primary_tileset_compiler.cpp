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
#include "porytiles2/domain/algorithms/tileset_compile_validators.hpp"
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
#include "porytiles2/domain/packing/services/overload_and_remove_strategy.hpp"
#include "porytiles2/domain/packing/services/palette_packer.hpp"
#include "porytiles2/domain/services/anim_tile_matcher.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/metatile_decompiler.hpp"
#include "porytiles2/utilities/functional/transform.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Result type for tile assignment operations during compilation.
 *
 * @details
 * Encapsulates the outcome of attempting to assign a tile via palette matching. Contains all information needed for
 * error reporting in failure cases.
 */
struct TileAssignmentResult {
    enum class Status { success, no_covering_pal, tile_not_found, tile_limit_reached };

    Status status{Status::success};
    std::optional<TilemapEntry> entry{};

    // Error reporting data (populated on failure)
    std::vector<PaletteMatchResult<Rgba32>> match_results{};
    PixelTile<IndexPixel> index_tile{};
    std::size_t pal_index{0};
    Palette<Rgba32, pal::max_size> matched_pal{};
};

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
          num_tiles_in_primary_{}, num_tiles_per_metatile_{}, pal_hints_enabled_{}, pal_hints_{}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> run();

  private:
    // Pipeline steps
    [[nodiscard]] ChainableResult<void> pipeline_step_process_porytiles_input();
    [[nodiscard]] ChainableResult<void> pipeline_step_process_porymap_input();
    [[nodiscard]] ChainableResult<void> pipeline_step_validate_input();
    [[nodiscard]] ChainableResult<void> pipeline_step_setup_working_data();
    [[nodiscard]] ChainableResult<void> pipeline_step_match_tiles_pals();
    [[nodiscard]] std::unique_ptr<Tileset> pipeline_step_assemble_output();

    // Pipeline helpers - tile matching
    [[nodiscard]] std::optional<TilemapEntry> pipeline_helper_try_reuse_porymap_tile(std::size_t tile_index);
    [[nodiscard]] TileAssignmentResult
    pipeline_helper_assign_tile_via_pal_match(const PixelTile<Rgba32> &porytiles_tile);

    // Pipeline helpers - palette packing
    [[nodiscard]] ChainableResult<void> pipeline_helper_run_pal_packing();
    [[nodiscard]] ChainableResult<ColorIndexMap<Rgba32>>
    pipeline_helper_build_color_index_map(const std::vector<PaletteHint> &hints, std::size_t color_count_limit) const;

    // Pipeline helpers - animation processing
    [[nodiscard]] ChainableResult<void> pipeline_helper_register_animations();
    [[nodiscard]] ChainableResult<void> pipeline_helper_register_animations_optimize();
    [[nodiscard]] ChainableResult<void> pipeline_helper_register_animations_patch();
    [[nodiscard]] ChainableResult<void> pipeline_helper_register_animations_locked();
    void pipeline_helper_compile_animations();

    // Pipeline helpers - error emission
    void pipeline_helper_emit_no_matching_tile_error(
        std::size_t tile_index,
        const PixelTile<IndexPixel> &index_tile,
        std::size_t pal_index,
        const Palette<Rgba32, pal::max_size> &matched_pal);
    void pipeline_helper_emit_no_matching_pal_error(
        std::size_t tile_index, const std::vector<PaletteMatchResult<Rgba32>> &matches);
    void pipeline_helper_emit_tile_limit_error(std::size_t tile_index, std::size_t tile_limit);

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
    std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> new_porymap_pals_{};

    // Working data
    std::unique_ptr<PorymapTilesetComponent> new_porymap_component_{};
    std::unique_ptr<TilesPngWorkspace> tiles_workspace_{};
    AnimTileMatcher anim_tile_matcher_{};
};

ChainableResult<std::unique_ptr<Tileset>> CompilerTask::run()
{
    /*
     * TODO: we have a bug. If pals_edit_mode::optimize and tiles_edit_mode::locked, on the first compile pass after
     * making no changes, it will optimize the pals, but emit identical metatile entries (since the Porytiles and
     * Porymap metatiles will match). Then it will emit the optimized pals but identical tilemap entries. The tileset
     * becomes corrupted and subsequent compilations crash out with a bazillion errors.
     *
     * Need to think about the right way to fix. Is there a scenario where setting pals::optimize tiles::locked even
     * makes sense? Instead of adding more tortured logic to the compiler, we could just ban this combo.
     *
     * Thinking about it more, I think we need to ban this combo. Tiles as an artifact are fundamentally dependent on
     * the palettes. If palettes change, tiles have to change. So it doesn't make sense to allow a setting where
     * palettes are being changed but then the tiles aren't.
     */

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
    PT_TRY_CALL_PASS_ERR(pipeline_step_process_porytiles_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_process_porymap_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_validate_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_setup_working_data(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_match_tiles_pals(), std::unique_ptr<Tileset>);

    return pipeline_step_assemble_output();
}

ChainableResult<void> CompilerTask::pipeline_step_process_porytiles_input()
{
    LayerImageMetatileizer<Rgba32> metatileizer{};

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

    // Decompose Porytiles metatiles and generate canonical versions
    porytiles_pixel_rgba_ = metatile::decompose(porytiles_metatiles_);
    porytiles_canonical_pixel_rgba_ = transform<CanonicalPixelTile<Rgba32>>(porytiles_pixel_rgba_);

    return {};
}

ChainableResult<void> CompilerTask::pipeline_step_process_porymap_input()
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

ChainableResult<void> CompilerTask::pipeline_step_validate_input()
{
    TilesetCompileValidatorServices services{config_, format_, diag_, tile_printer_, pal_printer_};

    /*
     * TODO: do we want to collate some of these before returning? It would present more errors to user at once. There
     * are pros and cons to this.
     */

    // Run metatile count validation
    PT_TRY_CALL_PASS_ERR(validate_metatile_count(services, tileset_.name(), false, porytiles_metatiles_), void);

    if (pals_edit_mode_ != ArtifactEditMode::optimize) {
        // Validate Porymap pals if user is asking for pals:locked or pals:patch
        for (std::size_t pal_index = 0; pal_index < tileset_.porymap_component().pals().size(); ++pal_index) {
            PT_TRY_CALL_PASS_ERR(
                validate_porymap_pal(
                    services, tileset_.name(), tileset_.porymap_component().pals().at(pal_index), pal_index),
                void);
        }
    }

    // Validate Porytiles pals
    // TODO: this loop should respect num_pals_in_primary setting
    for (std::size_t pal_index = 0; pal_index < tileset_.porytiles_component().pals().size(); ++pal_index) {
        if (tileset_.porytiles_component().pals().at(pal_index).has_value()) {
            PT_TRY_CALL_PASS_ERR(
                validate_porytiles_pal(
                    services, tileset_.name(), tileset_.porytiles_component().pals().at(pal_index).value(), pal_index),
                void);
        }
    }

    // Validate palette hints
    for (const auto &hint : pal_hints_.value()) {
        PT_TRY_CALL_PASS_ERR(validate_pal_hint(services, tileset_.name(), hint), void);
    }

    // Run alpha channel validation
    PT_TRY_CALL_PASS_ERR(
        validate_alpha_channels(
            services, tileset_.name(), porytiles_metatiles_, tileset_.porytiles_component().anims()),
        void);

    // Run layer mode validation
    PT_TRY_CALL_PASS_ERR(validate_layer_mode(services, tileset_.name(), porytiles_metatiles_), void);

    // Run tile color count validation
    PT_TRY_CALL_PASS_ERR(
        validate_tile_color_count(
            services, tileset_.name(), porytiles_metatiles_, tileset_.porytiles_component().anims()),
        void);

    // Run global color count validation
    PT_TRY_CALL_PASS_ERR(
        validate_global_color_count(
            services,
            tileset_.name(),
            false,
            porytiles_metatiles_,
            tileset_.porytiles_component().anims(),
            tileset_.porytiles_component().pals(),
            pal_hints_.value()),
        void);

    // Run precision loss validation
    PT_TRY_CALL_PASS_ERR(
        validate_precision_loss(
            services,
            tileset_.name(),
            porytiles_metatiles_,
            tileset_.porytiles_component().anims(),
            tileset_.porytiles_component().pals(),
            pal_hints_.value(),
            std::nullopt),
        void);

    // Run animation validation
    PT_TRY_CALL_PASS_ERR(validate_anim_frames(services, tileset_.name(), tileset_.porytiles_component().anims()), void);

    return {};
}

ChainableResult<void> CompilerTask::pipeline_step_setup_working_data()
{
    // Create palettes
    if (pals_edit_mode_ == ArtifactEditMode::locked) {
        // Collect all palettes from existing Porymap component
        for (std::size_t i = 0; i < pal::num_pals; i++) {
            new_porymap_pals_[i] = tileset_.porymap_component().pals()[i];
        }
    }
    else if (pals_edit_mode_ == ArtifactEditMode::patch) {
        panic("TODO: implement handling for pals ArtifactEditMode::patch");
    }
    else if (pals_edit_mode_ == ArtifactEditMode::optimize) {
        PT_TRY_CALL_PASS_ERR(pipeline_helper_run_pal_packing(), void);
    }
    else {
        panic("unexpected pals ArtifactEditMode");
    }

    // Create tiles workspace
    tiles_workspace_ = [](ArtifactEditMode tiles_edit_mode, const Tileset &tileset, std::size_t num_tiles_in_primary) {
        if (tiles_edit_mode == ArtifactEditMode::locked) {
            /*
             * When tiles are locked, compute the exact size of tiles.png so we keep it completely unchanged. When we
             * output, we'll also set ExportTrimMode::include_trailing_transparent so that if there was transparency at
             * the end, we don't remove it.
             */
            const auto size_in_tiles = tileset.porymap_component().tiles_png().size_in_tiles();
            return std::make_unique<TilesPngWorkspace>(tileset.porymap_component().tiles_png(), size_in_tiles);
        }
        if (tiles_edit_mode == ArtifactEditMode::patch) {
            return std::make_unique<TilesPngWorkspace>(tileset.porymap_component().tiles_png(), num_tiles_in_primary);
        }
        if (tiles_edit_mode == ArtifactEditMode::optimize) {
            return std::make_unique<TilesPngWorkspace>(num_tiles_in_primary);
        }
        panic("unexpected tiles_edit_mode");
    }(tiles_edit_mode_, tileset_, num_tiles_in_primary_.value());

    // Register animations (reserve slots, compile keyframes, register matcher)
    // Must be done before regular tile matching so animation slots are reserved
    PT_TRY_CALL_CHAIN_ERR(pipeline_helper_register_animations(), "failed to register animations", void);

    // Create new Porymap component for output
    new_porymap_component_ = std::make_unique<PorymapTilesetComponent>();

    return {};
}

ChainableResult<void> CompilerTask::pipeline_step_match_tiles_pals()
{
    // Temporary: pals:patch is not yet supported by underlying service code
    if (pals_edit_mode_ == ArtifactEditMode::patch) {
        panic("TODO: implement handling for pals ArtifactEditMode::patch");
    }

    bool matched_all_tiles = true;
    for (std::size_t i = 0; i < porytiles_pixel_rgba_.size(); i++) {
        const auto &porytiles_tile = porytiles_pixel_rgba_[i];

        // In non-optimize mode, first try to reuse existing porymap tile
        if (tiles_edit_mode_ != ArtifactEditMode::optimize) {
            if (const auto maybe_tilemap_entry = pipeline_helper_try_reuse_porymap_tile(i);
                maybe_tilemap_entry.has_value()) {
                new_porymap_component_->push_back_tilemap_entry(maybe_tilemap_entry.value());
                continue;
            }
        }

        // Assign via palette matching (shared logic for all modes)
        const auto tile_assignment_result = pipeline_helper_assign_tile_via_pal_match(porytiles_tile);

        switch (tile_assignment_result.status) {
        case TileAssignmentResult::Status::success:
            new_porymap_component_->push_back_tilemap_entry(tile_assignment_result.entry.value());
            break;

        case TileAssignmentResult::Status::no_covering_pal:
            if (pals_edit_mode_ == ArtifactEditMode::optimize) {
                panic("ArtifactEditMode::optimize but no covering pal found - this should have failed at packing step");
            }
            matched_all_tiles = false;
            pipeline_helper_emit_no_matching_pal_error(i, tile_assignment_result.match_results);
            break;

        case TileAssignmentResult::Status::tile_not_found:
            matched_all_tiles = false;
            pipeline_helper_emit_no_matching_tile_error(
                i,
                tile_assignment_result.index_tile,
                tile_assignment_result.pal_index,
                tile_assignment_result.matched_pal);
            break;

        case TileAssignmentResult::Status::tile_limit_reached:
            matched_all_tiles = false;
            pipeline_helper_emit_tile_limit_error(i, tiles_workspace_->capacity());
            break;
        }

        // Early exit on tile limit, no point printing a bazillion "limit hit" errors after first one
        if (tile_assignment_result.status == TileAssignmentResult::Status::tile_limit_reached) {
            break;
        }
    }

    if (!matched_all_tiles) {
        return ChainableResult<void>{FormattableError{"failed to match all Porytiles tiles"}};
    }

    return {};
}

std::unique_ptr<Tileset> CompilerTask::pipeline_step_assemble_output()
{
    /*
     * TODO: we should track tile+pal use and warn the user here about any unused tiles or pal colors. This would be
     * nice for cases where users add some assets and compile with "tiles/pals:patch", but then later decide to remove
     * the assets. We could warn them these assets are unused so that they can optionally remove to free up space. We
     * could also have a compilation option "force_remove" that forcibly removes unused stuff. This is obviously less
     * safe, since for vanilla primary tilesets, seemingly unused assets may be in use from the secondaries. We can
     * solve this by eventually having code that reads all tileset pairings from layouts.json and computes which primary
     * assets are truly unused. In fact, we'll need something like this in order to truly implement pals:patch mode,
     * since palettes have no "unused" sentinel value. And in fact, many of the vanilla '0 0 0' colors are actually used
     * by secondaries *facepalm* (e.g. see cave tileset). Which means we can't even assume '0 0 0' is unused. Until we
     * implement this, users can still simulate pals:patch by bringing in all Porymap pals as Porytiles override pals,
     * wildcarding slots they are OK overwriting, and setting pals:optimize.
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
    for (std::size_t i = 0; i < porytiles_metatiles_.size(); i++) {
        const auto &metatile = porytiles_metatiles_[i];
        LayerType layer_type;
        if (configured_layer_mode == LayerMode::dual) {
            layer_type = metatile.infer_layer_type(extrinsic_transparency_.value());
        }
        else {
            layer_type = LayerType::normal;
        }
        const auto maybe_porytiles_attr = tileset_.porytiles_component().get_attribute(i);
        MetatileAttribute new_attr{};
        new_attr.layer_type(layer_type);

        // TODO: handle firered/custom stuff here properly
        if (maybe_porytiles_attr.has_value()) {
            // Copy over attribute from Porytiles component, use inferred layer type.
            new_attr.behavior(maybe_porytiles_attr.value().behavior());
        }
        new_porymap_component_->push_back_attribute(new_attr);
    }

    // TODO: Copy over PLA files once we implement handling

    // Export tiles in original form
    if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
        /*
         * TODO: why is using ExportFlipMode::canonical here bugged? I think it has to do with how we computed the flip
         * bits in 'pipeline_helper_try_reuse_porymap_tile' and 'pipeline_helper_assign_tile_via_pal_match'. If we're
         * going to make this configurable, we'll need to check the config value in the matcher functions so we can
         * compute the flip bits correctly.
         */
        new_porymap_component_->tiles_png(
            tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::trim_trailing_transparent));
    }
    else {
        new_porymap_component_->tiles_png(
            tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent));
    }

    // Copy palettes from our processed porymap_pals vector
    for (std::size_t i = 0; i < pal::num_pals; i++) {
        new_porymap_component_->set_pal(i, new_porymap_pals_[i]);
    }

    // Compile animations from Porytiles format to Porymap format
    pipeline_helper_compile_animations();

    // Create the full Tileset and return
    return std::make_unique<Tileset>(
        tileset_.name(), std::move(new_porytiles_component), std::move(new_porymap_component_));
}

std::optional<TilemapEntry> CompilerTask::pipeline_helper_try_reuse_porymap_tile(std::size_t tile_index)
{
    // Preconditions for non-optimize mode
    assert_or_panic(tile_index < porytiles_pixel_rgba_.size(), "tile_index out of bounds for porytiles_pixel_rgba_");
    assert_or_panic(
        porymap_pixel_rgba_.size() == porymap_canonical_pixel_rgba_.size(),
        "porymap_pixel_rgba_.size() != porymap_canonical_pixel_rgba_.size()");
    assert_or_panic(
        porymap_canonical_pixel_rgba_.size() == porymap_tilemap_entries_.size(),
        "porymap_canonical_pixel_rgba_.size() != porymap_tilemap_entries_.size()");

    if (tile_index >= porymap_pixel_rgba_.size()) {
        // tile_index is out-of-range to reuse Porymap assets, so just return nullopt
        return std::nullopt;
    }

    const auto &porytiles_tile = porytiles_pixel_rgba_[tile_index];
    const auto &porymap_tile = porymap_pixel_rgba_[tile_index];
    const auto &canonical_porytiles_tile = porytiles_canonical_pixel_rgba_[tile_index];
    const auto &canonical_porymap_tile = porymap_canonical_pixel_rgba_[tile_index];
    const auto &porymap_tilemap_entry = porymap_tilemap_entries_[tile_index];

    // CASE: Exact match - Porytiles tile exactly matches Porymap tile
    if (porytiles_tile.equals_ignoring_transparency(porymap_tile, extrinsic_transparency_)) {
        return porymap_tilemap_entry;
    }

    // CASE: Canonical match - tiles match under flip transformation
    if (canonical_porytiles_tile.equals_ignoring_transparency(canonical_porymap_tile, extrinsic_transparency_)) {
        // XOR flip bits to compute transformation from Porytiles orientation to Porymap orientation
        const bool pt_to_pm_hflip = canonical_porytiles_tile.h_flip() ^ canonical_porymap_tile.h_flip();
        const bool pt_to_pm_vflip = canonical_porytiles_tile.v_flip() ^ canonical_porymap_tile.v_flip();
        return TilemapEntry{
            porymap_tilemap_entry.tile_index(),
            porymap_tilemap_entry.pal_index(),
            static_cast<bool>(porymap_tilemap_entry.h_flip() ^ pt_to_pm_hflip),
            static_cast<bool>(porymap_tilemap_entry.v_flip() ^ pt_to_pm_vflip)};
    }

    // No match found
    return std::nullopt;
}

TileAssignmentResult CompilerTask::pipeline_helper_assign_tile_via_pal_match(const PixelTile<Rgba32> &porytiles_tile)
{
    TileAssignmentResult result{};

    // TODO: top_n matches should be configurable
    // TODO: what if multiple pals match?
    std::vector<PaletteMatchResult<Rgba32>> matches =
        match_or_best(porytiles_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

    // No covering palette found
    if (!matches.at(0).is_covered) {
        result.status = TileAssignmentResult::Status::no_covering_pal;
        result.match_results = std::move(matches);
        return result;
    }

    const auto pal_index = matches.at(0).pal_index;
    const auto &matched_pal = new_porymap_pals_.at(pal_index);
    const auto index_tile = index_tile_from_color_tile(porytiles_tile, matched_pal, extrinsic_transparency_.value());
    const CanonicalPixelTile canonical_index_tile{index_tile};

    // Check if tile matches a registered animation keyframe
    if (const auto anim_match = anim_tile_matcher_.find_match(CanonicalPixelTile{porytiles_tile});
        anim_match.has_value()) {
        // Use the animation tile index with computed flip bits
        result.status = TileAssignmentResult::Status::success;
        result.entry = TilemapEntry{anim_match->tile_index, pal_index, anim_match->h_flip, anim_match->v_flip};
        return result;
    }

    // Tile found in workspace
    if (const auto maybe_tile_index = tiles_workspace_->first_occurrence_of(canonical_index_tile);
        maybe_tile_index.has_value()) {
        const auto workspace_tile_index = maybe_tile_index.value();
        const auto workspace_tile = tiles_workspace_->tile_at(workspace_tile_index);
        const bool pt_to_pm_hflip = canonical_index_tile.h_flip() ^ workspace_tile.h_flip();
        const bool pt_to_pm_vflip = canonical_index_tile.v_flip() ^ workspace_tile.v_flip();
        result.status = TileAssignmentResult::Status::success;
        result.entry = TilemapEntry{workspace_tile_index, pal_index, pt_to_pm_hflip, pt_to_pm_vflip};
        return result;
    }

    // Tile not found - locked mode cannot insert new tiles
    if (tiles_edit_mode_ == ArtifactEditMode::locked) {
        result.status = TileAssignmentResult::Status::tile_not_found;
        // TODO: pass index_tile or canonical_index_tile depending on user setting for the tiles.png output
        result.index_tile = index_tile;
        result.pal_index = pal_index;
        result.matched_pal = matched_pal;
        return result;
    }

    // Tile not found - check capacity before inserting
    if (tiles_workspace_->at_capacity()) {
        result.status = TileAssignmentResult::Status::tile_limit_reached;
        return result;
    }

    // Insert the new tile
    const std::size_t inserted_index = tiles_workspace_->insert_tile(canonical_index_tile);
    const auto workspace_tile = tiles_workspace_->tile_at(inserted_index);
    result.status = TileAssignmentResult::Status::success;
    const bool pt_to_pm_hflip = canonical_index_tile.h_flip() ^ workspace_tile.h_flip();
    const bool pt_to_pm_vflip = canonical_index_tile.v_flip() ^ workspace_tile.v_flip();
    result.entry = TilemapEntry{inserted_index, pal_index, pt_to_pm_hflip, pt_to_pm_vflip};
    return result;
}

ChainableResult<void> CompilerTask::pipeline_helper_run_pal_packing()
{
    /*
     * Create ColorIndexMap from the Porytiles tiles, Porytiles pals, and palette hints. This validates that we
     * don't exceed the global color count limit.
     */
    const std::size_t color_count_limit = num_pals_in_primary_.value() * (pal::max_size - 1);
    PT_TRY_ASSIGN_CHAIN_ERR(
        color_index_map,
        pipeline_helper_build_color_index_map(pal_hints_.value(), color_count_limit),
        "failed to build color index map for tileset " + tileset_.name(),
        void);

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

    OverloadAndRemoveStrategy packing_strategy{&format_, &diag_};
    PalettePacker pal_packer{&packing_strategy, &format_, &diag_};
    std::bitset<pal::num_pals> available_pals{0};
    for (std::size_t i = 0; i < num_pals_in_primary_; i++) {
        // TODO: support out-of-band primary palettes - see "Primary Palette Fixing" in topic_staging_area.md
        available_pals.set(i, true);
    }
    PackingParams packing_params{};
    packing_params.tiles_ = porytiles_pixel_rgba_;
    packing_params.anims_ = tileset_.porytiles_component().anims();
    packing_params.color_map_ = color_index_map;
    packing_params.extrinsic_transparency_ = extrinsic_transparency_.value();
    packing_params.prefilled_pals_ = tileset_.porytiles_component().pals();
    packing_params.hints_ = pal_hints_.value();
    packing_params.available_pals_ = available_pals;

    PT_TRY_ASSIGN_CHAIN_ERR(
        pal_packing,
        pal_packer.pack_tiles(packing_params),
        "failed to pack palettes for tileset " + tileset_.name(),
        void);

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        if (const auto &maybe_packed_pal = pal_packing.pals_.at(i); maybe_packed_pal.has_value()) {
            // Copy over the packed palette
            new_porymap_pals_[i] = maybe_packed_pal.value();
        }
        else if (tileset_.porytiles_component().pal_at(i).has_value()) {
            /*
             * Out-of-band Porytiles palette: exists but wasn't used in packing (e.g., palette 11.pal in a primary
             * tileset). Resolve all wildcards to black and copy it over.
             */
            const auto &porytiles_pal = tileset_.porytiles_component().pal_at(i).value();
            Palette<Rgba32, pal::max_size> resolved_pal{Rgba32{0, 0, 0, Rgba32::alpha_opaque}};

            // Handle slot 0: preserve if not wildcard, otherwise use extrinsic transparency
            if (!porytiles_pal.is_wildcard(0)) {
                resolved_pal.set(0, porytiles_pal.at(0));
            }
            else {
                resolved_pal.set(0, extrinsic_transparency_.value());
            }

            // Copy non-wildcard slots (wildcards remain as the default black)
            for (std::size_t j = 1; j < pal::max_size; ++j) {
                if (!porytiles_pal.is_wildcard(j)) {
                    resolved_pal.set(j, porytiles_pal.at(j));
                }
            }

            new_porymap_pals_[i] = resolved_pal;
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
            new_porymap_pals_[i] = tileset_.porymap_component().pal_at(i);
        }
    }

    // Emit diagnostic remarks for packed palettes
    for (std::size_t i = 0; i < pal::num_pals; i++) {
        const auto &maybe_packed_pal = pal_packing.pals_.at(i);
        if (maybe_packed_pal.has_value()) {
            constexpr auto tag = "palette-packing-result";
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(
                format_.format("packed '{}' contents:", FormatParam{pal_filename(i), Style::bold}));
            remark_lines.emplace_back();
            std::ranges::copy(
                pal_printer_.print_rgba_palette(maybe_packed_pal.value()), std::back_inserter(remark_lines));
            diag_.remark(tag, remark_lines);
        }
    }

    return {};
}

ChainableResult<ColorIndexMap<Rgba32>> CompilerTask::pipeline_helper_build_color_index_map(
    const std::vector<PaletteHint> &hints, std::size_t color_count_limit) const
{
    // Create ColorIndexMap from the Porytiles tiles
    ColorIndexMap<Rgba32> color_index_map{};
    for (const auto &tile : porytiles_pixel_rgba_) {
        color_index_map.add_tile(tile, extrinsic_transparency_.value());
    }

    // Add Porytiles anims
    for (const auto &anim : tileset_.porytiles_component().anims() | std::views::values) {
        color_index_map.add_anim(anim, extrinsic_transparency_.value());
    }

    // Add Porytiles palettes
    /*
     * TODO: we should be respecting the num_pals_in_primary fieldmap setting here: don't add palettes for pals that
     * aren't active.
     */
    for (std::size_t pal_index = 0; pal_index < tileset_.porytiles_component().pals().size(); ++pal_index) {
        const auto &maybe_porytiles_pal = tileset_.porytiles_component().pals().at(pal_index);
        if (!maybe_porytiles_pal.has_value()) {
            continue;
        }
        color_index_map.add_pal(maybe_porytiles_pal.value(), extrinsic_transparency_.value());
    }

    // Add palette hints
    for (const auto &hint : hints) {
        color_index_map.add_pal(hint.pal(), extrinsic_transparency_.value());
    }

    // Check color count one more time, we validated this earlier and provided granular feedback to user
    if (color_index_map.size() > color_count_limit) {
        panic(
            "color_index_map.size() > count_limit - this should have already been validated by "
            "pipeline_step_validate_input");
    }

    return color_index_map;
}

ChainableResult<void> CompilerTask::pipeline_helper_register_animations()
{
    /*
     * This function has two primary responsibilities. For each anim:
     *
     * 1. Place the anim's key frame tiles into tiles.png at computed offsets
     * 2. Register each animation and save the computed offsets
     *
     * The strategy differs by mode:
     * - optimize: Reserve slots at the start, place keyframes in reserved region
     * - patch: Try to reuse existing keyframes, else find contiguous free space
     * - locked: Keyframes must already exist in tiles.png
     */
    const auto &anims = tileset_.porytiles_component().anims();

    // Early exit if no animations
    if (anims.empty()) {
        return {};
    }

    // TODO: ANIM: these register functions are 75% code dupe
    if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
        return pipeline_helper_register_animations_optimize();
    }
    if (tiles_edit_mode_ == ArtifactEditMode::patch) {
        return pipeline_helper_register_animations_patch();
    }
    if (tiles_edit_mode_ == ArtifactEditMode::locked) {
        return pipeline_helper_register_animations_locked();
    }

    panic("unexpected tiles_edit_mode");
}

ChainableResult<void> CompilerTask::pipeline_helper_register_animations_optimize()
{
    const auto &anims = tileset_.porytiles_component().anims();

    // Compute total key frame tiles needed across all animations
    std::size_t total_key_frame_tiles = 0;
    for (const auto &anim : anims | std::views::values) {
        if (anim.has_frames()) {
            total_key_frame_tiles += anim.key_frame().tiles().size();
        }
    }

    // Reserve animation slots in workspace (starting at index 1)
    tiles_workspace_->reserve_anim_slots(total_key_frame_tiles);

    // Build a map of animation offsets for registration step
    std::map<std::string, std::size_t> anim_offsets; // name -> tile_offset

    // Process each animation: compile key frame tiles
    std::size_t current_offset = TilesPngWorkspace::anim_start_offset();
    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_frames()) {
            panic("anim '" + anim_name + "' has no frames");
        }

        const AnimationFrame<Rgba32> &composite_frame = anim.composite_frame(extrinsic_transparency_);
        const std::size_t tile_count = composite_frame.tiles().size();
        const std::size_t this_anim_offset = current_offset;

        // Process each composite and key frame tile
        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            const PixelTile<Rgba32> &composite_rgba_tile = composite_frame.tile_at(tile_idx);
            const PixelTile<Rgba32> &key_rgba_tile = anim.key_frame().tile_at(tile_idx);

            if (key_rgba_tile.is_transparent(extrinsic_transparency_.value())) {
                panic("illegal transparent key frame tile");
            }

            /*
             * Match tile to palette, we use the composite tile to guarantee that we're selecting the correct palette,
             * i.e. the one that can fully cover this subtile of the animation. The earlier palette packing step already
             * guaranteed this.
             */
            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_rgba_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

            // TODO: ANIM: better error message here
            if (!matches.at(0).is_covered) {
                return FormattableError{
                    "animation '{}' composite subtile '{}' has no covering palette",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{tile_idx, Style::bold}};
            }

            // Convert key frame tile to IndexPixel using matched palette
            const std::size_t pal_index = matches.at(0).pal_index;
            const auto &matched_pal = new_porymap_pals_.at(pal_index);
            const PixelTile<IndexPixel> indexed_key_frame_tile =
                index_tile_from_color_tile(key_rgba_tile, matched_pal, extrinsic_transparency_.value());
            const CanonicalPixelTile canonical_key_frame_tile{indexed_key_frame_tile};

            // Place in reserved workspace slot
            const std::size_t reserved_index = current_offset - TilesPngWorkspace::anim_start_offset();
            tiles_workspace_->place_anim_tile(reserved_index, canonical_key_frame_tile);

            ++current_offset;
        }

        // Store offset info for later use
        anim_offsets[anim_name] = this_anim_offset;
    }

    // Register all animations with the matcher for keyframe tile detection
    for (const auto &[anim_name, anim] : anims) {
        const std::size_t tile_offset = anim_offsets[anim_name];

        // Register with matcher
        anim_tile_matcher_.register_animation(anim_name, anim, tile_offset, extrinsic_transparency_);
    }

    return {};
}

ChainableResult<void> CompilerTask::pipeline_helper_register_animations_patch()
{
    const auto &anims = tileset_.porytiles_component().anims();

    // Build a map of animation offsets for registration step
    std::map<std::string, std::size_t> anim_offsets; // name -> tile_offset

    // Process each animation
    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_frames()) {
            panic("anim '" + anim_name + "' has no frames");
        }

        const AnimationFrame<Rgba32> &composite_frame = anim.composite_frame(extrinsic_transparency_);
        const std::size_t tile_count = composite_frame.tiles().size();

        // Build canonical IndexPixel keyframe tiles for this animation
        std::vector<CanonicalPixelTile<IndexPixel>> keyframe_tiles;
        keyframe_tiles.reserve(tile_count);

        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            const PixelTile<Rgba32> &composite_rgba_tile = composite_frame.tile_at(tile_idx);
            const PixelTile<Rgba32> &key_rgba_tile = anim.key_frame().tile_at(tile_idx);

            if (key_rgba_tile.is_transparent(extrinsic_transparency_.value())) {
                panic("illegal transparent key frame tile");
            }

            // Match tile to palette using composite frame
            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_rgba_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

            if (!matches.at(0).is_covered) {
                // TODO: ANIM: better error message here
                return FormattableError{
                    "animation '{}' composite subtile '{}' has no covering palette",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{tile_idx, Style::bold}};
            }

            // Convert key frame tile to IndexPixel using matched palette
            const std::size_t pal_index = matches.at(0).pal_index;
            const auto &matched_pal = new_porymap_pals_.at(pal_index);
            /*
             * TODO: ANIM: this is broken. E.g. when compiling vanilla general waterfall, it breaks because pal 04.pal
             * has duplicate colors in slot 7 and slot 14 and the vanilla game uses the slot 14 instance of the color.
             * This means that our function here computes an index tile that doesn't match (it always chooses first slot
             * matching color, so it chooses slot 7), and so the find_existing_contiguous_tiles call below fails to find
             * the block of tiles it should.
             *
             * One idea for a solution: index_tile_from_color_tile computes all possible index tiles (accounting for
             * this fact), and we try them all. This is not a great solution.
             *
             * It turns out, color 7 in 04.pal is unused. Maybe we can leverage that somehow.
             *
             * We need to figure this out, because it totally breaks gTileset_General patch builds.
             */
            const PixelTile<IndexPixel> indexed_key_frame_tile =
                index_tile_from_color_tile(key_rgba_tile, matched_pal, extrinsic_transparency_.value());

            keyframe_tiles.emplace_back(indexed_key_frame_tile);
        }

        // Step 1: Try to find existing contiguous keyframe sequence in workspace
        if (const auto existing_offset = tiles_workspace_->find_existing_contiguous_tiles(keyframe_tiles);
            existing_offset.has_value()) {
            // Reuse existing keyframes
            anim_offsets[anim_name] = existing_offset.value();
            continue;
        }

        // Step 2: Find contiguous free space for new keyframes
        if (const auto free_offset = tiles_workspace_->find_contiguous_transparent_slots(tile_count);
            free_offset.has_value()) {
            // Place tiles at the found location
            tiles_workspace_->place_tiles_at(free_offset.value(), keyframe_tiles);
            anim_offsets[anim_name] = free_offset.value();
            continue;
        }

        // Step 3: No space found - error
        // TODO: ANIM: better error message here
        return FormattableError{
            "animation '{}' requires {} contiguous tiles but no sufficient space found",
            FormatParam{anim_name, Style::bold},
            FormatParam{tile_count, Style::bold}};
    }

    // Register all animations with the matcher for keyframe tile detection
    for (const auto &[anim_name, anim] : anims) {
        const std::size_t tile_offset = anim_offsets[anim_name];

        // Register with matcher
        anim_tile_matcher_.register_animation(anim_name, anim, tile_offset, extrinsic_transparency_);
    }

    return {};
}

ChainableResult<void> CompilerTask::pipeline_helper_register_animations_locked()
{
    const auto &anims = tileset_.porytiles_component().anims();

    // Build a map of animation offsets for registration step
    std::map<std::string, std::size_t> anim_offsets; // name -> tile_offset

    // Process each animation - in locked mode, keyframes must already exist
    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_frames()) {
            panic("anim '" + anim_name + "' has no frames");
        }

        const AnimationFrame<Rgba32> &composite_frame = anim.composite_frame(extrinsic_transparency_);
        const std::size_t tile_count = composite_frame.tiles().size();

        // Build canonical IndexPixel keyframe tiles for this animation
        std::vector<CanonicalPixelTile<IndexPixel>> keyframe_tiles;
        keyframe_tiles.reserve(tile_count);

        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            const PixelTile<Rgba32> &composite_rgba_tile = composite_frame.tile_at(tile_idx);
            const PixelTile<Rgba32> &key_rgba_tile = anim.key_frame().tile_at(tile_idx);

            if (key_rgba_tile.is_transparent(extrinsic_transparency_.value())) {
                panic("illegal transparent key frame tile");
            }

            // Match tile to palette using composite frame
            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_rgba_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

            if (!matches.at(0).is_covered) {
                return FormattableError{
                    "animation '{}' composite subtile '{}' has no covering palette",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{tile_idx, Style::bold}};
            }

            // Convert key frame tile to IndexPixel using matched palette
            const std::size_t pal_index = matches.at(0).pal_index;
            const auto &matched_pal = new_porymap_pals_.at(pal_index);
            const PixelTile<IndexPixel> indexed_key_frame_tile =
                index_tile_from_color_tile(key_rgba_tile, matched_pal, extrinsic_transparency_.value());

            keyframe_tiles.emplace_back(indexed_key_frame_tile);
        }

        // In locked mode, keyframes MUST already exist contiguously
        if (const auto existing_offset = tiles_workspace_->find_existing_contiguous_tiles(keyframe_tiles);
            existing_offset.has_value()) {
            anim_offsets[anim_name] = existing_offset.value();
        }
        else {
            // TODO: better error here
            return FormattableError{
                "Tiles edit_mode is '{}': animation '{}' keyframes not found in existing tiles.png",
                FormatParam{"locked", Style::bold},
                FormatParam{anim_name, Style::bold}};
        }
    }

    // Register all animations with the matcher for keyframe tile detection
    for (const auto &[anim_name, anim] : anims) {
        const std::size_t tile_offset = anim_offsets[anim_name];

        // Register with matcher
        anim_tile_matcher_.register_animation(anim_name, anim, tile_offset, extrinsic_transparency_);
    }

    return {};
}

void CompilerTask::pipeline_helper_compile_animations()
{
    const auto &source_anims = tileset_.porytiles_component().anims();

    // Early exit if no animations
    if (source_anims.empty()) {
        return;
    }

    for (const auto &[anim_name, source_anim] : source_anims) {
        // 1. Get the computed tile offset from matcher
        auto maybe_tile_offset = anim_tile_matcher_.tile_offset_for(anim_name);
        if (!maybe_tile_offset.has_value()) {
            panic("animation '" + anim_name + "' not registered in anim_tile_matcher_");
        }
        const std::size_t tile_offset = maybe_tile_offset.value();

        // 2. Compute composite frame for per-subtile palette selection
        const AnimationFrame<Rgba32> composite = source_anim.composite_frame(extrinsic_transparency_.value());
        const std::size_t tile_count = composite.tile_count();

        // 3. Build per-subtile palette indices (same logic as registration step)
        std::vector<std::size_t> subtile_pal_indices;
        subtile_pal_indices.reserve(tile_count);

        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            const PixelTile<Rgba32> &composite_tile = composite.tile_at(tile_idx);

            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

            if (!matches.at(0).is_covered) {
                panic(
                    "animation '" + anim_name + "' subtile " + std::to_string(tile_idx) +
                    " has no covering palette during compilation");
            }

            subtile_pal_indices.push_back(matches.at(0).pal_index);
        }

        // 4. Determine palette for PNG display and warn if multiple palettes are used
        const std::size_t frame_pal_index = subtile_pal_indices.at(0);
        const bool uses_multiple_palettes =
            !std::ranges::all_of(subtile_pal_indices, [&](std::size_t idx) { return idx == frame_pal_index; });

        if (uses_multiple_palettes) {
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(format_.format(
                "animation '{}' uses multiple palettes across subtiles", FormatParam{anim_name, Style::bold}));
            warning_lines.emplace_back(format_.format(
                "Frame PNGs will be saved using palette '{}' for display purposes.",
                FormatParam{pal_filename(frame_pal_index), Style::bold}));
            diag_.warning("multi-palette-animation", warning_lines);
        }

        // Build a dynamic palette for embedding in the AnimationFrame
        const auto &fixed_pal = new_porymap_pals_.at(frame_pal_index);
        Palette<Rgba32> anim_palette{};
        for (std::size_t i = 0; i < fixed_pal.size(); ++i) {
            if (fixed_pal.is_wildcard(i)) {
                panic("Porymap pal '" + std::to_string(frame_pal_index) + "' has illegal wildcard");
            }
            anim_palette.add(fixed_pal.at(i));
        }

        // 5. Convert regular frames (key frame not needed in compiled format)
        Animation<IndexPixel> compiled_anim{anim_name};

        for (const auto &[frame_name, source_frame] : source_anim.frames()) {
            std::vector<PixelTile<IndexPixel>> frame_index_tiles;
            frame_index_tiles.reserve(tile_count);

            for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
                const PixelTile<Rgba32> &rgba_tile = source_frame.tile_at(tile_idx);
                const auto &pal = new_porymap_pals_.at(subtile_pal_indices[tile_idx]);

                frame_index_tiles.push_back(
                    index_tile_from_color_tile(rgba_tile, pal, extrinsic_transparency_.value()));
            }

            AnimationFrame<IndexPixel> frame{frame_name, std::move(frame_index_tiles)};
            frame.palette(anim_palette);
            compiled_anim.put_frame(frame_name, std::move(frame));
        }

        // 6. Set params with updated tile_offset/tile_count
        AnimationParams params = source_anim.params();
        params.tile_offset(tile_offset);
        params.tile_count(tile_count);
        compiled_anim.params(std::move(params));

        // 7. Add to output component (key_frame left as std::nullopt)
        new_porymap_component_->add_anim(std::move(compiled_anim));
    }
}

void CompilerTask::pipeline_helper_emit_no_matching_tile_error(
    std::size_t tile_index,
    const PixelTile<IndexPixel> &index_tile,
    std::size_t pal_index,
    const Palette<Rgba32, pal::max_size> &matched_pal)
{
    constexpr auto tag = "no-matching-tile";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> no_match_err{};
    no_match_err.emplace_back(format_.format(
        "{}: no matching tile found",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
    std::ranges::copy(
        tile_printer_.print_metatile_tile_highlight(
            porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_),
        std::back_inserter(no_match_err));
    diag_.error(tag, no_match_err);

    // Print note showing the palette that matched
    std::vector<std::string> pal_note{};
    pal_note.emplace_back(format_.format("matched palette '{}':", FormatParam{pal_filename(pal_index), Style::bold}));
    std::ranges::copy(pal_printer_.print_rgba_palette(matched_pal), std::back_inserter(pal_note));
    diag_.note(tag, pal_note);

    // Print note showing the generated IndexPixel tile
    std::vector<std::string> tile_note{};
    tile_note.emplace_back("generated index tile:");
    std::ranges::copy(
        tile_printer_.print_tile(index_tile, extrinsic_transparency_.value()), std::back_inserter(tile_note));
    diag_.note(tag, tile_note);
}

void CompilerTask::pipeline_helper_emit_no_matching_pal_error(
    std::size_t tile_index, const std::vector<PaletteMatchResult<Rgba32>> &matches)
{
    constexpr auto tag = "no-matching-palette";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> no_match_err{};
    no_match_err.emplace_back(format_.format(
        "{}: no matching palette found",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
    std::ranges::copy(
        tile_printer_.print_metatile_tile_highlight(
            porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_),
        std::back_inserter(no_match_err));
    diag_.error(tag, no_match_err);

    // Emit a long note showing the top N closest matches
    std::vector<std::string> closest_n_note{};
    // TODO: substitute configurable top_n for N
    closest_n_note.emplace_back("closest N match(es) with covered colors highlighted:");
    int match_index = 0;
    for (const auto &match : matches) {
        if (match_index != 0) {
            // Add a blank line between subsequent matches
            closest_n_note.emplace_back();
        }
        closest_n_note.push_back(
            format_.format("Palette match candidate: {}", FormatParam{pal_filename(match.pal_index), Style::bold}));
        std::ranges::copy(
            pal_printer_.print_rgba_palette_covered_missing(
                new_porymap_pals_.at(match.pal_index), match.covered_colors, match.missing_colors),
            std::back_inserter(closest_n_note));
        closest_n_note.emplace_back();
        closest_n_note.push_back(
            format_.format("Uncovered pixels with {}:", FormatParam{pal_filename(match.pal_index), Style::bold}));
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

void CompilerTask::pipeline_helper_emit_tile_limit_error(std::size_t tile_index, std::size_t tile_limit)
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
    diag_.error(tag, tile_limit_error);

    // Construct note text
    std::vector<std::string> note_text;
    note_text.push_back(
        format_.format("tile limit is '{}' due to configuration", FormatParam{num_tiles_in_primary_, Style::bold}));
    note_text.emplace_back();
    std::ranges::copy(num_tiles_in_primary_.prettify(format_), std::back_inserter(note_text));
    diag_.note(tag, note_text);
}

} // namespace

namespace porytiles2 {

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
