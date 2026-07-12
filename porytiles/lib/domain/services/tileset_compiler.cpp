#include "porytiles/domain/services/tileset_compiler.hpp"

#include <array>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "porytiles/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles/domain/algorithms/palette_matchers.hpp"
#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/algorithms/tile_extractors.hpp"
#include "porytiles/domain/algorithms/tileset_compile_validators.hpp"
#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/config/frame_linking.hpp"
#include "porytiles/domain/config/packing_strategy_params.hpp"
#include "porytiles/domain/config/packing_strategy_type.hpp"
#include "porytiles/domain/config/per_anim_overrides.hpp"
#include "porytiles/domain/config/tiles_palette_mode.hpp"
#include "porytiles/domain/models/anim_override_entry.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/color_index_map.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tiles_png_workspace.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/packing/services/backtracking_strategy.hpp"
#include "porytiles/domain/packing/services/best_fusion_strategy.hpp"
#include "porytiles/domain/packing/services/overload_and_remove_strategy.hpp"
#include "porytiles/domain/packing/services/palette_packer.hpp"
#include "porytiles/domain/services/anim_tile_matcher.hpp"
#include "porytiles/domain/services/image_tileizer.hpp"
#include "porytiles/domain/services/layer_image_metatileizer.hpp"
#include "porytiles/domain/services/layer_mode_converter.hpp"
#include "porytiles/domain/services/metatile_decompiler.hpp"
#include "porytiles/utilities/functional/transform.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles;

/// @brief Creates a packing strategy instance based on config settings.
///
/// @details
/// If the selected strategy's parameter block has any values set, constructs the strategy in single-config mode with
/// the provided parameters (unset fields fall back to their per-field defaults). Otherwise, constructs the strategy in
/// preset matrix mode. BestFusionStrategy has no parameters and always uses its parameterless constructor.
///
/// @param strategy_type The selected packing algorithm
/// @param params Per-strategy parameter blocks
/// @param diag Diagnostics interface for remarks about successful search parameters
/// @return A unique_ptr to the configured PackingStrategy
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

/// @brief Result type for tile assignment operations during compilation.
///
/// @details
/// Encapsulates the outcome of attempting to assign a tile via palette matching. Contains all information needed for
/// error reporting in failure cases.
struct TileAssignmentResult {
    enum class Status { success, no_covering_palette, tile_not_found, tile_limit_reached };

    Status status{Status::success};
    std::optional<TilemapEntry> entry{};

    // Error reporting data (populated on failure)
    std::vector<PaletteMatchResult<Rgba32>> match_results{};
    PixelTile<IndexPixel> index_tile{};
    std::size_t palette_index{0};
    Palette<Rgba32, palette::max_size> matched_palette{};
};

/// @brief Data structure holding processed keyframe tiles and their matched palettes.
///
/// @details
/// Used to pass the results of building keyframe data from the common helper to the mode-specific placement logic. The
/// palettes vector is only used in patch mode for color-equivalence matching; optimize and locked modes ignore it.
struct AnimKeyframeData {
    std::vector<CanonicalPixelTile<IndexPixel>> tiles;
    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    std::vector<std::size_t> palette_indices;
};

/// @brief Per-path text used to tag and phrase override-validation diagnostics.
///
/// @details
/// The two override-application paths (manual frame linking and primary_references) share identical validation logic
/// but differ in diagnostic tag prefix and message subject. This bundles those two per-path strings so the shared
/// validator can compose path-appropriate diagnostics.
struct OverridePathInfo {
    /// Prefix for diagnostic tags, e.g. "manual" or "primary-references".
    std::string tag_prefix;
    /// Format string (one "{}" placeholder for the animation name) naming the override subject in messages.
    std::string subject_template;
};

/// @brief Validates a single anim.json override entry before it is written to metatiles_bin.
///
/// @details
/// Both override-application paths route user-authored override entries through should_apply, which performs bounds and
/// encodability checks and emits graceful diagnostics (never a panic) on bad input. This consolidates checks that
/// previously diverged between the two paths: the manual path panicked on an out-of-range metatile_id and skipped the
/// frame_subtile bound entirely, while neither path checked palette_index or warned about entries destined for a
/// dual-layerization-dropped layer.
class OverrideEntryValidator {
  public:
    OverrideEntryValidator(
        const TextFormatter &format,
        const UserDiagnostics &diag,
        const ConfigValue<std::size_t> &num_palettes_total,
        LayerMode configured_layer_mode,
        const std::vector<Metatile<Rgba32>> &source_metatiles,
        Rgba32 extrinsic_transparency,
        const std::vector<std::optional<LayerType>> &explicit_layer_types)
        : format_{format}, diag_{diag}, num_palettes_total_{num_palettes_total},
          configured_layer_mode_{configured_layer_mode}, source_metatiles_{source_metatiles},
          extrinsic_transparency_{extrinsic_transparency}, explicit_layer_types_{explicit_layer_types}
    {
    }

    /// @brief Validates one override entry and reports whether it should be written to metatiles_bin.
    ///
    /// @details
    /// Emits an error diagnostic and returns false for unrecoverable problems (frame_subtile out of range, metatile_id
    /// out of range, palette_index unencodable in the 4-bit hardware field). Emits a warning but returns true for a
    /// palette_index that references a configured-but-unmanaged palette slot. Emits a warning and returns false for an
    /// entry targeting a layer that dual-layerization will drop.
    ///
    /// @param path Per-path diagnostic tag prefix and message subject
    /// @param anim_name The animation name, substituted into the subject template
    /// @param entry The override entry to validate
    /// @param tile_count The number of tiles in the referenced animation (the frame_subtile upper bound)
    /// @return True if the entry passed validation and should be applied, false otherwise
    [[nodiscard]] bool should_apply(
        const OverridePathInfo &path,
        const std::string &anim_name,
        const AnimOverrideEntry &entry,
        std::size_t tile_count) const;

  private:
    const TextFormatter &format_;
    const UserDiagnostics &diag_;
    const ConfigValue<std::size_t> &num_palettes_total_;
    LayerMode configured_layer_mode_;
    const std::vector<Metatile<Rgba32>> &source_metatiles_;
    Rgba32 extrinsic_transparency_;
    // Per-metatile explicit layer-type overrides (indexed by metatile_id, nullopt when unset). Must match the vector
    // dual_layerize receives so the dropped-layer check here agrees with what conversion actually drops.
    const std::vector<std::optional<LayerType>> &explicit_layer_types_;
};

bool OverrideEntryValidator::should_apply(
    const OverridePathInfo &path,
    const std::string &anim_name,
    const AnimOverrideEntry &entry,
    std::size_t tile_count) const
{
    const std::string subject = format_.format(path.subject_template, FormatParam{anim_name, Style::bold});

    // 1. frame_subtile must index a real tile in the animation.
    if (entry.frame_subtile >= tile_count) {
        std::vector<std::string> lines;
        lines.push_back(
            subject + format_.format(
                          " override has frame_subtile {} but the animation only has {} tiles.",
                          FormatParam{entry.frame_subtile},
                          FormatParam{tile_count}));
        diag_.error(path.tag_prefix + "-frame-subtile-oob", lines);
        return false;
    }

    // 2. metatile_id must be in range. This precedes the .at() in check 5.
    if (entry.metatile_id >= source_metatiles_.size()) {
        std::vector<std::string> lines;
        lines.push_back(
            subject +
            format_.format(
                " override references metatile_id {} which is out of range.", FormatParam{entry.metatile_id}));
        lines.push_back(format_.format("This tileset has {} metatiles.", FormatParam{source_metatiles_.size()}));
        diag_.error(path.tag_prefix + "-metatile-oob", lines);
        return false;
    }

    // 3. palette_index must fit the 4-bit GBA palette field.
    if (entry.palette_index >= palette::num_palettes) {
        std::vector<std::string> lines;
        lines.push_back(
            subject + format_.format(
                          " override has palette_index {} but the maximum palette index is {}.",
                          FormatParam{entry.palette_index},
                          FormatParam{palette::num_palettes - 1}));
        lines.push_back(format_.format(
            "The GBA hardware only supports {} background palettes.", FormatParam{palette::num_palettes}));
        diag_.error(path.tag_prefix + "-pal-index-oob", lines);
        return false;
    }

    // 4. palette_index is encodable but points past the configured palette count: warn, but still apply.
    if (entry.palette_index >= num_palettes_total_.value()) {
        std::vector<std::string> lines;
        lines.push_back(
            subject + format_.format(
                          " override has palette_index {} but only {} palettes are configured.",
                          FormatParam{entry.palette_index},
                          FormatParam{num_palettes_total_.value()}));
        lines.emplace_back(
            "Porytiles does not manage palettes beyond the configured count, so this override will render with "
            "whatever colors occupy that slot.");
        lines.append_range(format_config_note_with_separator(format_, num_palettes_total_));
        diag_.warning(path.tag_prefix + "-pal-index-unused", lines);
    }

    // 5. In dual-layer mode, an entry targeting the dropped layer would silently vanish. Use the same effective layer
    // type dual_layerize uses: an explicit override wins over inference, so a manual override targeting a layer that a
    // covered/split override keeps must not be rejected on the strength of an inferred 'normal'.
    if (configured_layer_mode_ == LayerMode::dual) {
        const LayerType inferred = source_metatiles_.at(entry.metatile_id).infer_layer_type(extrinsic_transparency_);
        const bool explicit_set =
            entry.metatile_id < explicit_layer_types_.size() && explicit_layer_types_[entry.metatile_id].has_value();
        const LayerType effective = explicit_set ? explicit_layer_types_[entry.metatile_id].value() : inferred;
        if (metatile::dropped_layer_for(effective) == entry.layer) {
            std::vector<std::string> lines;
            lines.push_back(
                subject + format_.format(
                              " override targets the '{}' layer of metatile {} but dual-layer conversion drops that "
                              "layer ({} layer type '{}').",
                              FormatParam{metatile::to_string(entry.layer)},
                              FormatParam{entry.metatile_id},
                              FormatParam{explicit_set ? std::string{"explicit"} : std::string{"inferred"}},
                              FormatParam{to_string(effective)}));
            lines.emplace_back("The override will be ignored.");
            diag_.warning(path.tag_prefix + "-dual-layer-drop", lines);
            return false;
        }
    }

    return true;
}

/// @brief Task encapsulating the compilation operation for primary tilesets.
///
/// @details
/// Breaks the monolithic compilation logic into discrete phases:
/// - process_porytiles_input() - metatileize, validate, decompose Porytiles layers
/// - process_porymap_input() - triple-layerize, decompile, decompose Porymap data
/// - setup_working_data() - initialize palettes, workspace, and output Porymap component
/// - match_tiles() - main loop matching Porytiles tiles to Porymap tiles/palettes
/// - assemble_output() - finalize output with dual-layer conversion, attributes, exports
class CompilerTask {
  public:
    CompilerTask(
        const Tileset &tileset,
        bool is_secondary,
        const Tileset *paired_primary,
        const TextFormatter &format,
        const UserDiagnostics &diag,
        const TilePrinter &tile_printer,
        const PalettePrinter &palette_printer,
        const DomainConfig &config,
        const Schema &schema)
        : tileset_{tileset}, is_secondary_{is_secondary}, paired_primary_{paired_primary}, format_{format}, diag_{diag},
          tile_printer_{tile_printer}, palette_printer_{palette_printer}, config_{config}, schema_{schema},
          extrinsic_transparency_{}, num_palettes_in_primary_{}, num_palettes_total_{}, num_metatiles_in_primary_{},
          num_tiles_in_primary_{}, num_tiles_per_metatile_{}, palette_hints_enabled_{}, palette_hints_{}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> run();

