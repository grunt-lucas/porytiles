#include "porytiles2/domain/services/tileset_compiler.hpp"

#include <array>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/algorithms/palette_matchers.hpp"
#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/algorithms/tileset_compile_validators.hpp"
#include "porytiles2/domain/config/artifact_edit_mode.hpp"
#include "porytiles2/domain/config/frame_linking.hpp"
#include "porytiles2/domain/config/packing_strategy_params.hpp"
#include "porytiles2/domain/config/packing_strategy_type.hpp"
#include "porytiles2/domain/config/per_anim_overrides.hpp"
#include "porytiles2/domain/config/tiles_pal_mode.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tiles_png_workspace.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/packing/services/backtracking_strategy.hpp"
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
 * @brief Creates a packing strategy instance based on config settings.
 *
 * @details
 * If the selected strategy's parameter block has any values set, constructs the strategy in single-config mode with the
 * provided parameters (unset fields fall back to their per-field defaults). Otherwise, constructs the strategy in
 * preset matrix mode. BestFusionStrategy has no parameters and always uses its parameterless constructor.
 *
 * @param strategy_type The selected packing algorithm
 * @param params Per-strategy parameter blocks
 * @param diag Diagnostics interface for remarks about successful search parameters
 * @return A unique_ptr to the configured PackingStrategy
 */
[[nodiscard]] std::unique_ptr<PackingStrategy> make_packing_strategy(
    PackingStrategyType strategy_type, const PackingStrategyParams &params, const UserDiagnostics &diag)
{
    switch (strategy_type) {
    case PackingStrategyType::best_fusion:
        return std::make_unique<BestFusionStrategy>();
    case PackingStrategyType::backtracking: {
        const auto &cfg = params.backtracking;
        if (!cfg.has_any()) {
            return std::make_unique<BacktrackingStrategy>(&diag);
        }
        return std::make_unique<BacktrackingStrategy>(
            cfg.search_algorithm.value.value_or(SearchAlgorithm::dfs),
            cfg.node_cutoff.value.value_or(1'000'000),
            cfg.best_branches.value.value_or(std::numeric_limits<std::size_t>::max()),
            cfg.smart_prune.value.value_or(true),
            &diag);
    }
    case PackingStrategyType::overload_and_remove: {
        const auto &cfg = params.overload_and_remove;
        if (!cfg.has_any()) {
            return std::make_unique<OverloadAndRemoveStrategy>(&diag);
        }
        return std::make_unique<OverloadAndRemoveStrategy>(
            cfg.max_attempts.value.value_or(20),
            cfg.seed.value.value_or(42),
            cfg.shuffle_strategy.value.value_or(ShuffleStrategy::noisy_ffd),
            &diag);
    }
    }
    panic("Unhandled PackingStrategyType value.");
}

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
 * @brief Data structure holding processed keyframe tiles and their matched palettes.
 *
 * @details
 * Used to pass the results of building keyframe data from the common helper to the mode-specific placement logic. The
 * palettes vector is only used in patch mode for color-equivalence matching; optimize and locked modes ignore it.
 */