  private:
    // Pipeline steps
    [[nodiscard]] ChainableResult<void> pipeline_step_process_porytiles_input();
    [[nodiscard]] ChainableResult<void> pipeline_step_process_porymap_input();
    [[nodiscard]] ChainableResult<void> pipeline_step_validate_input();
    [[nodiscard]] ChainableResult<void> pipeline_step_setup_working_data();
    [[nodiscard]] ChainableResult<void> pipeline_step_match_tiles_palettes();
    [[nodiscard]] std::unique_ptr<Tileset> pipeline_step_assemble_output();

    // Pipeline helpers - tile matching
    [[nodiscard]] std::optional<TilemapEntry> pipeline_helper_try_reuse_porymap_tile(std::size_t tile_index);
    [[nodiscard]] TileAssignmentResult
    pipeline_helper_assign_tile_via_palette_match(const PixelTile<Rgba32> &porytiles_tile, std::size_t flat_index);

    // Pipeline helpers - palette packing
    [[nodiscard]] ChainableResult<void> pipeline_helper_run_palette_packing();
    [[nodiscard]] ChainableResult<ColorIndexMap<Rgba32>>
    pipeline_helper_build_color_index_map(const std::vector<PaletteHint> &hints, std::size_t color_count_limit) const;
    // Pipeline helpers - animation processing
    [[nodiscard]] ChainableResult<void> pipeline_helper_register_animations();
    [[nodiscard]] ChainableResult<AnimKeyframeData>
    pipeline_helper_build_keyframe_data(const std::string &anim_name, const Animation<Rgba32> &anim) const;
    [[nodiscard]] ChainableResult<void> pipeline_helper_validate_primary_anim_subtile_coverage() const;
    void pipeline_helper_compile_animations();
    void pipeline_helper_apply_manual_overrides();

    // Builds the per-metatile explicit layer-type override vector (indexed by metatile_id) from the source Porytiles
    // attributes. Shared by manual-override validation and dual-layer conversion so both agree on what each metatile's
    // effective layer type is.
    [[nodiscard]] std::vector<std::optional<LayerType>> gather_explicit_layer_types() const;

    // Pipeline helpers - true_color mode
    void pipeline_helper_apply_true_color_to_tiles_png();

    [[nodiscard]] bool is_secondary() const
    {
        return is_secondary_;
    }

    [[nodiscard]] bool has_paired_primary() const
    {
        return paired_primary_ != nullptr;
    }

    // Pipeline helpers - error emission
    void pipeline_helper_emit_no_matching_tile_error(
        std::size_t tile_index,
        const PixelTile<IndexPixel> &index_tile,
        std::size_t palette_index,
        const Palette<Rgba32, palette::max_size> &matched_palette);
    void pipeline_helper_emit_no_matching_palette_error(
        std::size_t tile_index, const std::vector<PaletteMatchResult<Rgba32>> &matches);
    void pipeline_helper_emit_tile_limit_error(std::size_t tile_index, std::size_t tile_limit);

    // Dependencies (injected in ctor)
    const Tileset &tileset_;
    bool is_secondary_;
    const Tileset *paired_primary_;
    const TextFormatter &format_;
    const UserDiagnostics &diag_;
    const TilePrinter &tile_printer_;
    const PalettePrinter &palette_printer_;
    const DomainConfig &config_;
    const Schema &schema_;

    // Config values (populated in run())
    ConfigValue<Rgba32> extrinsic_transparency_;
    ConfigValue<Rgba32> paired_primary_extrinsic_transparency_{};
    ConfigValue<std::size_t> num_palettes_in_primary_;
    ConfigValue<std::size_t> num_palettes_total_;
    ConfigValue<std::size_t> num_metatiles_in_primary_;
    ConfigValue<std::size_t> num_tiles_in_primary_;
    ConfigValue<std::size_t> num_tiles_total_;
    ConfigValue<std::size_t> num_tiles_per_metatile_;
    ConfigValue<bool> palette_hints_enabled_;
    ConfigValue<std::vector<PaletteHint>> palette_hints_;
    ConfigValue<ArtifactEditMode> tiles_edit_mode_;
    ConfigValue<ArtifactEditMode> palettes_edit_mode_;
    ConfigValue<TilesPaletteMode> tiles_palette_mode_;
    ConfigValue<FrameLinking> global_frame_linking_;
    ConfigValue<PerAnimOverrides> per_anim_overrides_;
    ConfigValue<bool> cross_tileset_anim_linking_;

    // Intermediate state - Porytiles
    std::vector<Metatile<Rgba32>> porytiles_metatiles_{};
    std::vector<PixelTile<Rgba32>> porytiles_pixel_rgba_{};
    std::vector<CanonicalPixelTile<Rgba32>> porytiles_canonical_pixel_rgba_{};

    // Intermediate state - Porymap
    std::vector<TilemapEntry> porymap_tilemap_entries_{};
    std::vector<Metatile<Rgba32>> porymap_metatiles_{};
    std::vector<PixelTile<Rgba32>> porymap_pixel_rgba_{};
    std::vector<CanonicalPixelTile<Rgba32>> porymap_canonical_pixel_rgba_{};
    std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> new_porymap_palettes_{};
    std::map<std::size_t, std::size_t> tile_to_palette_{};

    // Working data
    std::unique_ptr<PorymapTilesetComponent> new_porymap_component_{};
    std::unique_ptr<TilesPngWorkspace> tiles_workspace_{};
    AnimTileMatcher anim_tile_matcher_{};
    std::map<std::string, std::vector<std::size_t>> anim_palette_indices_{};
};

ChainableResult<std::unique_ptr<Tileset>> CompilerTask::run()
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_REF(config_, extrinsic_transparency, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_palettes_in_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_palettes_total, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_metatiles_in_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_in_primary, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_total, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, num_tiles_per_metatile, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, palette_hints_enabled, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, palette_hints, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tiles_edit_mode, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, palettes_edit_mode, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tiles_palette_mode, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, global_frame_linking, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, per_anim_overrides, tileset_.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, cross_tileset_anim_linking, tileset_.name(), std::unique_ptr<Tileset>);

    extrinsic_transparency_ = extrinsic_transparency;
    num_palettes_in_primary_ = num_palettes_in_primary;
    num_palettes_total_ = num_palettes_total;
    num_metatiles_in_primary_ = num_metatiles_in_primary;
    num_tiles_in_primary_ = num_tiles_in_primary;
    num_tiles_total_ = num_tiles_total;
    num_tiles_per_metatile_ = num_tiles_per_metatile;
    palette_hints_enabled_ = palette_hints_enabled;
    palette_hints_ = palette_hints;
    tiles_edit_mode_ = tiles_edit_mode;
    palettes_edit_mode_ = palettes_edit_mode;
    tiles_palette_mode_ = tiles_palette_mode;
    global_frame_linking_ = global_frame_linking;
    per_anim_overrides_ = per_anim_overrides;
    cross_tileset_anim_linking_ = cross_tileset_anim_linking;

    // Resolve the paired primary's ET if applicable. This is needed for cross-tileset animation linking so that
    // primary subtiles are classified as transparent/opaque under the primary's own ET rather than the secondary's.
    // Using each tileset's own ET is what makes the cross-ET comparator on the matcher's lookup map find matches
    // across mismatched-ET inputs.
    if (has_paired_primary()) {
        PT_UNWRAP_TILESET_CONFIG_REF_AS(
            paired_primary_et, config_, extrinsic_transparency, paired_primary_->name(), std::unique_ptr<Tileset>);
        paired_primary_extrinsic_transparency_ = std::move(paired_primary_et);
    }

    // Execute subtasks
    PT_TRY_CALL_PASS_ERR(pipeline_step_process_porytiles_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_process_porymap_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_validate_input(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_setup_working_data(), std::unique_ptr<Tileset>);

    PT_TRY_CALL_PASS_ERR(pipeline_step_match_tiles_palettes(), std::unique_ptr<Tileset>);

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
        format_.format(
            "Failed to metatileize input layer images for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}));
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
        format_.format(
            "Failed to triple-layerize Porymap component for tileset '{}'.",
            FormatParam{tileset_.name(), Style::bold}));
    porymap_tilemap_entries_ = std::move(tilemap_entries);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            porymap_tilemap_entries_,
            tileset_.porymap_component().tiles_png(),
            tileset_.porymap_component().palettes()),
        void,
        format_.format(
            "Failed to decompile Porymap component for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}));
    porymap_metatiles_ = std::move(metatiles);

    // We don't need to run any validation (including size validation) on porymap_metatiles here. We're going to
    // overwrite them anyway. We only need to check the size of the final tilemap entry vector. Patch builds don't need
    // to preserve tilemap entries since those cannot be referenced by other tilesets. We can just write a new entry
    // vector every time.

    // Decompose Porymap metatiles and generate canonical versions
    porymap_pixel_rgba_ = metatile::decompose(porymap_metatiles_);
    porymap_canonical_pixel_rgba_ = transform<CanonicalPixelTile<Rgba32>>(porymap_pixel_rgba_);

    return {};
}

ChainableResult<void> CompilerTask::pipeline_step_validate_input()
{
    TilesetCompileValidatorServices services{config_, diag_, tile_printer_, palette_printer_};

    // Reject mode combinations that this compiler does not support before running any content-based validation. This
    // function is the single source of truth for which compile mode combinations are supported.

    if (is_secondary() && tiles_edit_mode_ != ArtifactEditMode::optimize) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(format_.format(
            "Secondary compilation of tileset '{}' does not yet support tiles edit mode '{}'. For now, only '{}' is "
            "supported for secondary tilesets. Support for '{}' and '{}' is planned for a future update.",
            FormatParam{tileset_.name(), Style::bold},
            FormatParam{to_string(tiles_edit_mode_.value()), Style::bold},
            FormatParam{"optimize", Style::bold},
            FormatParam{"locked", Style::bold},
            FormatParam{"patch", Style::bold}));
        err_msg.append_range(format_config_note_with_separator(format_, tiles_edit_mode_));
        return FormattableError{err_msg};
    }

    if (is_secondary() && palettes_edit_mode_ != ArtifactEditMode::optimize) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(format_.format(
            "Secondary compilation of tileset '{}' does not yet support palettes edit mode '{}'. For now, only '{}' is "
            "supported for secondary tilesets. Support for '{}' and '{}' is planned for a future update.",
            FormatParam{tileset_.name(), Style::bold},
            FormatParam{to_string(palettes_edit_mode_.value()), Style::bold},
            FormatParam{"optimize", Style::bold},
            FormatParam{"locked", Style::bold},
            FormatParam{"patch", Style::bold}));
        err_msg.append_range(format_config_note_with_separator(format_, palettes_edit_mode_));
        return FormattableError{err_msg};
    }

    if (palettes_edit_mode_ == ArtifactEditMode::patch) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(format_.format(
            "Tileset '{}' uses palettes edit mode '{}', which is not yet implemented.",
            FormatParam{tileset_.name(), Style::bold},
            FormatParam{to_string(palettes_edit_mode_.value()), Style::bold}));
        err_msg.append_range(format_config_note_with_separator(format_, palettes_edit_mode_));
        return FormattableError{err_msg};
    }

    if (palettes_edit_mode_ == ArtifactEditMode::optimize && tiles_edit_mode_ == ArtifactEditMode::locked) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(format_.format(
            "Tileset '{}' uses palettes edit mode '{}' with tiles edit mode '{}', which is not a valid combination. "
            "Tiles are fundamentally dependent on palettes, so optimizing palettes while keeping tiles locked is "
            "not coherent.",
            FormatParam{tileset_.name(), Style::bold},
            FormatParam{"optimize", Style::bold},
            FormatParam{"locked", Style::bold}));
        err_msg.append_range(format_config_note(format_, palettes_edit_mode_));
        err_msg.append_range(format_config_note_with_separator(format_, tiles_edit_mode_));
        return FormattableError{err_msg};
    }

    // Run metatile count validation
    PT_TRY_CALL_PASS_ERR(
        validate_metatile_count(services, tileset_.name(), is_secondary(), porytiles_metatiles_), void);

    std::size_t palette_start = is_secondary() ? num_palettes_in_primary_.value() : 0;

    // For secondary compiles, validate the paired primary's Porymap palettes before validating the secondary's own
    // palettes. The paired primary's palettes are loaded directly into the palette packer as pre-filled slots, so if
    // they contain the extrinsic transparency color in a non-slot-0 position the packer will panic. Running
    // validate_porymap_palette here turns that crash into a proper diagnostic scoped to the primary's name. This runs
    // unconditionally since secondary compilation always consumes the primary's Porymap palettes.
    if (is_secondary() && has_paired_primary()) {
        for (std::size_t palette_index = 0; palette_index < num_palettes_in_primary_.value(); ++palette_index) {
            PT_TRY_CALL_PASS_ERR(
                validate_porymap_palette(
                    services,
                    paired_primary_->name(),
                    paired_primary_->porymap_component().palette_at(palette_index),
                    palette_index),
                void);
        }
    }

    if (palettes_edit_mode_ != ArtifactEditMode::optimize) {
        // Validate Porymap palettes if user is asking for palettes:locked or palettes:patch
        for (std::size_t palette_index = palette_start; palette_index < tileset_.porymap_component().palettes().size();
             ++palette_index) {
            PT_TRY_CALL_PASS_ERR(
                validate_porymap_palette(
                    services,
                    tileset_.name(),
                    tileset_.porymap_component().palettes().at(palette_index),
                    palette_index),
                void);
        }
    }

    // Fail fast if secondary tileset defines an override palette in a primary slot
    if (is_secondary()) {
        for (std::size_t palette_index = 0; palette_index < num_palettes_in_primary_.value(); ++palette_index) {
            if (palette_index < tileset_.porytiles_component().palettes().size() &&
                tileset_.porytiles_component().palettes().at(palette_index).has_value()) {
                return FormattableError{
                    "Secondary tileset '{}' defines a Porytiles override palette in primary slot '{}'.",
                    FormatParam{tileset_.name(), Style::bold},
                    FormatParam{palette_filename(palette_index), Style::bold}};
            }
        }
    }

    // Validate Porytiles palettes (skip primary slots for secondary)
    for (std::size_t palette_index = palette_start; palette_index < tileset_.porytiles_component().palettes().size();
         ++palette_index) {
        if (tileset_.porytiles_component().palettes().at(palette_index).has_value()) {
            PT_TRY_CALL_PASS_ERR(
                validate_porytiles_palette(
                    services,
                    tileset_.name(),
                    tileset_.porytiles_component().palettes().at(palette_index).value(),
                    palette_index),
                void);
        }
    }

    // Validate palette hints
    for (const auto &hint : palette_hints_.value()) {
        PT_TRY_CALL_PASS_ERR(validate_palette_hint(services, tileset_.name(), hint), void);
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
            tileset_.porytiles_component().palettes(),
            palette_hints_.value()),
        void);

    // Run precision loss validation
    PT_TRY_CALL_PASS_ERR(
        validate_precision_loss(
            services,
            tileset_.name(),
            porytiles_metatiles_,
            tileset_.porytiles_component().anims(),
            tileset_.porytiles_component().palettes(),
            palette_hints_.value(),
            std::nullopt),
        void);

    // Run animation validation
    PT_TRY_CALL_PASS_ERR(validate_anim_frames(services, tileset_.name(), tileset_.porytiles_component().anims()), void);

    return {};
}