struct AnimKeyframeData {
    std::vector<CanonicalPixelTile<IndexPixel>> tiles;
    std::vector<const Palette<Rgba32, pal::max_size> *> palettes;
    std::vector<std::size_t> pal_indices;
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
        const Tileset *paired_primary,
        const TextFormatter &format,
        const UserDiagnostics &diag,
        const TilePrinter &tile_printer,
        const PalettePrinter &pal_printer,
        const DomainConfig &config)
        : tileset_{tileset}, paired_primary_{paired_primary}, format_{format}, diag_{diag}, tile_printer_{tile_printer},
          pal_printer_{pal_printer}, config_{config}, extrinsic_transparency_{}, num_pals_in_primary_{},
          num_pals_total_{}, num_metatiles_in_primary_{}, num_tiles_in_primary_{}, num_tiles_per_metatile_{},
          pal_hints_enabled_{}, pal_hints_{}
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
    pipeline_helper_assign_tile_via_pal_match(const PixelTile<Rgba32> &porytiles_tile, std::size_t flat_index);

    // Pipeline helpers - palette packing
    [[nodiscard]] ChainableResult<void> pipeline_helper_run_pal_packing();
    [[nodiscard]] ChainableResult<ColorIndexMap<Rgba32>>
    pipeline_helper_build_color_index_map(const std::vector<PaletteHint> &hints, std::size_t color_count_limit) const;
    // Pipeline helpers - animation processing
    [[nodiscard]] ChainableResult<void> pipeline_helper_register_animations();
    [[nodiscard]] ChainableResult<AnimKeyframeData>
    pipeline_helper_build_keyframe_data(const std::string &anim_name, const Animation<Rgba32> &anim) const;
    void pipeline_helper_compile_animations();
    void pipeline_helper_apply_manual_overrides();

    // Pipeline helpers - true_color mode
    void pipeline_helper_apply_true_color_to_tiles_png();

    [[nodiscard]] bool is_secondary() const
    {
        return paired_primary_ != nullptr;
    }

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
    const Tileset *paired_primary_;
    const TextFormatter &format_;
    const UserDiagnostics &diag_;
    const TilePrinter &tile_printer_;
    const PalettePrinter &pal_printer_;
    const DomainConfig &config_;

    // Config values (populated in run())
    ConfigValue<Rgba32> extrinsic_transparency_;
    ConfigValue<std::size_t> num_pals_in_primary_;
    ConfigValue<std::size_t> num_pals_total_;
    ConfigValue<std::size_t> num_metatiles_in_primary_;
    ConfigValue<std::size_t> num_tiles_in_primary_;
    ConfigValue<std::size_t> num_tiles_total_;
    ConfigValue<std::size_t> num_tiles_per_metatile_;
    ConfigValue<bool> pal_hints_enabled_;
    ConfigValue<std::vector<PaletteHint>> pal_hints_;
    ConfigValue<ArtifactEditMode> tiles_edit_mode_;
    ConfigValue<ArtifactEditMode> pals_edit_mode_;
    ConfigValue<TilesPalMode> tiles_pal_mode_;
    ConfigValue<FrameLinking> global_frame_linking_;
    ConfigValue<PerAnimOverrides> per_anim_overrides_;

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
    std::map<std::size_t, std::size_t> tile_to_pal_{};

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
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_total, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_per_metatile, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, pal_hints_enabled, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, pal_hints, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tiles_edit_mode, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, pals_edit_mode, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tiles_pal_mode, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, global_frame_linking, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, per_anim_overrides, tileset_.name(), std::unique_ptr<Tileset>);

    extrinsic_transparency_ = extrinsic_transparency;
    num_pals_in_primary_ = num_pals_in_primary;
    num_pals_total_ = num_pals_total;
    num_metatiles_in_primary_ = num_metatiles_in_primary;
    num_tiles_in_primary_ = num_tiles_in_primary;
    num_tiles_total_ = num_tiles_total;
    num_tiles_per_metatile_ = num_tiles_per_metatile;
    pal_hints_enabled_ = pal_hints_enabled;
    pal_hints_ = pal_hints;
    tiles_edit_mode_ = tiles_edit_mode;
    pals_edit_mode_ = pals_edit_mode;
    tiles_pal_mode_ = tiles_pal_mode;
    global_frame_linking_ = global_frame_linking;
    per_anim_overrides_ = per_anim_overrides;

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
        void,
        "failed to metatileize input layer images for " + tileset_.name());
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
        void,
        std::format("Failed to triple-layerize Porymap component for tileset '{}'.", tileset_.name()));
    porymap_tilemap_entries_ = std::move(tilemap_entries);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            porymap_tilemap_entries_, tileset_.porymap_component().tiles_png(), tileset_.porymap_component().pals()),
        void,
        std::format("Failed to decompile Porymap component for tileset '{}'.", tileset_.name()));
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
    TilesetCompileValidatorServices services{config_, diag_, tile_printer_, pal_printer_};

    /*
     * TODO: do we want to collate some of these before returning? It would present more errors to user at once. There
     * are pros and cons to this.
     */

    // Run metatile count validation
    PT_TRY_CALL_PASS_ERR(
        validate_metatile_count(services, tileset_.name(), is_secondary(), porytiles_metatiles_), void);

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
            is_secondary(),
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
    // Secondary compilation only supports optimize mode for now (locked/patch deferred)
    if (is_secondary() && tiles_edit_mode_ != ArtifactEditMode::optimize) {
        panic("Secondary compilation only supports tiles ArtifactEditMode::optimize (locked/patch deferred).");
    }
    if (is_secondary() && pals_edit_mode_ != ArtifactEditMode::optimize) {
        panic("Secondary compilation only supports pals ArtifactEditMode::optimize (locked/patch deferred).");
    }

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
    if (tiles_edit_mode_ == ArtifactEditMode::locked) {
        /*
         * When tiles are locked, compute the exact size of tiles.png so we keep it completely unchanged. When we
         * output, we'll also set ExportTrimMode::include_trailing_transparent so that if there was transparency at
         * the end, we don't remove it.
         */
        const auto size_in_tiles = tileset_.porymap_component().tiles_png().size_in_tiles();
        tiles_workspace_ =
            std::make_unique<TilesPngWorkspace>(tileset_.porymap_component().tiles_png(), size_in_tiles);
    }
    else if (tiles_edit_mode_ == ArtifactEditMode::patch) {
        // TODO: here, should we compute the size and match the size like above?
        tiles_workspace_ = std::make_unique<TilesPngWorkspace>(
            tileset_.porymap_component().tiles_png(), num_tiles_in_primary_.value());
    }
    else if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
        if (is_secondary()) {
            tiles_workspace_ = std::make_unique<TilesPngWorkspace>(TilesPngWorkspace::for_secondary(
                paired_primary_->porymap_component().tiles_png(),
                num_tiles_in_primary_.value(),
                num_tiles_total_.value()));
        }
        else {
            tiles_workspace_ = std::make_unique<TilesPngWorkspace>(num_tiles_in_primary_.value());
        }
    }
    else {
        panic("unexpected tiles_edit_mode");
    }

    // Register animations (reserve slots, compile keyframes, register matcher)
    // Must be done before regular tile matching so animation slots are reserved
    PT_TRY_CALL_CHAIN_ERR(pipeline_helper_register_animations(), void, "Failed to register animations.");

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

        /*
         * Transparent tiles always map to tile index 0 (the reserved transparent tile).
         *
         * If tile 0 transparency is a pokeemerald convention, why does this come after the
         * pipeline_helper_try_reuse_porymap_tile step for non-tiles-optimize builds? It's because Porytiles2 design
         * philosophy prioritizes surgical edits where possible. A user could have other locations in tiles.png marked
         * transparent in addition to tile 0. If one of their metatiles referenced one of these alternate locations, we
         * don't want to create a diff by forcing the metatile reference to change to tile 0. Instead, we'll just
         * respect the idiosyncrasy by calling pipeline_helper_try_reuse_porymap_tile and letting it match there first.
         */
        if (porytiles_tile.is_transparent(extrinsic_transparency_.value())) {
            new_porymap_component_->push_back_tilemap_entry(TilemapEntry{0, 0, false, false});
            continue;
        }

        // Assign via palette matching (shared logic for all modes)
        const auto tile_assignment_result = pipeline_helper_assign_tile_via_pal_match(porytiles_tile, i);

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
            {
                const std::size_t user_visible_tile_limit = is_secondary()
                    ? (num_tiles_total_.value() - num_tiles_in_primary_.value())
                    : num_tiles_in_primary_.value();
                pipeline_helper_emit_tile_limit_error(i, user_visible_tile_limit);
            }
            break;
        }

        // Early exit on tile limit, no point printing a bazillion "limit hit" errors after first one
        if (tile_assignment_result.status == TileAssignmentResult::Status::tile_limit_reached) {
            break;
        }
    }

    if (!matched_all_tiles) {
        return ChainableResult<void>{FormattableError{"Failed to match all Porytiles tiles."}};
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
     * assets are truly unused.
     *
     * In fact, we'll need something like this in order to truly implement pals:patch mode, since palettes have no
     * "unused" sentinel value. And in fact, many of the vanilla '0 0 0' colors are actually used by secondaries
     * *facepalm* (e.g. see cave tileset). Which means we can't even assume '0 0 0' is unused. Until we implement this,
     * users can still simulate pals:patch by bringing in all Porymap pals as Porytiles override pals, wildcarding slots
     * they are OK overwriting, and setting pals:optimize.
     */

    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset_.porytiles_component());

    // Update porytiles component animation params with computed tile offsets
    for (auto &[anim_name, anim] : new_porytiles_component->anims()) {
        if (auto maybe_offset = anim_tile_matcher_.tile_offset_for(anim_name); maybe_offset.has_value()) {
            AnimParams updated_params = anim.params();
            updated_params.tile_offset(maybe_offset.value());
            anim.params(std::move(updated_params));
        }
    }

    // Compile animations from Porytiles format to Porymap format
    pipeline_helper_compile_animations();

    // Apply manual animation overrides to metatiles_bin (must happen before dual-layerization)
    pipeline_helper_apply_manual_overrides();

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
        if (maybe_porytiles_attr.has_value()) {
            new_attr = maybe_porytiles_attr.value();
        }
        new_attr.layer_type(layer_type);
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
        if (is_secondary()) {
            new_porymap_component_->tiles_png(tiles_workspace_->export_secondary_image(
                num_tiles_in_primary_.value(), ExportFlipMode::original, ExportTrimMode::trim_trailing_transparent));
        }
        else {
            new_porymap_component_->tiles_png(
                tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::trim_trailing_transparent));
        }
    }
    else {
        new_porymap_component_->tiles_png(
            tiles_workspace_->export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent));
    }

    // Copy palettes to output
    if (is_secondary()) {
        // Primary palettes from paired primary
        for (std::size_t i = 0; i < num_pals_in_primary_.value(); i++) {
            new_porymap_component_->set_pal(i, paired_primary_->porymap_component().pal_at(i));
        }
        // Secondary palettes from packing result
        for (std::size_t i = num_pals_in_primary_.value(); i < num_pals_total_.value(); i++) {
            new_porymap_component_->set_pal(i, new_porymap_pals_.at(i));
        }
        // Junk/reserved palettes (13-15) from original secondary component
        for (std::size_t i = num_pals_total_.value(); i < pal::num_pals; i++) {
            new_porymap_component_->set_pal(i, tileset_.porymap_component().pal_at(i));
        }
    }
    else {
        for (std::size_t i = 0; i < pal::num_pals; i++) {
            new_porymap_component_->set_pal(i, new_porymap_pals_.at(i));
        }
    }

    // Apply true_color palette encoding to tiles.png if configured
    if (tiles_pal_mode_ == TilesPalMode::true_color) {
        pipeline_helper_apply_true_color_to_tiles_png();
    }

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