ChainableResult<void> CompilerTask::pipeline_step_setup_working_data()
{
    // Create palettes
    if (palettes_edit_mode_ == ArtifactEditMode::locked) {
        // Collect all palettes from existing Porymap component
        for (std::size_t i = 0; i < palette::num_palettes; i++) {
            new_porymap_palettes_[i] = tileset_.porymap_component().palettes()[i];
        }
    }
    else if (palettes_edit_mode_ == ArtifactEditMode::optimize) {
        PT_TRY_CALL_PASS_ERR(pipeline_helper_run_palette_packing(), void);
    }
    else {
        panic("unexpected palettes ArtifactEditMode");
    }

    // Create tiles workspace
    if (tiles_edit_mode_ == ArtifactEditMode::locked) {
        // When tiles are locked, compute the exact size of tiles.png so we keep it completely unchanged. When we
        // output, we'll also set ExportTrimMode::include_trailing_transparent so that if there was transparency at
        // the end, we don't remove it.
        const auto size_in_tiles = tileset_.porymap_component().tiles_png().size_in_tiles();
        tiles_workspace_ = std::make_unique<TilesPngWorkspace>(tileset_.porymap_component().tiles_png(), size_in_tiles);
    }
    else if (tiles_edit_mode_ == ArtifactEditMode::patch) {
        tiles_workspace_ = std::make_unique<TilesPngWorkspace>(
            tileset_.porymap_component().tiles_png(), num_tiles_in_primary_.value());
    }
    else if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
        if (is_secondary()) {
            if (has_paired_primary()) {
                tiles_workspace_ = std::make_unique<TilesPngWorkspace>(TilesPngWorkspace::for_secondary(
                    paired_primary_->porymap_component().tiles_png(),
                    num_tiles_in_primary_.value(),
                    num_tiles_total_.value()));
            }
            else {
                tiles_workspace_ = std::make_unique<TilesPngWorkspace>(TilesPngWorkspace::for_standalone_secondary(
                    num_tiles_in_primary_.value(), num_tiles_total_.value()));
            }
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

ChainableResult<void> CompilerTask::pipeline_step_match_tiles_palettes()
{
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

        // Transparent tiles always map to tile index 0 (the reserved transparent tile).
        //
        // If tile 0 transparency is a pokeemerald convention, why does this come after the
        // pipeline_helper_try_reuse_porymap_tile step for non-tiles-optimize builds? It's because Porytiles design
        // philosophy prioritizes surgical edits where possible. A user could have other locations in tiles.png marked
        // transparent in addition to tile 0. If one of their metatiles referenced one of these alternate locations, we
        // don't want to create a diff by forcing the metatile reference to change to tile 0. Instead, we'll just
        // respect the idiosyncrasy by calling pipeline_helper_try_reuse_porymap_tile and letting it match there first.
        if (porytiles_tile.is_transparent(extrinsic_transparency_.value())) {
            new_porymap_component_->push_back_tilemap_entry(TilemapEntry{0, 0, false, false});
            continue;
        }

        // Assign via palette matching (shared logic for all modes)
        const auto tile_assignment_result = pipeline_helper_assign_tile_via_palette_match(porytiles_tile, i);

        switch (tile_assignment_result.status) {
        case TileAssignmentResult::Status::success:
            new_porymap_component_->push_back_tilemap_entry(tile_assignment_result.entry.value());
            break;

        case TileAssignmentResult::Status::no_covering_palette:
            if (palettes_edit_mode_ == ArtifactEditMode::optimize) {
                panic(
                    "ArtifactEditMode::optimize but no covering palette found - this should have failed at packing "
                    "step");
            }
            matched_all_tiles = false;
            pipeline_helper_emit_no_matching_palette_error(i, tile_assignment_result.match_results);
            break;

        case TileAssignmentResult::Status::tile_not_found:
            matched_all_tiles = false;
            pipeline_helper_emit_no_matching_tile_error(
                i,
                tile_assignment_result.index_tile,
                tile_assignment_result.palette_index,
                tile_assignment_result.matched_palette);
            break;

        case TileAssignmentResult::Status::tile_limit_reached:
            matched_all_tiles = false;
            {
                const std::size_t user_visible_tile_limit =
                    is_secondary() ? (num_tiles_total_.value() - num_tiles_in_primary_.value())
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

    // Catches unreferenced non-transparent animation subtiles at primary compile time, rather than letting the failure
    // surface from a paired secondary compile with a confusing primary-pointing error. Secondary compiles still keep
    // the defense-in-depth check in pipeline_helper_register_animations.
    if (!is_secondary()) {
        PT_TRY_CALL_PASS_SAME_ERR(pipeline_helper_validate_primary_anim_subtile_coverage());
    }

    return {};
}

std::unique_ptr<Tileset> CompilerTask::pipeline_step_assemble_output()
{
    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>(tileset_.porytiles_component());

    // Update porytiles component animation params with computed tile offsets
    for (auto &[anim_name, anim] : new_porytiles_component->anims()) {
        if (auto maybe_offset = anim_tile_matcher_.tile_offset_for(anim_name); maybe_offset.has_value()) {
            AnimParams updated_params = anim.params();
            const std::size_t local_offset =
                is_secondary() ? maybe_offset.value() - num_tiles_in_primary_.value() : maybe_offset.value();
            updated_params.tile_offset(local_offset);
            anim.params(std::move(updated_params));
        }
    }

    // Compile animations from Porytiles format to Porymap format
    pipeline_helper_compile_animations();

    // Apply manual animation overrides to metatiles_bin (must happen before dual-layerization)
    pipeline_helper_apply_manual_overrides();

    // If user is requesting dual-layer, use the input Porytiles-format metatiles to infer the LayerType for each
    // metatile and remove the relevant tilemap entries. Here, we assume that the Porytiles metatiles have already been
    // validated in an earlier step as dual-layer compatible.
    // Gather any per-metatile explicit layer-type overrides. These pin the layer type against inference and, in dual
    // mode, drive both the dropped-layer selection and the stored attribute below.
    const std::vector<std::optional<LayerType>> explicit_layer_types = gather_explicit_layer_types();

    LayerModeConverter layer_mode_converter{&format_, &diag_, &tile_printer_, extrinsic_transparency_};
    const auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile_);
    if (configured_layer_mode == LayerMode::dual) {
        const auto &dual_layerized = layer_mode_converter.dual_layerize(
            new_porymap_component_->metatiles_bin(), porytiles_metatiles_, explicit_layer_types);
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
        const auto maybe_porytiles_attribute = tileset_.porytiles_component().get_attribute(i);
        MetatileAttribute new_attribute{};
        if (maybe_porytiles_attribute.has_value()) {
            new_attribute = maybe_porytiles_attribute.value();
        }
        else {
            // A metatile with no stored attribute (e.g. a CSV row omitted as all-default) materializes from the
            // schema defaults, not from all-zero fields. This is what lets the CSV writer omit all-default rows even
            // under a schema with nonzero defaults: the omitted row reloads as an absent attribute here and comes
            // back as exactly the defaults it was omitted for.
            for (const Field &field : schema_.fields()) {
                new_attribute.field(field.name(), field.default_value());
            }
        }
        // An explicit override wins uniformly, including triple mode: the user owns those rows.
        new_attribute.layer_type(new_attribute.explicit_layer_type().value_or(layer_type));
        new_porymap_component_->push_back_attribute(new_attribute);
    }

    // Export tiles in original form
    if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
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
        // Primary palette slots
        if (has_paired_primary()) {
            for (std::size_t i = 0; i < num_palettes_in_primary_.value(); i++) {
                new_porymap_component_->set_palette(i, paired_primary_->porymap_component().palette_at(i));
            }
        }
        else {
            // Standalone secondary: zeroed palettes for primary slots
            for (std::size_t i = 0; i < num_palettes_in_primary_.value(); i++) {
                new_porymap_component_->set_palette(
                    i, Palette<Rgba32, palette::max_size>{Rgba32{0, 0, 0, Rgba32::alpha_opaque}});
            }
        }
        // Secondary palettes from packing result
        for (std::size_t i = num_palettes_in_primary_.value(); i < num_palettes_total_.value(); i++) {
            new_porymap_component_->set_palette(i, new_porymap_palettes_.at(i));
        }
        // Junk/reserved palettes (13-15) from original secondary component
        for (std::size_t i = num_palettes_total_.value(); i < palette::num_palettes; i++) {
            new_porymap_component_->set_palette(i, tileset_.porymap_component().palette_at(i));
        }
    }
    else {
        for (std::size_t i = 0; i < palette::num_palettes; i++) {
            new_porymap_component_->set_palette(i, new_porymap_palettes_.at(i));
        }
    }

    // Apply true_color palette encoding to tiles.png if configured
    if (tiles_palette_mode_ == TilesPaletteMode::true_color) {
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
            porymap_tilemap_entry.palette_index(),
            static_cast<bool>(porymap_tilemap_entry.h_flip() ^ pt_to_pm_hflip),
            static_cast<bool>(porymap_tilemap_entry.v_flip() ^ pt_to_pm_vflip)};
    }

    // No match found
    return std::nullopt;
}

TileAssignmentResult CompilerTask::pipeline_helper_assign_tile_via_palette_match(
    const PixelTile<Rgba32> &porytiles_tile, std::size_t flat_index)
{
    TileAssignmentResult result{};

    // Use the packer's authoritative palette assignment when available (optimize mode). This ensures tile sharing
    // alignment is respected. The packer and alignment system chose specific palettes for each tile, and re-deriving
    // via match_or_best could pick a different palette that breaks sharing slot alignment.
    //
    // Falls back to match_or_best for tiles not in the packer's assignments (e.g., locked/patch modes, or tiles
    // excluded from packing like animation keyframes).
    std::size_t palette_index;
    if (tile_to_palette_.contains(flat_index)) {
        palette_index = tile_to_palette_.at(flat_index);
    }
    else {
        std::vector<PaletteMatchResult<Rgba32>> matches =
            match_or_best(porytiles_tile, new_porymap_palettes_, extrinsic_transparency_.value(), 1);

        if (!matches.at(0).is_covered) {
            result.status = TileAssignmentResult::Status::no_covering_palette;
            result.match_results = std::move(matches);
            return result;
        }
        palette_index = matches.at(0).palette_index;
    }

    const auto &matched_palette = new_porymap_palettes_.at(palette_index);
    const auto index_tile =
        index_tile_from_color_tile(porytiles_tile, matched_palette, extrinsic_transparency_.value());
    const CanonicalPixelTile canonical_index_tile{index_tile};

    // In non-optimize modes with available original tilemap data, only use the animation matcher if the original
    // tile_index was within a registered animation range. This prevents false positive interception where a static tile
    // that visually matches an animation keyframe gets incorrectly mapped to the animation tile_index, causing
    // unintended animation at runtime.
    bool should_check_anim_matcher = true;
    if (tiles_edit_mode_ != ArtifactEditMode::optimize && flat_index < porymap_tilemap_entries_.size()) {
        const auto original_tile_index = porymap_tilemap_entries_[flat_index].tile_index();
        should_check_anim_matcher = anim_tile_matcher_.is_in_animation_range(original_tile_index);
    }

    // Check if tile matches a registered animation keyframe
    if (should_check_anim_matcher) {
        if (const auto anim_match = anim_tile_matcher_.find_match(
                CanonicalPixelTile{porytiles_tile, extrinsic_transparency_.value()}, extrinsic_transparency_.value());
            anim_match.has_value()) {
            // The remark is gated on is_cross_tileset by design: it surfaces new runtime behavior, i.e. a tile that
            // now animates because it links into primary art. Matches against this tileset's own animations are the
            // expected path and stay silent.
            if (anim_match->is_cross_tileset) {
                std::vector<std::string> remark_lines;
                remark_lines.emplace_back(format_.format(
                    "Tile at flat index '{}' matched primary animation '{}' subtile '{}'.",
                    FormatParam{flat_index, Style::bold},
                    FormatParam{anim_match->anim_name, Style::bold},
                    FormatParam{anim_match->keyframe_tile_idx, Style::bold}));
                diag_.remark("cross-tileset-anim-match", remark_lines);
            }
            // Use the animation tile index with palette from anim_palette_indices_ and computed flip bits
            result.status = TileAssignmentResult::Status::success;
            result.entry = TilemapEntry{
                anim_match->tile_index,
                anim_palette_indices_.at(anim_match->anim_name).at(anim_match->keyframe_tile_idx),
                anim_match->h_flip,
                anim_match->v_flip};
            return result;
        }
    }

    // Tile found in workspace
    //
    // In optimize mode, we use fast O(1) exact index matching because palettes are freshly computed by the palette
    // packing algorithm, which never produces duplicate colors. In patch/locked modes, we use O(n) color-equivalence
    // comparison because vanilla palettes may contain duplicate colors at different indices. For example, if palette
    // slots 7 and 14 both contain RGB(255,0,0), our index_tile_from_color_tile() always picks slot 7 (the first
    // match), but vanilla workspace tiles might use slot 14. Exact index matching would fail to find the tile, causing
    // unnecessary tile insertions or "tile not found" errors in locked mode.
    const auto maybe_tile_index =
        (tiles_edit_mode_ == ArtifactEditMode::optimize)
            ? tiles_workspace_->first_occurrence_of(canonical_index_tile)
            : tiles_workspace_->first_occurrence_of_by_color(canonical_index_tile, matched_palette);

    if (maybe_tile_index.has_value()) {
        const auto workspace_tile_index = maybe_tile_index.value();

        // Warn if workspace fallthrough resolved to a primary animation range. When cross-tileset linking is
        // enabled, the RGBA key frame matcher (above) catches tiles that visually match primary key frames.
        // This branch catches a different case: tiles that don't match the RGBA key frame pixels but produce
        // identical IndexPixel data after palette mapping (indexed-pixel coincidence). This happens when two
        // visually distinct RGBA tiles map to the same palette indices. When cross-tileset linking is disabled,
        // this catches all workspace-level matches to primary animation ranges.
        //
        // The O(primary_anims) scan is deliberate. AnimTileMatcher::is_in_animation_range() cannot replace it: when
        // cross-tileset linking is disabled the primary animations are never registered in the matcher, the matcher
        // mixes in this tileset's own animation ranges, and it returns only a bool while the diagnostic needs the
        // animation name.
        if (is_secondary() && has_paired_primary()) {
            const auto &primary_porymap_anims = paired_primary_->porymap_component().anims();
            for (const auto &[anim_name, anim] : primary_porymap_anims) {
                const std::size_t offset = anim.params().tile_offset();
                const std::size_t count = anim.params().tile_count();
                if (workspace_tile_index >= offset && workspace_tile_index < offset + count) {
                    std::vector<std::string> warning_lines;
                    if (cross_tileset_anim_linking_.value()) {
                        warning_lines.emplace_back(format_.format(
                            "Tile at flat index '{}' resolved to primary animation '{}' range via workspace lookup "
                            "(not key frame matching).",
                            FormatParam{flat_index},
                            FormatParam{anim_name, Style::bold}));
                        warning_lines.emplace_back(
                            "The tile may not visually match the key frame but produces identical indexed pixel data.");
                        diag_.warning("cross-tileset-anim-fallthrough", warning_lines);
                    }
                    else {
                        warning_lines.emplace_back(format_.format(
                            "Tile at flat index '{}' resolved to primary animation '{}' range via workspace "
                            "deduplication, despite '{}' being disabled.",
                            FormatParam{flat_index},
                            FormatParam{anim_name, Style::bold},
                            FormatParam{"cross_tileset_anim_linking", Style::bold}));
                        warning_lines.emplace_back("This tile will animate at runtime.");
                        warning_lines.emplace_back(
                            "To suppress, restructure your tile art to avoid matching primary animation pixels, or "
                            "enable cross-tileset linking for explicit control.");
                        diag_.warning("cross-tileset-anim-fallthrough-disabled", warning_lines);
                    }
                    break;
                }
            }
        }

        const auto workspace_tile = tiles_workspace_->tile_at(workspace_tile_index);
        const bool pt_to_pm_hflip = canonical_index_tile.h_flip() ^ workspace_tile.h_flip();
        const bool pt_to_pm_vflip = canonical_index_tile.v_flip() ^ workspace_tile.v_flip();
        result.status = TileAssignmentResult::Status::success;
        result.entry = TilemapEntry{workspace_tile_index, palette_index, pt_to_pm_hflip, pt_to_pm_vflip};
        return result;
    }

    // Tile not found - locked mode cannot insert new tiles
    if (tiles_edit_mode_ == ArtifactEditMode::locked) {
        result.status = TileAssignmentResult::Status::tile_not_found;
        result.index_tile = index_tile;
        result.palette_index = palette_index;
        result.matched_palette = matched_palette;
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
    result.entry = TilemapEntry{inserted_index, palette_index, pt_to_pm_hflip, pt_to_pm_vflip};
    return result;
}

ChainableResult<void> CompilerTask::pipeline_helper_run_palette_packing()
{
    // Create ColorIndexMap from the Porytiles tiles, Porytiles palettes, and palette hints. We already validated
    // earlier that we don't exceed the global color count limit. So this will panic if there are too many global unique
    // colors.
    const std::size_t color_count_limit =
        is_secondary() ? (num_palettes_total_.value() - num_palettes_in_primary_.value()) * (palette::max_size - 1)
                       : num_palettes_in_primary_.value() * (palette::max_size - 1);
    PT_TRY_ASSIGN_CHAIN_ERR(
        color_index_map,
        pipeline_helper_build_color_index_map(palette_hints_.value(), color_count_limit),
        void,
        format_.format("Failed to build color index map for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}));

    PT_UNWRAP_TILESET_CONFIG_REF(config_, packing_strategy, tileset_.name(), void);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, packing_strategy_params, tileset_.name(), void);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tile_sharing_packing, tileset_.name(), void);
    PT_UNWRAP_TILESET_CONFIG_REF(config_, tile_sharing_alignment, tileset_.name(), void);
    auto strategy = make_packing_strategy(packing_strategy.value(), packing_strategy_params.value(), diag_);
    PalettePacker palette_packer{strategy.get(), &format_, &diag_, &tile_printer_, &palette_printer_};
    std::bitset<palette::num_palettes> available_palettes{0};
    if (is_secondary()) {
        if (has_paired_primary()) {
            // Enable primary palette slots so the packer can assign tiles whose colors are a subset
            // of a locked primary palette. Primary palettes are fully locked via prefilled_palettes_, so
            // the packer cannot add new colors -- it can only assign tiles to them.
            for (std::size_t i = 0; i < num_palettes_in_primary_; i++) {
                available_palettes.set(i, true);
            }
        }
        for (std::size_t i = num_palettes_in_primary_; i < num_palettes_total_; i++) {
            available_palettes.set(i, true);
        }
    }
    else {
        for (std::size_t i = 0; i < num_palettes_in_primary_; i++) {
            available_palettes.set(i, true);
        }
    }
    PackingParams packing_params{};
    packing_params.tiles_ = porytiles_pixel_rgba_;
    packing_params.anims_ = tileset_.porytiles_component().anims();
    packing_params.color_map_ = color_index_map;
    packing_params.extrinsic_transparency_ = extrinsic_transparency_.value();
    if (is_secondary()) {
        std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled{};
        if (has_paired_primary()) {
            // Lock primary palettes from the compiled paired primary
            for (std::size_t i = 0; i < num_palettes_in_primary_.value(); ++i) {
                prefilled.at(i) = paired_primary_->porymap_component().palette_at(i);
            }
        }
        // Carry over secondary Porytiles palette overrides (slots >= num_palettes_in_primary)
        for (std::size_t i = num_palettes_in_primary_.value(); i < palette::num_palettes; ++i) {
            if (tileset_.porytiles_component().palette_at(i).has_value()) {
                prefilled.at(i) = tileset_.porytiles_component().palette_at(i).value();
            }
        }
        packing_params.prefilled_palettes_ = prefilled;
    }
    else {
        packing_params.prefilled_palettes_ = tileset_.porytiles_component().palettes();
    }
    packing_params.hints_ = palette_hints_.value();
    packing_params.available_palettes_ = available_palettes;
    packing_params.tile_sharing_packing_ = tile_sharing_packing;
    packing_params.tile_sharing_alignment_ = tile_sharing_alignment;

    // Reconstruct RGBA tiles from the paired primary's compiled data for cross-tileset shape group analysis
    if (is_secondary() && has_paired_primary()) {
        const auto &primary_porymap = paired_primary_->porymap_component();
        ImageTileizer<IndexPixel> tileizer{};
        PT_TRY_ASSIGN_CHAIN_ERR(
            primary_indexed_tiles,
            tileizer.tileize(primary_porymap.tiles_png()),
            void,
            "Failed to tileize paired primary's tiles.png for cross-tileset shape group analysis.");

        // Normalize the paired primary's entries to triple-layer so a flat slot index decodes cleanly via
        // metatile::from_tile_index. This absorbs the dual-layer per-LayerType layout variations (normal/covered/split)
        // into canonical bottom/middle/top positioning; the inserted transparent entries are skipped by the existing
        // tile_index == 0 filter below.
        LayerModeConverter layer_mode_converter{&format_, &diag_, &tile_printer_, extrinsic_transparency_.value()};
        PT_TRY_ASSIGN_CHAIN_ERR(
            primary_triple_entries,
            layer_mode_converter.triple_layerize(primary_porymap),
            void,
            "Failed to triple-layerize paired primary for cross-tileset shape group analysis.");

        // Dedup on (tile_index, palette_index) ignoring flips. Shape group analysis canonicalizes
        // orientations, so different flip variants of the same tile produce the same canonical form.
        std::set<std::pair<std::size_t, std::size_t>> seen_tile_palette_pairs;

        for (std::size_t slot = 0; slot < primary_triple_entries.size(); ++slot) {
            const auto &entry = primary_triple_entries.at(slot);
            if (entry.tile_index() == 0) {
                continue;
            }
            auto key = std::make_pair(entry.tile_index(), entry.palette_index());
            if (seen_tile_palette_pairs.contains(key)) {
                continue;
            }
            seen_tile_palette_pairs.insert(key);

            if (entry.tile_index() >= primary_indexed_tiles.size()) {
                continue;
            }
            const auto &index_tile = primary_indexed_tiles.at(entry.tile_index());
            auto flipped_tile = index_tile.flip(entry.h_flip(), entry.v_flip());
            auto rgba_tile = color_tile_from_index_tile(
                flipped_tile, primary_porymap.palette_at(entry.palette_index()), extrinsic_transparency_.value());
            if (rgba_tile.is_transparent(extrinsic_transparency_.value())) {
                continue;
            }
            auto [mt_index, layer, subtile] = metatile::from_tile_index(slot);
            packing_params.primary_tiles_.emplace_back(
                PackingParams::PrimaryTileRef{std::move(rgba_tile), entry.palette_index(), mt_index, layer, subtile});
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        palette_packing,
        palette_packer.pack_tiles(packing_params),
        void,
        format_.format("Failed to pack palettes for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}));

    tile_to_palette_ = std::move(palette_packing.tile_to_palette_);

    for (std::size_t i = 0; i < palette::num_palettes; i++) {
        if (const auto &maybe_packed_palette = palette_packing.palettes_.at(i); maybe_packed_palette.has_value()) {
            // Copy over the packed palette
            new_porymap_palettes_[i] = maybe_packed_palette.value();
        }
        else if (tileset_.porytiles_component().palette_at(i).has_value()) {
            // Out-of-band Porytiles palette: exists but wasn't used in packing (e.g., palette 11.pal in a primary
            // tileset). Resolve all wildcards to black and copy it over.
            const auto &porytiles_palette = tileset_.porytiles_component().palette_at(i).value();
            Palette<Rgba32, palette::max_size> resolved_palette{Rgba32{0, 0, 0, Rgba32::alpha_opaque}};

            // Handle slot 0: preserve if not wildcard, otherwise use extrinsic transparency
            if (!porytiles_palette.is_wildcard(0)) {
                resolved_palette.set(0, porytiles_palette.at(0));
            }
            else {
                resolved_palette.set(0, extrinsic_transparency_.value());
            }

            // Copy non-wildcard slots (wildcards remain as the default black)
            for (std::size_t j = 1; j < palette::max_size; ++j) {
                if (!porytiles_palette.is_wildcard(j)) {
                    resolved_palette.set(j, porytiles_palette.at(j));
                }
            }

            new_porymap_palettes_[i] = resolved_palette;
        }
        else {
            // Copy remaining secondary palettes from the original component. The "secondary" palettes in a primary
            // tileset's folder won't be actually loaded by the game engine. Porymap also doesn't show them -- it
            // will grab palettes from the relevant secondary set folder. However, we copy them here for consistency. If
            // for some reason the user had edited them, we don't want to clobber their edits. Porytiles should be
            // surgical where possible.
            //
            // Copy junk palettes. 13.pal, 14.pal, 15.pal exist in the tileset but are reserved by the game engine for
            // overworld/shop UI. Here we just copy them over as-is. Again, if for some reason the user had edited
            // them, let's not clobber anything unnecessarily.
            new_porymap_palettes_[i] = tileset_.porymap_component().palette_at(i);
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
    const std::size_t palette_start = is_secondary() ? num_palettes_in_primary_.value() : 0;
    const std::size_t palette_end = is_secondary() ? num_palettes_total_.value() : num_palettes_in_primary_.value();
    for (std::size_t palette_index = palette_start; palette_index < palette_end; ++palette_index) {
        const auto &maybe_porytiles_palette = tileset_.porytiles_component().palettes().at(palette_index);
        if (!maybe_porytiles_palette.has_value()) {
            continue;
        }
        color_index_map.add_palette(maybe_porytiles_palette.value(), extrinsic_transparency_.value());
    }

    // Add palette hints
    for (const auto &hint : hints) {
        color_index_map.add_palette(hint.palette(), extrinsic_transparency_.value());
    }

    // Check color count one more time, we validated this earlier and provided granular feedback to user
    if (color_index_map.size() > color_count_limit) {
        panic(
            "color_index_map.size() > count_limit - this should have already been validated by "
            "pipeline_step_validate_input");
    }

    // For secondary compilation, add primary palette colors to the map. The packer needs these to build ColorSets for
    // locked primary palettes, enabling secondary tiles that only use primary colors to be correctly assigned to a
    // primary palette. These colors don't count against the secondary color budget, so they're added after the limit
    // check.
    if (is_secondary() && has_paired_primary()) {
        for (std::size_t i = 0; i < num_palettes_in_primary_.value(); ++i) {
            const auto &primary_palette = paired_primary_->porymap_component().palette_at(i);
            color_index_map.add_palette(primary_palette, extrinsic_transparency_.value());
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

    // For automatic/hybrid mode, we use the key frame tiles. For manual mode (no key frame),
    // we use the first regular frame's tiles as the representative tiles to place in tiles.png.
    const AnimFrame<Rgba32> &representative_frame =
        anim.has_key_frame() ? anim.key_frame() : anim.frames().begin()->second;

    for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
        const PixelTile<Rgba32> &composite_rgba_tile = composite_frame.tile_at(tile_idx);
        const PixelTile<Rgba32> &representative_tile = representative_frame.tile_at(tile_idx);

        // Transparent representative tiles are valid for animations without a key frame. They just produce a
        // transparent IndexPixel tile with palette index 0. For key frame animations, validate_anim_frames() catches
        // transparent tiles before we get here.
        if (representative_tile.is_transparent(extrinsic_transparency_.value())) {
            PixelTile<IndexPixel> transparent_tile{IndexPixel{0}};
            result.tiles.emplace_back(transparent_tile);
            result.palette_indices.push_back(0);
            result.palettes.push_back(&new_porymap_palettes_.at(0));
            continue;
        }

        // Match tile to palette using composite frame to guarantee correct palette selection. As we have seen, some
        // animations, like FireRed General's water_current_landwatersedge, have animated tiles that different palettes
        // in different tilemap entries. Here, we're only selecting the first matching palette. It will be up to the
        // user to ensure that the other palettes are aligned such that the IndexTile we generate from this step will
        // work for every palette the animation uses.
        //
        // Eventually, when we support tileset.tiles.sharing configuration, we might want to make this approach more
        // sophisticated.
        std::vector<PaletteMatchResult<Rgba32>> matches =
            match_or_best(composite_rgba_tile, new_porymap_palettes_, extrinsic_transparency_.value(), 1);

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
                err_params.push_back({FormatParam{palette_filename(match.palette_index), Style::bold}});
                for (const auto &line : palette_printer_.print_rgba_palette_covered_missing(
                         new_porymap_palettes_.at(match.palette_index), match.covered_colors, match.missing_colors)) {
                    err_lines.push_back(line);
                    err_params.emplace_back();
                }
                match_idx++;
            }

            return FormattableError{std::move(err_lines), std::move(err_params)};
        }

        // Convert key frame tile to IndexPixel using matched palette
        const std::size_t palette_index = matches.at(0).palette_index;
        const auto &matched_palette = new_porymap_palettes_.at(palette_index);
        const PixelTile<IndexPixel> indexed_key_frame_tile =
            index_tile_from_color_tile(representative_tile, matched_palette, extrinsic_transparency_.value());

        result.tiles.emplace_back(indexed_key_frame_tile);
        result.palette_indices.push_back(palette_index);
        // We'll only actually use this vector in patch mode, but compute anyway to simplify code paths
        result.palettes.push_back(&matched_palette);
    }

    return result;
}

ChainableResult<void> CompilerTask::pipeline_helper_register_animations()
{
    // This function has two primary responsibilities. For each anim:
    //
    // 1. Place the anim's key frame tiles into tiles.png at computed offsets
    // 2. Register each animation and save the computed offsets
    //
    // The strategy differs by mode:
    // - optimize: Reserve slots at the start, place keyframes in reserved region
    // - patch: Try to reuse existing keyframes, else find contiguous free space
    // - locked: Keyframes must already exist in tiles.png
    const auto &anims = tileset_.porytiles_component().anims();

    if (!anims.empty()) {

        if (tiles_edit_mode_ == ArtifactEditMode::optimize) {
            std::size_t total_keyframe_tiles = 0;
            for (const auto &anim : anims | std::views::values) {
                if (anim.has_key_frame()) {
                    total_keyframe_tiles += anim.key_frame().tiles().size();
                }
                else if (anim.has_frames()) {
                    total_keyframe_tiles += anim.frames().begin()->second.tiles().size();
                }
            }
            const std::size_t anim_start = is_secondary() ? (num_tiles_in_primary_.value() + 1) : 1;
            tiles_workspace_->reserve_anim_slots(total_keyframe_tiles, anim_start);
        }

        std::map<std::string, std::size_t> anim_offsets;
        std::map<std::string, std::vector<std::size_t>> anim_palette_indices;
        std::size_t current_offset = tiles_workspace_->anim_start_offset();

        const auto &per_anim_overrides = per_anim_overrides_.value();

        for (const auto &[anim_name, anim] : anims) {
            if (!anim.has_frames()) {
                panic("anim '" + anim_name + "' has no frames");
            }

            // Build keyframe data (common to all modes, needed for palette_indices even if we skip tile placement)
            PT_TRY_ASSIGN_PASS_ERR(keyframe_data, pipeline_helper_build_keyframe_data(anim_name, anim), void);

            const std::size_t tile_count = keyframe_data.tiles.size();
            anim_palette_indices[anim_name] = keyframe_data.palette_indices;
            std::size_t offset{};

            // Resolve effective FrameLinking for this animation
            const ConfigValue<FrameLinking> effective_linking =
                (per_anim_overrides.contains(anim_name) && per_anim_overrides.at(anim_name).linking.has_value())
                    ? per_anim_overrides_.derive(per_anim_overrides.at(anim_name).linking)
                    : global_frame_linking_;

            if (effective_linking == FrameLinking::manual && tiles_edit_mode_ != ArtifactEditMode::optimize) {
                // Manual frame linking in patch/locked mode: use the tile_offset from anim.json directly.
                // Don't search tiles.png. The keyframes may not be findable via color matching. Whatever
                // is already at that offset in tiles.png will be dynamically overwritten by the game's
                // animation DMA code at runtime anyway.
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
                        std::vector<std::string> err_msg{};
                        err_msg.emplace_back(format_.format(
                            "Animation '{}' keyframes not found in existing tiles.png.",
                            FormatParam{anim_name, Style::bold}));
                        err_msg.emplace_back(format_.format(
                            "Cannot proceed due to '{}' setting '{}'.",
                            FormatParam{"Tiles Edit Mode", Style::bold},
                            FormatParam{"locked", Style::bold}));
                        err_msg.append_range(format_config_note_with_separator(format_, tiles_edit_mode_));
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
            anim_palette_indices_[anim_name] = anim_palette_indices.at(anim_name);
            anim_tile_matcher_.register_animation(
                anim_name, anim, anim_offsets.at(anim_name), extrinsic_transparency_.value());
        }

    } // if (!anims.empty())

    // Register primary animations for cross-tileset linking (secondary only). Ordering is load-bearing: the
    // secondary's own animations were registered above, so they win the matcher's first-registration-wins lookups
    // when key frames overlap. The collision check below then turns what would otherwise be a silent primary-side
    // loss into a fatal error.
    if (is_secondary() && has_paired_primary() && cross_tileset_anim_linking_.value()) {
        const auto &primary_porytiles_anims = paired_primary_->porytiles_component().anims();
        const auto &primary_porymap_anims = paired_primary_->porymap_component().anims();

        // Build a lookup from tile_index to palette_index using the primary's compiled metatile data.
        // This is the authoritative source for which palette each primary tile was compiled against.
        // If multiple metatile entries reference the same tile with different palettes, the first
        // entry wins (consistent with the first-match convention used in
        // pipeline_helper_build_keyframe_data). The two conventions must stay in sync: switching
        // either side to last-match-wins (or raising on conflict) without the other would assign
        // cross-tileset animation tiles palettes that disagree with how their reused siblings were
        // compiled.
        std::map<std::size_t, std::size_t> primary_tile_palette_map;
        for (const auto &entry : paired_primary_->porymap_component().metatiles_bin()) {
            if (entry.tile_index() == 0) {
                continue;
            }
            primary_tile_palette_map.try_emplace(entry.tile_index(), entry.palette_index());
        }

        // Build primary palette vector once for RGBA fallback matching
        std::vector<Palette<Rgba32, palette::max_size>> primary_palettes;
        primary_palettes.reserve(num_palettes_in_primary_.value());
        for (std::size_t i = 0; i < num_palettes_in_primary_.value(); ++i) {
            primary_palettes.push_back(paired_primary_->porymap_component().palette_at(i));
        }

        // Check for stale compiled data: animations in porymap but removed from porytiles source
        for (const auto &primary_anim_name : primary_porymap_anims | std::views::keys) {
            if (!primary_porytiles_anims.contains(primary_anim_name)) {
                return FormattableError{std::vector<std::string>{
                    format_.format(
                        "Primary animation '{}' exists in compiled Porymap data but not in Porytiles source.",
                        FormatParam{primary_anim_name, Style::bold}),
                    "The paired primary tileset has uncompiled changes. Recompile it before compiling this "
                    "secondary."}};
            }
        }

        for (const auto &[prim_anim_name, prim_anim] : primary_porytiles_anims) {
            if (!primary_porymap_anims.contains(prim_anim_name)) {
                return FormattableError{std::vector<std::string>{
                    format_.format(
                        "Primary animation '{}' exists in Porytiles source but not in compiled Porymap data.",
                        FormatParam{prim_anim_name, Style::bold}),
                    "The paired primary tileset has uncompiled changes. Recompile it before compiling this "
                    "secondary."}};
            }
            if (!prim_anim.has_key_frame()) {
                // Manual-mode primary animations have no key frame for RGBA matching. They are still present in the
                // workspace and can be linked via fallthrough (which emits its own diagnostic).
                std::vector<std::string> remark_lines;
                remark_lines.emplace_back(format_.format(
                    "Primary animation '{}' has no key frame (likely manual frame linking).",
                    FormatParam{prim_anim_name, Style::bold}));
                remark_lines.emplace_back("Cross-tileset key frame matching is not possible for this animation.");
                diag_.remark("cross-tileset-anim-skip-no-keyframe", remark_lines);
                continue;
            }

            // Same-name collision check. A secondary-owned animation sharing a name with a paired-primary
            // animation cannot coexist with cross-tileset linking: the anim_palette_indices_ write below would
            // clobber the secondary's entry, and the matcher panics on cross-tileset name reuse as a backstop
            // invariant. Checked after the key-frame skip above so manual-linking primary animations, which
            // are never registered here, keep compiling as before.
            if (anims.contains(prim_anim_name)) {
                std::vector<std::string> err_msg{};
                err_msg.emplace_back(format_.format(
                    "Primary animation '{}' has the same name as a secondary animation.",
                    FormatParam{prim_anim_name, Style::bold}));
                err_msg.emplace_back(
                    "Cross-tileset animation linking requires unique animation names across primary and secondary.");
                err_msg.emplace_back("Rename the secondary (or primary) animation so the names are distinct.");
                err_msg.append_range(format_config_note_with_separator(format_, cross_tileset_anim_linking_));
                return FormattableError{err_msg};
            }

            const auto &prim_porymap_anim = primary_porymap_anims.at(prim_anim_name);
            const std::size_t prim_tile_offset = prim_porymap_anim.params().tile_offset();

            const std::size_t prim_tile_count = prim_anim.key_frame().tile_count();

            // Collision detection is performed before palette index resolution so that the error path does not waste
            // work on palette lookups. If a user has both a collision and an unreferenced subtile, the collision wins:
            // collisions indicate an art-side conflict between the primary and secondary that must be resolved before
            // anything else, while an unreferenced subtile is a data-layout issue downstream of art choices.
            //
            // The two loops are independent. Collision detection only reads anim_tile_matcher_;
            // the palette lookup only reads primary_tile_palette_map.
            //
            // Check for cross-tileset key frame collisions. Any non-cross-tileset match is a collision with a
            // secondary animation. Matches flagged is_cross_tileset come from primary animations registered on
            // earlier loop iterations and are intentionally ignored: two primary animations sharing subtile art is a
            // primary-side authoring mistake, and the primary compiler owns its own validation. This check only
            // protects the cross-tileset boundary.
            //
            // Each side of the comparison uses its own ET: primary subtiles are classified under the paired primary's
            // ET and the canonical form is built under that ET, while the matcher's internal comparator classifies
            // the already-registered secondary entries under the secondary's ET. This is what lets the comparator
            // find a collision across mismatched-ET inputs.
            //
            // The check must keep using CanonicalPixelTile + find_match, the exact mechanism the tile assignment loop
            // later uses to match secondary tiles against these key frames. A different or looser comparator would
            // let the collision check pass for subtiles that still collide at assignment time, reintroducing the
            // silent first-registration-wins loss this check exists to prevent.
            for (std::size_t i = 0; i < prim_tile_count; ++i) {
                if (prim_anim.key_frame().tile_at(i).is_transparent(paired_primary_extrinsic_transparency_.value())) {
                    continue;
                }
                CanonicalPixelTile<Rgba32> canonical{
                    prim_anim.key_frame().tile_at(i), paired_primary_extrinsic_transparency_.value()};
                auto match = anim_tile_matcher_.find_match(canonical, paired_primary_extrinsic_transparency_.value());
                if (match.has_value() && !match->is_cross_tileset) {
                    return FormattableError{std::vector<std::string>{
                        format_.format(
                            "Primary animation '{}' subtile '{}' has identical RGBA data to secondary animation '{}' "
                            "subtile '{}'.",
                            FormatParam{prim_anim_name, Style::bold},
                            FormatParam{i},
                            FormatParam{match->anim_name, Style::bold},
                            FormatParam{match->keyframe_tile_idx}),
                        "Cross-tileset animation linking requires unique key frame subtiles across primary and "
                        "secondary.",
                        "Fix the secondary (or primary) animation art so key frame subtiles are visually distinct."}};
                }
            }

            // Palette resolution cascade for cross-tileset subtiles:
            // 1. Try metatile lookup (authoritative when subtile is referenced in primary metatiles)
            // 2. Fall back to RGBA matching against primary palettes (for subtiles only referenced cross-tileset)
            // Use the composite frame for RGBA matching — it covers all colors across all animation frames.
            const AnimFrame<Rgba32> composite =
                prim_anim.composite_frame(paired_primary_extrinsic_transparency_.value());

            std::vector<std::size_t> subtile_palette_indices;
            subtile_palette_indices.reserve(prim_tile_count);
            for (std::size_t i = 0; i < prim_tile_count; ++i) {
                const std::size_t abs_tile_index = prim_tile_offset + i;

                if (prim_anim.key_frame().tile_at(i).is_transparent(paired_primary_extrinsic_transparency_.value())) {
                    // Transparent subtiles are skipped during register_animation. Push a dummy value.
                    subtile_palette_indices.push_back(0);
                    continue;
                }

                if (primary_tile_palette_map.contains(abs_tile_index)) {
                    subtile_palette_indices.push_back(primary_tile_palette_map.at(abs_tile_index));
                }
                else {
                    // Subtile not referenced in any primary metatile. Fall back to RGBA matching the composite tile
                    // against the primary's compiled palettes.
                    auto matches = match_or_best(
                        composite.tile_at(i), primary_palettes, paired_primary_extrinsic_transparency_.value(), 1);
                    if (matches.at(0).is_covered) {
                        subtile_palette_indices.push_back(matches.at(0).palette_index);
                        std::vector<std::string> remark_lines;
                        remark_lines.emplace_back(format_.format(
                            "Primary animation '{}' subtile '{}' (tile_index='{}') resolved via RGBA palette fallback "
                            "to palette '{}'.",
                            FormatParam{prim_anim_name, Style::bold},
                            FormatParam{i, Style::bold},
                            FormatParam{abs_tile_index, Style::bold},
                            FormatParam{matches.at(0).palette_index, Style::bold}));
                        remark_lines.emplace_back(
                            "Subtile is not referenced by any primary metatile but its colors match a primary "
                            "palette.");
                        diag_.remark("cross-tileset-anim-rgba-fallback", remark_lines);
                    }
                    else {
                        return FormattableError{std::vector<std::string>{
                            format_.format(
                                "Primary animation '{}' subtile '{}' (tile_index='{}') is not referenced by any "
                                "primary "
                                "metatile entry and its colors do not fully match any primary palette.",
                                FormatParam{prim_anim_name, Style::bold},
                                FormatParam{i},
                                FormatParam{abs_tile_index}),
                            "Cannot determine the correct palette index for cross-tileset linking.",
                            "Recompile the primary tileset, or verify that all primary animation subtiles are used "
                            "in at least one primary metatile."}};
                    }
                }
            }

            anim_palette_indices_[prim_anim_name] = subtile_palette_indices;
            anim_tile_matcher_.register_animation(
                prim_anim_name,
                prim_anim,
                prim_tile_offset,
                paired_primary_extrinsic_transparency_.value(),
                /*is_cross_tileset=*/true);
        }
    }

    return {};
}

ChainableResult<void> CompilerTask::pipeline_helper_validate_primary_anim_subtile_coverage() const
{
    // Walk each primary animation's key frame subtiles. For every non-transparent subtile, verify its absolute tile
    // index appears in at least one metatile entry of this primary's tilemap entries. Unreferenced subtiles are not
    // fatal. Paired secondary compiles can resolve their palette via RGBA fallback matching. However, they are worth
    // warning about since explicit metatile references are the preferred palette resolution path.
    //
    // Animations without a key frame (manual frame linking, no RGBA reference) are skipped: they have no palette to
    // resolve via metatile lookup.
    const auto &anims = tileset_.porytiles_component().anims();
    if (anims.empty()) {
        return {};
    }

    // Collect tile indices referenced by any metatile entry in this primary's compiled tilemap. Tile 0 is the reserved
    // transparent tile and is excluded (it carries no palette information).
    std::unordered_set<std::size_t> referenced_tile_indices;
    for (const auto &entry : new_porymap_component_->metatiles_bin()) {
        if (entry.tile_index() == 0) {
            continue;
        }
        referenced_tile_indices.insert(entry.tile_index());
    }

    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_key_frame()) {
            continue;
        }

        auto maybe_tile_offset = anim_tile_matcher_.tile_offset_for(anim_name);
        if (!maybe_tile_offset.has_value()) {
            panic("animation '" + anim_name + "' not registered in anim_tile_matcher_");
        }
        const std::size_t tile_offset = maybe_tile_offset.value();
        const std::size_t tile_count = anim.key_frame().tile_count();

        for (std::size_t i = 0; i < tile_count; ++i) {
            if (anim.key_frame().tile_at(i).is_transparent(extrinsic_transparency_.value())) {
                continue;
            }
            const std::size_t abs_tile_index = tile_offset + i;
            if (!referenced_tile_indices.contains(abs_tile_index)) {
                std::vector<std::string> warn_lines;
                warn_lines.emplace_back(format_.format(
                    "Primary animation '{}' subtile '{}' (tile_index='{}') is not referenced by any metatile:",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{i, Style::bold},
                    FormatParam{abs_tile_index, Style::bold}));
                warn_lines.append_range(
                    tile_printer_.print_tile(anim.key_frame().tile_at(i), extrinsic_transparency_.value()));
                warn_lines.emplace_back(
                    "Palette assignment for this subtile will use RGBA fallback matching during secondary "
                    "compilation.");

                diag_.warning("primary-anim-unreferenced-subtile", warn_lines);
            }
        }
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
        std::vector<std::size_t> subtile_palette_indices;
        subtile_palette_indices.reserve(tile_count);

        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            const PixelTile<Rgba32> &composite_tile = composite.tile_at(tile_idx);

            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_tile, new_porymap_palettes_, extrinsic_transparency_.value(), 1);

            if (!matches.at(0).is_covered) {
                panic(
                    "animation '" + anim_name + "' subtile " + std::to_string(tile_idx) +
                    " has no covering palette during compilation");
            }

            subtile_palette_indices.push_back(matches.at(0).palette_index);
        }

        // 4. Determine palette for PNG display and warn if multiple palettes are used
        const std::size_t frame_palette_index = subtile_palette_indices.at(0);
        const bool uses_multiple_palettes =
            !std::ranges::all_of(subtile_palette_indices, [&](std::size_t idx) { return idx == frame_palette_index; });

        if (uses_multiple_palettes) {
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(format_.format(
                "Animation '{}' uses multiple palettes across subtiles.", FormatParam{anim_name, Style::bold}));
            warning_lines.emplace_back(format_.format(
                "Porymap-component frame PNGs will be saved using palette '{}' for display purposes.",
                FormatParam{palette_filename(frame_palette_index), Style::bold}));
            diag_.warning("multi-palette-animation", warning_lines);
        }

        // Build a dynamic palette for embedding in the AnimFrame
        const auto &fixed_palette = new_porymap_palettes_.at(frame_palette_index);
        Palette<Rgba32> anim_palette{};
        for (std::size_t i = 0; i < fixed_palette.size(); ++i) {
            if (fixed_palette.is_wildcard(i)) {
                panic("Porymap palette '" + std::to_string(frame_palette_index) + "' has illegal wildcard");
            }
            anim_palette.add(fixed_palette.at(i));
        }

        // 5. Convert regular frames (key frame not needed in compiled format)
        Animation<IndexPixel> compiled_anim{anim_name};

        for (const auto &[frame_name, source_frame] : source_anim.frames()) {
            std::vector<PixelTile<IndexPixel>> frame_index_tiles;
            frame_index_tiles.reserve(tile_count);

            for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
                const PixelTile<Rgba32> &rgba_tile = source_frame.tile_at(tile_idx);
                const auto &palette = new_porymap_palettes_.at(subtile_palette_indices[tile_idx]);

                frame_index_tiles.push_back(
                    index_tile_from_color_tile(rgba_tile, palette, extrinsic_transparency_.value()));
            }

            AnimFrame frame{frame_name, std::move(frame_index_tiles)};
            frame.palette(anim_palette);
            compiled_anim.put_frame(frame_name, std::move(frame));
        }

        // 6. Set params with updated tile_offset/tile_count
        AnimParams params = source_anim.params();
        const std::size_t local_offset = is_secondary() ? tile_offset - num_tiles_in_primary_.value() : tile_offset;
        params.tile_offset(local_offset);
        params.tile_count(tile_count);
        compiled_anim.params(std::move(params));

        // 7. Add to output component (key_frame left as std::nullopt)
        new_porymap_component_->add_anim(std::move(compiled_anim));
    }
}

std::vector<std::optional<LayerType>> CompilerTask::gather_explicit_layer_types() const
{
    std::vector<std::optional<LayerType>> explicit_layer_types;
    explicit_layer_types.reserve(porytiles_metatiles_.size());
    for (std::size_t i = 0; i < porytiles_metatiles_.size(); i++) {
        const auto maybe_attribute = tileset_.porytiles_component().get_attribute(i);
        explicit_layer_types.push_back(
            maybe_attribute.has_value() ? maybe_attribute.value().explicit_layer_type() : std::nullopt);
    }
    return explicit_layer_types;
}

void CompilerTask::pipeline_helper_apply_manual_overrides()
{
    const auto &source_anims = tileset_.porytiles_component().anims();
    // Bail only when there is genuinely nothing to apply. A secondary can carry primary_references without defining any
    // animations of its own, so guarding on source_anims alone would silently drop those overrides.
    if (source_anims.empty() && tileset_.porytiles_component().primary_anim_overrides().empty()) {
        return;
    }

    const auto &per_anim_overrides = per_anim_overrides_.value();

    // Kept alive for the validator, which holds it by const reference. Both this validation and dual_layerize must see
    // the same overrides so their dropped-layer decisions agree.
    const std::vector<std::optional<LayerType>> explicit_layer_types = gather_explicit_layer_types();

    const OverrideEntryValidator validator{
        format_,
        diag_,
        num_palettes_total_,
        layer_mode_from_val(num_tiles_per_metatile_.value()),
        porytiles_metatiles_,
        extrinsic_transparency_.value(),
        explicit_layer_types};
    const OverridePathInfo manual_path{"manual", "Animation '{}'"};
    const OverridePathInfo primary_refs_path{"primary-references", "Primary reference '{}'"};

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

            // Get the tile_offset and tile_count for this animation from the matcher
            auto maybe_tile_offset = anim_tile_matcher_.tile_offset_for(anim_name);
            if (!maybe_tile_offset.has_value()) {
                panic("animation '" + anim_name + "' not registered in anim_tile_matcher_");
            }
            const std::size_t tile_offset = maybe_tile_offset.value();

            auto maybe_tile_count = anim_tile_matcher_.tile_count_for(anim_name);
            if (!maybe_tile_count.has_value()) {
                panic("animation '" + anim_name + "' not registered in anim_tile_matcher_");
            }
            const std::size_t tile_count = maybe_tile_count.value();

            // Apply each override entry to metatiles_bin
            auto &metatiles_bin = new_porymap_component_->metatiles_bin();
            for (const auto &entry : overrides) {
                if (!validator.should_apply(manual_path, anim_name, entry, tile_count)) {
                    continue;
                }

                const std::size_t bin_index =
                    entry.metatile_id * metatile::entries_per_metatile_triple +
                    static_cast<std::size_t>(entry.layer) * metatile::tiles_per_metatile_layer +
                    static_cast<std::size_t>(entry.subtile);

                const std::size_t absolute_tile = tile_offset + entry.frame_subtile;
                metatiles_bin.at(bin_index) =
                    TilemapEntry{absolute_tile, entry.palette_index, entry.h_flip, entry.v_flip};
            }
            break;
        }

        case FrameLinking::hybrid: {
            std::vector<std::string> err_lines;
            err_lines.emplace_back(format_.format(
                "Hybrid frame linking is not yet implemented (animation '{}').", FormatParam{anim_name, Style::bold}));
            err_lines.emplace_back("Use 'automatic' or 'manual' frame linking until hybrid support.");
            err_lines.append_range(format_config_note_with_separator(format_, effective_linking));
            diag_.error("hybrid-frame-linking-not-implemented", err_lines);
            break;
        }

        default:
            panic("unhandled value for FrameLinking");
        }
    }

    // Apply primary animation reference overrides (secondary tilesets only)
    const auto &primary_refs = tileset_.porytiles_component().primary_anim_overrides();

    if (!primary_refs.empty() && !is_secondary()) {
        std::vector<std::string> err_lines;
        err_lines.emplace_back(format_.format(
            "Primary tilesets cannot have '{}' in anim.json.", FormatParam{"primary_references", Style::bold}));
        err_lines.emplace_back("Only secondary tilesets may reference primary animation tiles.");
        diag_.error("primary-references-on-primary", err_lines);
        return;
    }

    if (!primary_refs.empty() && is_secondary()) {
        if (!has_paired_primary()) {
            std::vector<std::string> err_lines;
            err_lines.emplace_back(format_.format(
                "The '{}' section requires a paired primary tileset (pairing mode must not be off).",
                FormatParam{"primary_references", Style::bold}));
            diag_.error("primary-references-no-paired-primary", err_lines);
            return;
        }

        const auto &primary_anims = paired_primary_->porymap_component().anims();
        auto &metatiles_bin = new_porymap_component_->metatiles_bin();

        for (const auto &[prim_anim_name, entries] : primary_refs) {
            if (!primary_anims.contains(prim_anim_name)) {
                std::vector<std::string> err_lines;
                err_lines.emplace_back(format_.format(
                    "Primary animation '{}' referenced in '{}' was not found in the paired primary tileset.",
                    FormatParam{prim_anim_name, Style::bold},
                    FormatParam{"primary_references", Style::bold}));
                diag_.error("primary-references-anim-not-found", err_lines);
                continue;
            }

            const auto &prim_anim = primary_anims.at(prim_anim_name);
            const std::size_t prim_tile_offset = prim_anim.params().tile_offset();
            const std::size_t prim_tile_count = prim_anim.params().tile_count();

            for (const auto &entry : entries) {
                if (!validator.should_apply(primary_refs_path, prim_anim_name, entry, prim_tile_count)) {
                    continue;
                }

                const std::size_t bin_index =
                    entry.metatile_id * metatile::entries_per_metatile_triple +
                    static_cast<std::size_t>(entry.layer) * metatile::tiles_per_metatile_layer +
                    static_cast<std::size_t>(entry.subtile);

                const std::size_t absolute_tile = prim_tile_offset + entry.frame_subtile;
                metatiles_bin.at(bin_index) =
                    TilemapEntry{absolute_tile, entry.palette_index, entry.h_flip, entry.v_flip};
            }
        }
    }
}

void CompilerTask::pipeline_helper_apply_true_color_to_tiles_png()
{
    // Phase 1: Build tile_index -> first_palette_index map from tilemap entries
    std::unordered_map<std::size_t, std::size_t> tile_to_first_palette;
    std::unordered_map<std::size_t, std::set<std::size_t>> tile_to_all_palettes;

    // Secondary tiles.png is densely packed from tile 0, but metatile entries reference absolute
    // indices (e.g., 512+ for secondary). This offset converts absolute to relative for image access.
    const std::size_t tile_index_offset = is_secondary() ? num_tiles_in_primary_.value() : 0;

    // Secondary palettes are stored at absolute indices (e.g., 6-11), but the PNG palette only
    // covers this tileset's palettes (indices 0-5). This offset converts absolute to relative.
    // When a secondary tileset has a paired primary, the packer can assign tiles to primary palettes.
    // Use offset 0 so the encoding preserves absolute palette indices in the PNG pixel values.
    const std::size_t palette_index_offset =
        (is_secondary() && !has_paired_primary()) ? num_palettes_in_primary_.value() : 0;

    // For diagnostic display of unreferenced tiles, always use the first palette belonging to this tileset.
    const std::size_t default_display_palette = is_secondary() ? num_palettes_in_primary_.value() : 0;

    for (const auto &entry : new_porymap_component_->metatiles_bin()) {
        const auto tile_idx = entry.tile_index();
        const auto palette_idx = entry.palette_index();

        if (tile_idx == 0) {
            continue; // Skip transparent tile
        }

        tile_to_all_palettes[tile_idx].insert(palette_idx);

        if (!tile_to_first_palette.contains(tile_idx)) {
            tile_to_first_palette[tile_idx] = palette_idx;
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

            if (tile_to_first_palette.contains(absolute_tile_idx)) {
                continue; // Already mapped from metatiles_bin
            }

            const PixelTile<Rgba32> &composite_tile = composite.tile_at(subtile_idx);
            std::vector<PaletteMatchResult<Rgba32>> matches =
                match_or_best(composite_tile, new_porymap_palettes_, extrinsic_transparency_.value(), 1);

            if (matches.at(0).is_covered) {
                const std::size_t matched_palette_idx = matches.at(0).palette_index;
                tile_to_first_palette[absolute_tile_idx] = matched_palette_idx;

                // Extract the tile to check for transparency and for visualization
                const auto &tiles_img = new_porymap_component_->tiles_png();
                const PixelTile<IndexPixel> index_tile =
                    extract_single_tile(tiles_img, absolute_tile_idx - tile_index_offset);

                // Skip remark for transparent tiles (unused slots)
                if (index_tile.is_transparent()) {
                    continue;
                }

                // Emit remark for animation-only tiles not referenced in metatiles
                constexpr auto tag = "true-color-anim-only-tile";
                std::vector<std::string> remark_lines;
                remark_lines.emplace_back(format_.format(
                    "Tile index '{}' (animation '{}', subtile '{}') is not referenced in metatiles.",
                    FormatParam{absolute_tile_idx, Style::bold},
                    FormatParam{anim_name, Style::bold},
                    FormatParam{subtile_idx, Style::bold}));
                remark_lines.emplace_back(format_.format(
                    "Using '{}' for true-color encoding (determined via palette matching).",
                    FormatParam{palette_filename(matched_palette_idx), Style::bold}));

                // Visualize the tile using the matched palette
                const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                    index_tile, new_porymap_palettes_.at(matched_palette_idx), extrinsic_transparency_.value());
                remark_lines.emplace_back();
                remark_lines.append_range(tile_printer_.print_tile(rgba_tile, extrinsic_transparency_.value()));

                diag_.remark(tag, remark_lines);
            }
        }
    }

    // Phase 3: Emit diagnostic remark for tiles used with multiple palettes
    for (const auto &[absolute_tile_idx, palettes] : tile_to_all_palettes) {
        if (palettes.size() > 1) {
            // Primary tiles are not in this tileset's tiles.png, skip
            if (absolute_tile_idx < tile_index_offset) {
                continue;
            }

            // Extract the tile to check for transparency and for visualization
            const auto &tiles_img = new_porymap_component_->tiles_png();
            const PixelTile<IndexPixel> index_tile =
                extract_single_tile(tiles_img, absolute_tile_idx - tile_index_offset);

            // Skip remark for transparent tiles (unused slots)
            if (index_tile.is_transparent()) {
                continue;
            }

            constexpr auto tag = "true-color-multi-palette-tile";
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(format_.format(
                "Tile index '{}' is used with multiple palettes.", FormatParam{absolute_tile_idx, Style::bold}));

            std::string palette_list;
            for (const auto palette : palettes) {
                if (!palette_list.empty()) {
                    palette_list += ", ";
                }
                palette_list += palette_filename(palette);
            }

            const std::size_t selected_palette_idx = tile_to_first_palette.at(absolute_tile_idx);
            remark_lines.emplace_back(format_.format(
                "Palettes used: {}; tiles.png will display using '{}'.",
                FormatParam{palette_list},
                FormatParam{palette_filename(selected_palette_idx), Style::bold}));

            // Visualize the tile under each palette resolution
            for (const auto palette_idx : palettes) {
                remark_lines.emplace_back();
                remark_lines.emplace_back(
                    format_.format("{} resolution:", FormatParam{palette_filename(palette_idx), Style::bold}));
                const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                    index_tile, new_porymap_palettes_.at(palette_idx), extrinsic_transparency_.value());
                remark_lines.append_range(tile_printer_.print_tile(rgba_tile, extrinsic_transparency_.value()));
            }

            diag_.remark(tag, remark_lines);
        }
    }

    // Phase 4: Transform tiles_png pixels
    Image<IndexPixel> tiles_img = new_porymap_component_->tiles_png();
    constexpr std::size_t tiles_per_row = metatile::metatiles_per_row * metatile::tiles_per_side;

    const std::size_t total_tiles = tiles_img.size_in_tiles();

    for (std::size_t tile_idx = 1; tile_idx < total_tiles; ++tile_idx) {
        const std::size_t absolute_tile_idx = tile_idx + tile_index_offset;
        if (!tile_to_first_palette.contains(absolute_tile_idx)) {
            // Extract the tile to check for transparency
            const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_img, tile_idx, tiles_per_row);

            // Skip remark for transparent tiles (unused slots) - user already knows they're unused
            if (index_tile.is_transparent()) {
                continue;
            }

            // Emit remark for unreferenced non-transparent tiles
            constexpr auto tag = "true-color-unreferenced-tile";
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(format_.format(
                "Tile index '{}' is not referenced in metatiles or animations.",
                FormatParam{absolute_tile_idx, Style::bold}));

            remark_lines.emplace_back("This tile may be used by a secondary tileset, or it may be completely unused.");
            remark_lines.emplace_back(format_.format(
                "Displaying using '{}' for color resolution.",
                FormatParam{palette_filename(default_display_palette), Style::bold}));

            // Visualize the tile using the first palette for this tileset
            const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                index_tile, new_porymap_palettes_.at(default_display_palette), extrinsic_transparency_.value());
            remark_lines.emplace_back();
            remark_lines.append_range(tile_printer_.print_tile(rgba_tile, extrinsic_transparency_.value()));

            diag_.remark(tag, remark_lines);
            continue; // Skip unreferenced tiles (no palette encoding needed)
        }

        const std::size_t palette_idx = tile_to_first_palette.at(absolute_tile_idx);
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
                const std::size_t new_index = ((palette_idx - palette_index_offset) << 4) | color_idx;
                tiles_img.set(row, col, IndexPixel{new_index});
            }
        }
    }

    // Phase 5: Build the 8-bit palette for the PNG (this tileset's palettes * 16 colors)
    std::size_t num_palettes;
    if (!is_secondary()) {
        num_palettes = num_palettes_in_primary_.value();
    }
    else if (has_paired_primary()) {
        num_palettes = num_palettes_total_.value();
    }
    else {
        num_palettes = num_palettes_total_.value() - num_palettes_in_primary_.value();
    }
    std::vector<Rgba32> true_color_palette;
    true_color_palette.reserve(num_palettes * palette::max_size);

    for (std::size_t i = 0; i < num_palettes; ++i) {
        const auto &palette = new_porymap_palettes_.at(i + palette_index_offset);
        for (std::size_t color_idx = 0; color_idx < palette::max_size; ++color_idx) {
            true_color_palette.push_back(palette.at(color_idx));
        }
    }

    tiles_img.palette(std::move(true_color_palette));
    new_porymap_component_->tiles_png(tiles_img);
}