TileAssignmentResult
CompilerTask::pipeline_helper_assign_tile_via_pal_match(const PixelTile<Rgba32> &porytiles_tile, std::size_t flat_index)
{
    TileAssignmentResult result{};

    /*
     * Use the packer's authoritative palette assignment when available (optimize mode). This ensures tile sharing
     * alignment is respected. The packer and alignment system chose specific palettes for each tile, and re-deriving
     * via match_or_best could pick a different palette that breaks sharing slot alignment.
     *
     * Falls back to match_or_best for tiles not in the packer's assignments (e.g., locked/patch modes, or tiles
     * excluded from packing like animation keyframes).
     */
    // TODO: top_n matches should be configurable
    std::size_t pal_index;
    if (tile_to_pal_.contains(flat_index)) {
        pal_index = tile_to_pal_.at(flat_index);
    }
    else {
        std::vector<PaletteMatchResult<Rgba32>> matches =
            match_or_best(porytiles_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

        if (!matches.at(0).is_covered) {
            result.status = TileAssignmentResult::Status::no_covering_pal;
            result.match_results = std::move(matches);
            return result;
        }
        pal_index = matches.at(0).pal_index;
    }

    const auto &matched_pal = new_porymap_pals_.at(pal_index);
    const auto index_tile = index_tile_from_color_tile(porytiles_tile, matched_pal, extrinsic_transparency_.value());
    const CanonicalPixelTile canonical_index_tile{index_tile};

    /*
     * In non-optimize modes with available original tilemap data, only use the animation matcher if the original
     * tile_index was within a registered animation range. This prevents false positive interception where a static tile
     * that visually matches an animation keyframe gets incorrectly mapped to the animation tile_index, causing
     * unintended animation at runtime.
     */
    bool should_check_anim_matcher = true;
    if (tiles_edit_mode_ != ArtifactEditMode::optimize && flat_index < porymap_tilemap_entries_.size()) {
        const auto original_tile_index = porymap_tilemap_entries_[flat_index].tile_index();
        should_check_anim_matcher = anim_tile_matcher_.is_in_animation_range(original_tile_index);
    }

    // Check if tile matches a registered animation keyframe
    if (should_check_anim_matcher) {
        if (const auto anim_match = anim_tile_matcher_.find_match(CanonicalPixelTile{porytiles_tile});
            anim_match.has_value()) {
            // Use the animation tile index with composite-aware palette and computed flip bits
            result.status = TileAssignmentResult::Status::success;
            result.entry =
                TilemapEntry{anim_match->tile_index, anim_match->pal_index, anim_match->h_flip, anim_match->v_flip};
            return result;
        }
    }

    /*
     * Tile found in workspace
     *
     * In optimize mode, we use fast O(1) exact index matching because palettes are freshly computed by the palette
     * packing algorithm, which never produces duplicate colors. In patch/locked modes, we use O(n) color-equivalence
     * comparison because vanilla palettes may contain duplicate colors at different indices. For example, if palette
     * slots 7 and 14 both contain RGB(255,0,0), our index_tile_from_color_tile() always picks slot 7 (the first
     * match), but vanilla workspace tiles might use slot 14. Exact index matching would fail to find the tile, causing
     * unnecessary tile insertions or "tile not found" errors in locked mode.
     */
    const auto maybe_tile_index =
        (tiles_edit_mode_ == ArtifactEditMode::optimize)
            ? tiles_workspace_->first_occurrence_of(canonical_index_tile)
            : tiles_workspace_->first_occurrence_of_by_color(canonical_index_tile, matched_pal);

    if (maybe_tile_index.has_value()) {
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
     * Create ColorIndexMap from the Porytiles tiles, Porytiles pals, and palette hints. We already validated earlier
     * that we don't exceed the global color count limit. So this will panic if there are too many global unique colors.
     */
    const std::size_t color_count_limit =
        is_secondary() ? (num_pals_total_.value() - num_pals_in_primary_.value()) * (pal::max_size - 1)
                        : num_pals_in_primary_.value() * (pal::max_size - 1);
    PT_TRY_ASSIGN_CHAIN_ERR(
        color_index_map,
        pipeline_helper_build_color_index_map(pal_hints_.value(), color_count_limit),
        void,
        std::format("Failed to build color index map for tileset '{}'.", tileset_.name()));

    PT_UNWRAP_TILESET_CONFIG_REF(config_, packing_strategy, tileset_.name(), void);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, packing_strategy_params, tileset_.name(), void);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tile_sharing_packing, tileset_.name(), void);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tile_sharing_alignment, tileset_.name(), void);
    auto strategy = make_packing_strategy(packing_strategy.value(), packing_strategy_params.value(), diag_);
    PalettePacker pal_packer{strategy.get(), &format_, &diag_, &tile_printer_, &pal_printer_};
    std::bitset<pal::num_pals> available_pals{0};
    if (is_secondary()) {
        for (std::size_t i = num_pals_in_primary_; i < num_pals_total_; i++) {
            available_pals.set(i, true);
        }
    }
    else {
        for (std::size_t i = 0; i < num_pals_in_primary_; i++) {
            // TODO: support out-of-band primary palettes - see "Primary Palette Fixing" in topic_staging_area.md
            available_pals.set(i, true);
        }
    }
    PackingParams packing_params{};
    packing_params.tiles_ = porytiles_pixel_rgba_;
    packing_params.anims_ = tileset_.porytiles_component().anims();
    packing_params.color_map_ = color_index_map;
    packing_params.extrinsic_transparency_ = extrinsic_transparency_.value();
    if (is_secondary()) {
        // Lock primary palettes from the compiled paired primary
        std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled{};
        for (std::size_t i = 0; i < num_pals_in_primary_.value(); ++i) {
            prefilled.at(i) = paired_primary_->porymap_component().pal_at(i);
        }
        // Carry over secondary Porytiles pal overrides (slots >= num_pals_in_primary)
        for (std::size_t i = num_pals_in_primary_.value(); i < pal::num_pals; ++i) {
            if (tileset_.porytiles_component().pal_at(i).has_value()) {
                prefilled.at(i) = tileset_.porytiles_component().pal_at(i).value();
            }
        }
        packing_params.prefilled_pals_ = prefilled;
    }
    else {
        packing_params.prefilled_pals_ = tileset_.porytiles_component().pals();
    }
    packing_params.hints_ = pal_hints_.value();
    packing_params.available_pals_ = available_pals;
    packing_params.tile_sharing_packing_ = tile_sharing_packing;
    packing_params.tile_sharing_alignment_ = tile_sharing_alignment;

    PT_TRY_ASSIGN_CHAIN_ERR(
        pal_packing,
        pal_packer.pack_tiles(packing_params),
        void,
        format_.format("Failed to pack palettes for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}));

    tile_to_pal_ = std::move(pal_packing.tile_to_pal_);

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

    // Add Porytiles palettes (for secondary, iterate over secondary palette slots)
    const std::size_t pal_start = is_secondary() ? num_pals_in_primary_.value() : 0;
    const std::size_t pal_end = is_secondary() ? num_pals_total_.value() : num_pals_in_primary_.value();
    for (std::size_t pal_index = pal_start; pal_index < pal_end; ++pal_index) {
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

    /*
     * For secondary compilation, add primary palette colors to the map. The packer needs these to build ColorSets for
     * locked primary palettes, enabling secondary tiles that only use primary colors to be correctly assigned to a
     * primary palette. These colors don't count against the secondary color budget, so they're added after the limit
     * check.
     */
    if (is_secondary()) {
        for (std::size_t i = 0; i < num_pals_in_primary_.value(); ++i) {
            const auto &primary_pal = paired_primary_->porymap_component().pal_at(i);
            color_index_map.add_pal(primary_pal, extrinsic_transparency_.value());
        }
    }

    return color_index_map;
}

ChainableResult<AnimKeyframeData>
CompilerTask::pipeline_helper_build_keyframe_data(const std::string &anim_name, const Animation<Rgba32> &anim) const
{
    const AnimFrame<Rgba32> &composite_frame = anim.composite_frame(extrinsic_transparency_);
    const std::size_t tile_count = composite_frame.tiles().size();

    AnimKeyframeData result;
    result.tiles.reserve(tile_count);
    result.palettes.reserve(tile_count);

    /*
     * For automatic/hybrid mode, we use the key frame tiles. For manual mode (no key frame),
     * we use the first regular frame's tiles as the representative tiles to place in tiles.png.
     *
     * TODO: frames() is a std::map<std::string, ...>, so begin() yields the lexicographically first key. This works
     * for single-digit frame names ("0", "1", ...) but would break for 10+ frames ("10" sorts before "2"). Consider
     * using params().frame_names()[0] to look up the intended first frame instead.
     */
    const AnimFrame<Rgba32> &representative_frame =
        anim.has_key_frame() ? anim.key_frame() : anim.frames().begin()->second;

    for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
        const PixelTile<Rgba32> &composite_rgba_tile = composite_frame.tile_at(tile_idx);
        const PixelTile<Rgba32> &representative_tile = representative_frame.tile_at(tile_idx);

        /*
         * Transparent representative tiles are valid for animations without a key frame. They just produce a
         * transparent IndexPixel tile with palette index 0. For key frame animations, validate_anim_frames() catches
         * transparent tiles before we get here.
         */
        if (representative_tile.is_transparent(extrinsic_transparency_.value())) {
            PixelTile<IndexPixel> transparent_tile{IndexPixel{0}};
            result.tiles.emplace_back(transparent_tile);
            result.pal_indices.push_back(0);
            result.palettes.push_back(&new_porymap_pals_.at(0));
            continue;
        }

        /*
         * Match tile to palette using composite frame to guarantee correct palette selection. As we have seen, some
         * animations, like FireRed General's water_current_landwatersedge, have animated tiles that different palettes
         * in different tilemap entries. Here, we're only selecting the first matching pal. It will be up to the user to
         * ensure that the other pals are aligned such that the IndexTile we generate from this step will work for every
         * palette the animation uses.
         *
         * Eventually, when we support tileset.tiles.sharing configuration, we might want to make this approach more
         * sophisticated.
         */
        std::vector<PaletteMatchResult<Rgba32>> matches =
            match_or_best(composite_rgba_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

        if (!matches.at(0).is_covered) {
            std::vector<std::string> err_lines;
            std::vector<std::vector<FormatParam>> err_params;

            // Header line
            err_lines.emplace_back("Animation '{}' composite subtile '{}': no matching palette found.");
            err_params.push_back({FormatParam{anim_name, Style::bold}, FormatParam{tile_idx, Style::bold}});

            // Closest N match(es) with covered/missing colors
            err_lines.emplace_back();
            err_params.emplace_back();
            err_lines.emplace_back("Closest N match(es) with covered colors highlighted:");
            err_lines.emplace_back();
            err_params.emplace_back();
            err_params.emplace_back();
            int match_idx = 0;
            for (const auto &match : matches) {
                if (match_idx != 0) {
                    err_lines.emplace_back();
                    err_params.emplace_back();
                }
                err_lines.emplace_back("Palette match candidate: {}");
                err_params.push_back({FormatParam{pal_filename(match.pal_index), Style::bold}});
                for (const auto &line : pal_printer_.print_rgba_palette_covered_missing(
                         new_porymap_pals_.at(match.pal_index), match.covered_colors, match.missing_colors)) {
                    err_lines.push_back(line);
                    err_params.emplace_back();
                }
                match_idx++;
            }

            return FormattableError{std::move(err_lines), std::move(err_params)};
        }

        // Convert key frame tile to IndexPixel using matched palette
        const std::size_t pal_index = matches.at(0).pal_index;
        const auto &matched_pal = new_porymap_pals_.at(pal_index);
        const PixelTile<IndexPixel> indexed_key_frame_tile =
            index_tile_from_color_tile(representative_tile, matched_pal, extrinsic_transparency_.value());

        result.tiles.emplace_back(indexed_key_frame_tile);
        result.pal_indices.push_back(pal_index);
        // We'll only actually use this vector in patch mode, but compute anyway to simplify code paths
        result.palettes.push_back(&matched_pal);
    }

    return result;
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

    if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
        std::size_t total_keyframe_tiles = 0;
        for (const auto &anim : anims | std::views::values) {
            if (anim.has_key_frame()) {
                total_keyframe_tiles += anim.key_frame().tiles().size();
            }
            else if (anim.has_frames()) {
                // TODO: same lexicographic ordering caveat as in pipeline_helper_build_keyframe_data
                total_keyframe_tiles += anim.frames().begin()->second.tiles().size();
            }
        }
        const std::size_t anim_start = is_secondary() ? (num_tiles_in_primary_.value() + 1) : 1;
        tiles_workspace_->reserve_anim_slots(total_keyframe_tiles, anim_start);
    }

    std::map<std::string, std::size_t> anim_offsets;
    std::map<std::string, std::vector<std::size_t>> anim_pal_indices;
    std::size_t current_offset = tiles_workspace_->anim_start_offset();

    const auto &per_anim_overrides = per_anim_overrides_.value();

    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_frames()) {
            panic("anim '" + anim_name + "' has no frames");
        }

        // Build keyframe data (common to all modes, needed for pal_indices even if we skip tile placement)
        PT_TRY_ASSIGN_PASS_ERR(keyframe_data, pipeline_helper_build_keyframe_data(anim_name, anim), void);

        const std::size_t tile_count = keyframe_data.tiles.size();
        anim_pal_indices[anim_name] = keyframe_data.pal_indices;
        std::size_t offset{};

        // Resolve effective FrameLinking for this animation
        const ConfigValue<FrameLinking> effective_linking =
            (per_anim_overrides.contains(anim_name) && per_anim_overrides.at(anim_name).linking.has_value())
                ? per_anim_overrides_.derive(per_anim_overrides.at(anim_name).linking)
                : global_frame_linking_;

        if (effective_linking == FrameLinking::manual && tiles_edit_mode_ != ArtifactEditMode::optimize) {
            /*
             * Manual frame linking in patch/locked mode: use the tile_offset from anim.json directly.
             * Don't search tiles.png. The keyframes may not be findable via color matching. Whatever
             * is already at that offset in tiles.png will be dynamically overwritten by the game's
             * animation DMA code at runtime anyway.
             */
            const std::size_t json_offset = anim.params().tile_offset();
            if (json_offset == 0) {
                return FormattableError{
                    "Animation '{}' uses manual frame linking in '{}' mode but has no tile_offset in anim.json.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{to_string(tiles_edit_mode_.value()), Style::bold}};
            }
            if (json_offset + tile_count > tiles_workspace_->capacity()) {
                return FormattableError{
                    "Animation '{}' tile_offset '{}' + tile_count '{}' exceeds tiles.png capacity '{}'.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{json_offset, Style::bold},
                    FormatParam{tile_count, Style::bold},
                    FormatParam{tiles_workspace_->capacity(), Style::bold}};
            }
            offset = json_offset;
        }
        else {
            // Automatic mode (all edit modes) OR manual mode with optimize
            if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
                offset = current_offset;
                for (std::size_t i = 0; i < tile_count; ++i) {
                    const std::size_t reserved_index = current_offset - tiles_workspace_->anim_start_offset();
                    tiles_workspace_->place_anim_tile(reserved_index, keyframe_data.tiles[i]);
                    ++current_offset;
                }
            }
            else if (tiles_edit_mode_ == ArtifactEditMode::patch) {
                // Try to find existing contiguous keyframe sequence using color-equivalence comparison
                if (const auto existing_offset = tiles_workspace_->find_existing_contiguous_tiles_by_color(
                        keyframe_data.tiles, keyframe_data.palettes);
                    existing_offset.has_value()) {
                    offset = existing_offset.value();
                }
                // If full sequence not found, find sufficient contiguous free space to insert
                else if (
                    const auto free_offset = tiles_workspace_->find_contiguous_transparent_slots(tile_count);
                    free_offset.has_value()) {
                    tiles_workspace_->place_tiles_at(free_offset.value(), keyframe_data.tiles);
                    offset = free_offset.value();
                }
                else {
                    /*
                     * TODO: This condition branching doesn't handle the possibility that a partial contiguous sequence
                     * exists, with sufficient free space after to complete the insertion. The match is all-or-nothing.
                     * Either the full tile sequence must already exist, or there must be contiguous free space large
                     * enough to insert it. Future versions of Porytiles may want to handle this case more cleanly,
                     * either by successfully "completing" a partial key frame sequence, or at least notifying the user
                     * that this special edge case was hit.
                     */
                    return FormattableError{
                        "Animation '{}' requires {} contiguous tiles but no sufficient space found.",
                        FormatParam{anim_name, Style::bold},
                        FormatParam{tile_count, Style::bold}};
                }
            }
            else if (tiles_edit_mode_ == ArtifactEditMode::locked) {
                // In locked mode, keyframes must already exist contiguously
                // Use color-equivalence comparison to handle duplicate palette colors (same fix as patch mode)
                const auto existing_offset = tiles_workspace_->find_existing_contiguous_tiles_by_color(
                    keyframe_data.tiles, keyframe_data.palettes);
                if (existing_offset.has_value()) {
                    offset = existing_offset.value();
                }
                else {
                    // TODO: could we improve this error by showing violating tiles?
                    std::vector<std::string> err_msg{};
                    err_msg.emplace_back(format_.format(
                        "Animation '{}' keyframes not found in existing tiles.png.",
                        FormatParam{anim_name, Style::bold}));
                    err_msg.emplace_back(format_.format(
                        "Cannot proceed due to '{}' setting '{}'.",
                        FormatParam{"Tiles Edit Mode", Style::bold},
                        FormatParam{"locked", Style::bold}));
                    std::ranges::copy(
                        format_config_note_with_separator(format_, tiles_edit_mode_), std::back_inserter(err_msg));
                    return FormattableError{err_msg};
                }
            }
            else {
                panic("unexpected tiles_edit_mode");
            }
        }

        anim_offsets[anim_name] = offset;
    }

    for (const auto &[anim_name, anim] : anims) {
        anim_tile_matcher_.register_animation(
            anim_name, anim, anim_offsets.at(anim_name), extrinsic_transparency_, anim_pal_indices.at(anim_name));
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
        const AnimFrame<Rgba32> composite = source_anim.composite_frame(extrinsic_transparency_.value());
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
            // TODO: could we display something more here?
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(format_.format(
                "Animation '{}' uses multiple palettes across subtiles.", FormatParam{anim_name, Style::bold}));
            warning_lines.emplace_back(format_.format(
                "Porymap-component frame PNGs will be saved using palette '{}' for display purposes.",
                FormatParam{pal_filename(frame_pal_index), Style::bold}));
            diag_.warning("multi-palette-animation", warning_lines);
        }

        // Build a dynamic palette for embedding in the AnimFrame
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

            AnimFrame frame{frame_name, std::move(frame_index_tiles)};
            frame.palette(anim_palette);
            compiled_anim.put_frame(frame_name, std::move(frame));
        }

        // 6. Set params with updated tile_offset/tile_count
        AnimParams params = source_anim.params();
        params.tile_offset(tile_offset);
        params.tile_count(tile_count);
        compiled_anim.params(std::move(params));

        // 7. Add to output component (key_frame left as std::nullopt)
        new_porymap_component_->add_anim(std::move(compiled_anim));
    }
}