void CompilerTask::pipeline_helper_emit_no_matching_tile_error(
    std::size_t tile_index,
    const PixelTile<IndexPixel> &index_tile,
    std::size_t palette_index,
    const Palette<Rgba32, palette::max_size> &matched_palette)
{
    constexpr auto tag = "no-matching-tile";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> no_match_err{};
    no_match_err.emplace_back(format_.format(
        "{}: no matching tile found",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
    no_match_err.append_range(tile_printer_.print_metatile_tile_highlight(
        porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_));
    diag_.error(tag, no_match_err);

    // Print note showing the palette that matched
    std::vector<std::string> palette_note{};
    palette_note.emplace_back(
        format_.format("matched palette '{}':", FormatParam{palette_filename(palette_index), Style::bold}));
    palette_note.append_range(palette_printer_.print_rgba_palette(matched_palette));
    diag_.error_note(tag, palette_note);

    // Print note showing the generated IndexPixel tile
    std::vector<std::string> tile_note{};
    tile_note.emplace_back("generated index tile:");
    tile_note.append_range(tile_printer_.print_tile(index_tile, extrinsic_transparency_.value()));
    diag_.error_note(tag, tile_note);
}

void CompilerTask::pipeline_helper_emit_no_matching_palette_error(
    std::size_t tile_index, const std::vector<PaletteMatchResult<Rgba32>> &matches)
{
    constexpr auto tag = "no-matching-palette";
    auto [metatile_index, layer, subtile] = metatile::from_tile_index(tile_index);

    // Emit error
    std::vector<std::string> no_match_err{};
    no_match_err.emplace_back(format_.format(
        "{}: no matching palette found",
        FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
    no_match_err.append_range(tile_printer_.print_metatile_tile_highlight(
        porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_));
    diag_.error(tag, no_match_err);

    // Emit a long note showing the top N closest matches
    std::vector<std::string> closest_n_note{};
    closest_n_note.emplace_back("closest N match(es) with covered colors highlighted:");
    int match_index = 0;
    for (const auto &match : matches) {
        if (match_index != 0) {
            // Add a blank line between subsequent matches
            closest_n_note.emplace_back();
        }
        closest_n_note.push_back(format_.format(
            "Palette match candidate: {}", FormatParam{palette_filename(match.palette_index), Style::bold}));
        closest_n_note.append_range(palette_printer_.print_rgba_palette_covered_missing(
            new_porymap_palettes_.at(match.palette_index), match.covered_colors, match.missing_colors));
        closest_n_note.emplace_back();
        closest_n_note.push_back(format_.format(
            "Uncovered pixels with {}:", FormatParam{palette_filename(match.palette_index), Style::bold}));
        closest_n_note.append_range(tile_printer_.print_metatile_pixel_highlights(
            porytiles_metatiles_.at(metatile_index),
            layer,
            subtile,
            match.uncovered_pixel_indices,
            extrinsic_transparency_));
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
    tile_limit_error.append_range(tile_printer_.print_metatile_tile_highlight(
        porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_));
    diag_.error(tag, tile_limit_error);

    // Construct note text
    std::vector<std::string> note_text;
    if (is_secondary()) {
        note_text.append_range(
            build_subtraction_limit_lines(format_, "Tile limit", tile_limit, num_tiles_total_, num_tiles_in_primary_));
    }
    else {
        note_text.push_back(
            format_.format("Tile limit is '{}' due to configuration.", FormatParam{tile_limit, Style::bold}));
        note_text.emplace_back();
        note_text.append_range(format_config_note(format_, num_tiles_in_primary_));
    }
    diag_.error_note(tag, note_text);
}

} // namespace

namespace porytiles {

ChainableResult<std::unique_ptr<Tileset>>
TilesetCompiler::compile(const Tileset &tileset, bool is_secondary, const Tileset *paired_primary) const
{
    CompilerTask task{
        tileset, is_secondary, paired_primary, *format_, *diag_, *tile_printer_, *palette_printer_, *config_, *schema_};
    return task.run();
}

} // namespace porytiles