void CompilerTask::pipeline_helper_apply_manual_overrides()
{
    const auto &source_anims = tileset_.porytiles_component().anims();
    if (source_anims.empty()) {
        return;
    }

    const auto &per_anim_overrides = per_anim_overrides_.value();

    for (const auto &[anim_name, source_anim] : source_anims) {
        // Resolve effective FrameLinking for this animation
        const ConfigValue<FrameLinking> effective_linking =
            (per_anim_overrides.contains(anim_name) && per_anim_overrides.at(anim_name).linking.has_value())
                ? per_anim_overrides_.derive(per_anim_overrides.at(anim_name).linking)
                : global_frame_linking_;

        const auto &overrides = source_anim.params().overrides();

        switch (effective_linking) {
        case FrameLinking::automatic: {
            if (!overrides.empty()) {
                std::vector<std::string> warning_lines;
                warning_lines.emplace_back(format_.format(
                    "Animation '{}' has frame_linking 'automatic' but overrides are present in anim.json.",
                    FormatParam{anim_name, Style::bold}));
                warning_lines.emplace_back("The overrides will be ignored.");
                diag_.warning("automatic-mode-overrides-ignored", warning_lines);
            }
            break;
        }

        case FrameLinking::manual: {
            if (overrides.empty()) {
                std::vector<std::string> warning_lines;
                warning_lines.emplace_back(format_.format(
                    "Animation '{}' has frame_linking '{}' but no overrides are present in anim.json.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{"manual", Style::bold}));
                warning_lines.emplace_back("Animation tiles will not be linked to any metatiles.");
                diag_.warning("manual-no-overrides", warning_lines);
                break;
            }

            // Get the tile_offset for this animation from the matcher
            auto maybe_tile_offset = anim_tile_matcher_.tile_offset_for(anim_name);
            if (!maybe_tile_offset.has_value()) {
                panic("animation '" + anim_name + "' not registered in anim_tile_matcher_");
            }
            const std::size_t tile_offset = maybe_tile_offset.value();

            // Apply each override entry to metatiles_bin
            auto &metatiles_bin = new_porymap_component_->metatiles_bin();
            for (const auto &entry : overrides) {
                const std::size_t bin_index =
                    entry.metatile_id * metatile::entries_per_metatile_triple +
                    static_cast<std::size_t>(entry.layer) * metatile::tiles_per_metatile_layer +
                    static_cast<std::size_t>(entry.subtile);

                if (bin_index >= metatiles_bin.size()) {
                    panic(
                        "animation '" + anim_name + "' override references metatile_id " +
                        std::to_string(entry.metatile_id) + " which is out of range");
                }

                const std::size_t absolute_tile = tile_offset + entry.frame_subtile;
                metatiles_bin[bin_index] = TilemapEntry{absolute_tile, entry.pal_index, entry.h_flip, entry.v_flip};
            }
            break;
        }

        case FrameLinking::hybrid:
            panic("TODO: implement hybrid frame linking");

        default:
            panic("unhandled value for FrameLinking");
        }
    }
}

void CompilerTask::pipeline_helper_apply_true_color_to_tiles_png()
{
    // Phase 1: Build tile_index -> first_pal_index map from tilemap entries
    std::unordered_map<std::size_t, std::size_t> tile_to_first_pal;
    std::unordered_map<std::size_t, std::set<std::size_t>> tile_to_all_pals;

    for (const auto &entry : new_porymap_component_->metatiles_bin()) {
        const auto tile_idx = entry.tile_index();
        const auto pal_idx = entry.pal_index();

        if (tile_idx == 0) {
            continue; // Skip transparent tile
        }

        tile_to_all_pals[tile_idx].insert(pal_idx);

        if (!tile_to_first_pal.contains(tile_idx)) {
            tile_to_first_pal[tile_idx] = pal_idx;
        }
    }

    // Phase 2: Handle animation-only tiles (not in metatiles_bin)
    for (const auto &[anim_name, source_anim] : tileset_.porytiles_component().anims()) {
        auto maybe_tile_offset = anim_tile_matcher_.tile_offset_for(anim_name);
        if (!maybe_tile_offset.has_value()) {
            continue;
        }

        const std::size_t tile_offset = maybe_tile_offset.value();
        const AnimFrame<Rgba32> composite = source_anim.composite_frame(extrinsic_transparency_.value());
        const std::size_t tile_count = composite.tile_count();

        for (std::size_t subtile_idx = 0; subtile_idx < tile_count; ++subtile_idx) {
            const std::size_t absolute_tile_idx = tile_offset + subtile_idx;

            if (tile_to_first_pal.contains(absolute_tile_idx)) {
                continue; // Already mapped from metatiles_bin
            }

            const PixelTile<Rgba32> &composite_tile = composite.tile_at(subtile_idx);
            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_tile, new_porymap_pals_, extrinsic_transparency_.value(), 1);

            if (matches.at(0).is_covered) {
                const std::size_t matched_pal_idx = matches.at(0).pal_index;
                tile_to_first_pal[absolute_tile_idx] = matched_pal_idx;

                // Extract the tile to check for transparency and for visualization
                const auto &tiles_img = new_porymap_component_->tiles_png();
                const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_img, absolute_tile_idx);

                // Skip remark for transparent tiles (unused slots)
                if (index_tile.is_transparent()) {
                    continue;
                }

                // Emit remark for animation-only tiles not referenced in metatiles
                constexpr auto tag = "true-color-anim-only-tile";
                std::vector<std::string> remark_lines;
                remark_lines.emplace_back(format_.format(
                    "tile index '{}' (animation '{}', subtile '{}') is not referenced in metatiles",
                    FormatParam{absolute_tile_idx, Style::bold},
                    FormatParam{anim_name, Style::bold},
                    FormatParam{subtile_idx, Style::bold}));
                remark_lines.emplace_back(format_.format(
                    "Using '{}' for true-color encoding (determined via palette matching).",
                    FormatParam{pal_filename(matched_pal_idx), Style::bold}));

                // Visualize the tile using the matched palette
                const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                    index_tile, new_porymap_pals_.at(matched_pal_idx), extrinsic_transparency_.value());
                remark_lines.emplace_back();
                std::ranges::copy(
                    tile_printer_.print_tile(rgba_tile, extrinsic_transparency_.value()),
                    std::back_inserter(remark_lines));

                diag_.remark(tag, remark_lines);
            }
        }
    }

    // Phase 3: Emit diagnostic remark for tiles used with multiple palettes
    for (const auto &[tile_idx, pals] : tile_to_all_pals) {
        if (pals.size() > 1) {
            // Extract the tile to check for transparency and for visualization
            const auto &tiles_img = new_porymap_component_->tiles_png();
            const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_img, tile_idx);

            // Skip remark for transparent tiles (unused slots)
            if (index_tile.is_transparent()) {
                continue;
            }

            constexpr auto tag = "true-color-multi-palette-tile";
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(
                format_.format("tile index '{}' is used with multiple palettes", FormatParam{tile_idx, Style::bold}));

            std::string pal_list;
            for (const auto pal : pals) {
                if (!pal_list.empty()) {
                    pal_list += ", ";
                }
                pal_list += pal_filename(pal);
            }

            const std::size_t selected_pal_idx = tile_to_first_pal.at(tile_idx);
            remark_lines.emplace_back(format_.format(
                "Palettes used: {}; tiles.png will display using '{}'.",
                FormatParam{pal_list},
                FormatParam{pal_filename(selected_pal_idx), Style::bold}));

            // Visualize the tile under each palette resolution
            for (const auto pal_idx : pals) {
                remark_lines.emplace_back();
                remark_lines.emplace_back(
                    format_.format("{} resolution:", FormatParam{pal_filename(pal_idx), Style::bold}));
                const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                    index_tile, new_porymap_pals_.at(pal_idx), extrinsic_transparency_.value());
                std::ranges::copy(
                    tile_printer_.print_tile(rgba_tile, extrinsic_transparency_.value()),
                    std::back_inserter(remark_lines));
            }

            diag_.remark(tag, remark_lines);
        }
    }

    // Phase 4: Transform tiles_png pixels
    Image<IndexPixel> tiles_img = new_porymap_component_->tiles_png();
    constexpr std::size_t tiles_per_row = metatile::metatiles_per_row * metatile::tiles_per_side;

    const std::size_t total_tiles = tiles_img.size_in_tiles();

    for (std::size_t tile_idx = 1; tile_idx < total_tiles; ++tile_idx) {
        if (!tile_to_first_pal.contains(tile_idx)) {
            // Extract the tile to check for transparency
            const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_img, tile_idx, tiles_per_row);

            // Skip warning for transparent tiles (unused slots) - user already knows they're unused
            if (index_tile.is_transparent()) {
                continue;
            }

            // Emit warning for unreferenced non-transparent tiles
            constexpr auto tag = "true-color-unreferenced-tile";
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(format_.format(
                "tile index '{}' is not referenced in metatiles or animations", FormatParam{tile_idx, Style::bold}));
            diag_.warning(tag, warning_lines);

            std::vector<std::string> note_lines;
            note_lines.emplace_back("This tile may be used by a secondary tileset, or it may be completely unused.");
            note_lines.emplace_back(format_.format(
                "Displaying using '{}' for color resolution.", FormatParam{pal_filename(0), Style::bold}));

            // Visualize the tile using palette 0
            const PixelTile<Rgba32> rgba_tile =
                color_tile_from_index_tile(index_tile, new_porymap_pals_.at(0), extrinsic_transparency_.value());
            note_lines.emplace_back();
            std::ranges::copy(
                tile_printer_.print_tile(rgba_tile, extrinsic_transparency_.value()), std::back_inserter(note_lines));

            diag_.warning_note(tag, note_lines);
            continue; // Skip unreferenced tiles (no palette encoding needed)
        }

        const std::size_t pal_idx = tile_to_first_pal.at(tile_idx);
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;
        const std::size_t pixel_row_start = tile_row * tile::side_length_pix;
        const std::size_t pixel_col_start = tile_col * tile::side_length_pix;

        for (std::size_t py = 0; py < tile::side_length_pix; ++py) {
            for (std::size_t px = 0; px < tile::side_length_pix; ++px) {
                const std::size_t row = pixel_row_start + py;
                const std::size_t col = pixel_col_start + px;
                const IndexPixel old_pixel = tiles_img.at(row, col);
                const std::size_t color_idx = old_pixel.color_index();
                const std::size_t new_index = (pal_idx << 4) | color_idx;
                tiles_img.set(row, col, IndexPixel{new_index});
            }
        }
    }

    // Phase 5: Build the 8-bit palette for the PNG (num_pals_in_primary * 16 colors)
    std::vector<Rgba32> true_color_palette;
    true_color_palette.reserve(num_pals_in_primary_.value() * pal::max_size);

    for (std::size_t pal_idx = 0; pal_idx < num_pals_in_primary_.value(); ++pal_idx) {
        const auto &pal = new_porymap_pals_.at(pal_idx);
        for (std::size_t color_idx = 0; color_idx < pal::max_size; ++color_idx) {
            true_color_palette.push_back(pal.at(color_idx));
        }
    }

    tiles_img.palette(std::move(true_color_palette));
    new_porymap_component_->tiles_png(tiles_img);
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
    std::ranges::copy(pal_printer_.print_rgba_pal(matched_pal), std::back_inserter(pal_note));
    diag_.error_note(tag, pal_note);

    // Print note showing the generated IndexPixel tile
    std::vector<std::string> tile_note{};
    tile_note.emplace_back("generated index tile:");
    std::ranges::copy(
        tile_printer_.print_tile(index_tile, extrinsic_transparency_.value()), std::back_inserter(tile_note));
    diag_.error_note(tag, tile_note);
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
    diag_.error_note(tag, closest_n_note);
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
    note_text.push_back(format_.format(
        "tile limit is '{}' due to configuration", FormatParam{num_tiles_in_primary_.value(), Style::bold}));
    note_text.emplace_back();
    std::ranges::copy(num_tiles_in_primary_.prettify(format_), std::back_inserter(note_text));
    diag_.error_note(tag, note_text);
}

} // namespace

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>>
TilesetCompiler::compile(const Tileset &tileset, const Tileset *paired_primary) const
{
    CompilerTask task{tileset, paired_primary, *format_, *diag_, *tile_printer_, *pal_printer_, *config_};
    return task.run();
}

} // namespace porytiles2
