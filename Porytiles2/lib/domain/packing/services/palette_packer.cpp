#include "porytiles2/domain/packing/services/palette_packer.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/algorithms/shape_group_analyzer.hpp"
#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/packing/algorithms/indirect_link_builder.hpp"
#include "porytiles2/domain/packing/algorithms/palette_builder.hpp"
#include "porytiles2/domain/packing/models/palette_pool.hpp"
#include "porytiles2/domain/packing/models/shape_group_metadata.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Result of extracting colors from a palette, tracking both unique colors and occupied slots.
 */
struct ColorSetWithOccupancy {
    ColorSet color_set;
    std::size_t occupied_slots{};
};

/**
 * @brief Converts a \link PixelTile PixelTile's\endlink colors to a ColorSet.
 *
 * @details
 * Extracts unique non-transparent colors from the tile and looks up each color in the ColorIndexMap to build a ColorSet
 * bitset.
 *
 * @param tile The pixel tile to convert
 * @param color_map The color-to-index mapping
 * @param extrinsic The extrinsic transparency color
 * @pre All colors in the tile are present in color_map
 * @return ColorSet containing all non-transparent tile colors
 */
[[nodiscard]] ColorSet build_color_set_from_tile(
    const PixelTile<Rgba32> &tile, const ColorIndexMap<Rgba32> &color_map, const Rgba32 &extrinsic)
{
    ColorSet color_set{};
    const auto unique_colors = tile.unique_nontransparent_colors(extrinsic);

    for (const auto &color : unique_colors) {
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            /*
             * This will throw if a hint contains the extrinsic transparency color, since the ColorIndexMap won't
             * contain any transparency colors. Callers of the packer service should have used the PaletteValidator
             * service to validate input palettes and generate good user diagnostics.
             */
            panic("tile color " + to_string(color) + " not found in color index map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/**
 * @brief Converts a fixed-size palette's non-wildcard colors to a ColorSet.
 *
 * @details
 * Extracts colors from slots 1-15 (skipping slot 0 which is transparency) and looks up each in the ColorIndexMap.
 * Wildcards are skipped. Returns both the ColorSet of unique colors and the count of occupied slots.
 *
 * @param pal The palette to convert
 * @param color_map The color-to-index mapping
 * @pre All colors in the pal are present in color_map
 * @return ColorSetWithOccupancy containing unique colors and occupied slot count
 */
[[nodiscard]] ColorSetWithOccupancy
build_color_set_from_pal(const Palette<Rgba32, pal::max_size> &pal, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};
    std::size_t occupied_slots = 0;

    // Start from slot 1 (slot 0 is transparency)
    for (std::size_t i = 1; i < pal.size(); ++i) {
        if (pal.is_wildcard(i)) {
            continue;
        }
        ++occupied_slots;
        const auto color = pal.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            /*
             * This will throw if a pal contains the extrinsic transparency color, since the ColorIndexMap won't
             * contain any transparency colors. Callers of the packer service should have used the PaletteValidator
             * service to validate input palettes and generate good user diagnostics.
             */
            panic("pal color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return ColorSetWithOccupancy{color_set, occupied_slots};
}

/**
 * @brief Converts a hint palette's colors to a ColorSet.
 *
 * @details
 * Extracts colors from the hint palette and looks up each in the ColorIndexMap. Wildcards are skipped.
 *
 * @param pal The dynamic palette to convert
 * @param color_map The color-to-index mapping
 * @pre All colors in the pal are present in color_map
 * @pre Palette contains no wildcards
 * @return ColorSet containing all non-transparent pal colors
 */
[[nodiscard]] ColorSet build_color_set_from_hint_pal(const Palette<Rgba32> &pal, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};

    for (std::size_t i = 0; i < pal.size(); ++i) {
        if (pal.is_wildcard(i)) {
            panic("build_color_set_from_hint_palette palette contained unexpected wildcard");
        }
        const auto color = pal.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            panic("hint color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/**
 * @brief Builds a set of canonical key frame tiles from animation data.
 *
 * @details
 * Mirrors the representative frame selection logic in @c AnimTileMatcher::register_animation(): if the animation has a
 * key frame, use it; otherwise fall back to the first regular frame. Each non-transparent tile is canonicalized and
 * added to the result set. This set is used to exclude animation key frame tiles from shape group analysis, since
 * @c AnimTileMatcher::find_match() redirects these tiles to pre-placed animation tile indices during tile assignment,
 * making any sharing alignment computed for them illusory.
 *
 * @param anims The animation map from PackingParams
 * @param extrinsic The extrinsic transparency color
 * @return Set of canonical key frame tiles (as PixelTile<Rgba32>)
 */
[[nodiscard]] std::set<PixelTile<Rgba32>>
build_anim_keyframe_set(const std::map<std::string, Animation<Rgba32>> &anims, const Rgba32 &extrinsic)
{
    std::set<PixelTile<Rgba32>> result;
    for (const auto &[name, anim] : anims) {
        if (anim.frames().empty()) {
            continue;
        }
        const AnimFrame<Rgba32> &representative_frame =
            anim.has_key_frame() ? anim.key_frame() : anim.frames().begin()->second;
        for (const auto &tile : representative_frame.tiles()) {
            if (tile.is_transparent(extrinsic)) {
                continue;
            }
            CanonicalPixelTile<Rgba32> canonical{tile};
            result.insert(static_cast<const PixelTile<Rgba32> &>(canonical));
        }
    }
    return result;
}

/**
 * @brief Builds a combined tile vector of regular tiles and a parallel ID mapping.
 *
 * @details
 * The combined vector is used as input to @c analyze_shape_groups(). The parallel @c combined_index_to_id vector maps
 * each combined index back to its PackableTile::RegularId, allowing shape group membership to be expressed in terms of
 * PackableTile IDs. Anim tiles are excluded because they occupy DMA-overwritten VRAM slots and cannot share with
 * regular tiles. Additionally, tiles whose canonical form matches an animation key frame tile are excluded, since
 * @c AnimTileMatcher will redirect them to animation tile indices during tile assignment.
 */
struct CombinedTiles {
    std::vector<PixelTile<Rgba32>> tiles;
    std::vector<PackableTile::Id> index_to_id;
};

[[nodiscard]] CombinedTiles
build_combined_tiles(const PackingParams &params, const std::set<PixelTile<Rgba32>> &anim_keyframe_tiles)
{
    CombinedTiles combined;
    combined.tiles.reserve(params.tiles_.size());
    combined.index_to_id.reserve(params.tiles_.size());

    for (std::size_t i = 0; i < params.tiles_.size(); ++i) {
        CanonicalPixelTile<Rgba32> canonical{params.tiles_.at(i)};
        if (anim_keyframe_tiles.contains(static_cast<const PixelTile<Rgba32> &>(canonical))) {
            continue;
        }
        combined.tiles.push_back(params.tiles_.at(i));
        combined.index_to_id.push_back(PackableTile::RegularId{i});
    }

    return combined;
}

/**
 * @brief Describes a single member of a sharing group for diagnostic purposes.
 */
struct SharingGroupMember {
    std::size_t tile_index;
    std::size_t pal_index;
};

/**
 * @brief Describes a shape group whose members landed in distinct palettes after packing.
 *
 * @details
 * Used locally within pack_tiles() for Phase 2/3/4 diagnostic emission and summary computation.
 */
struct PartitionGroup {
    std::vector<SharingGroupMember> members;
    std::vector<std::size_t> color_version_tile_indices;
};

/**
 * @brief Tracks per-group statistics for partially aligned sharing groups.
 *
 * @details
 * Populated during Phase 3 verification when a sharing group has some members that aligned with the reference indexed
 * tile and others that diverged. Used to emit summary diagnostics showing the full/partial breakdown.
 */
struct PartialAlignmentInfo {
    std::size_t group_id;
    std::size_t verified_count;
    std::size_t matching_count;
    std::size_t dropped_color_version_count;
};

} // namespace

namespace porytiles2 {

ChainableResult<PalettePacking> PalettePacker::pack_tiles(const PackingParams &params) const
{
    // === STEP 1: Convert regular tiles and anims to PackableTile vector ===
    std::vector<PackableTile> regular_tiles;
    regular_tiles.reserve(params.tiles_.size());
    for (std::size_t i = 0; i < params.tiles_.size(); ++i) {
        auto color_set =
            build_color_set_from_tile(params.tiles_.at(i), params.color_map_, params.extrinsic_transparency_);
        regular_tiles.emplace_back(PackableTile::RegularId{i}, color_set);
    }
    for (const auto &[anim_name, anim] : params.anims_) {
        const auto &composite_frame = anim.composite_frame(params.extrinsic_transparency_);
        for (std::size_t subtile_index = 0; subtile_index < composite_frame.tiles().size(); ++subtile_index) {
            const auto &composite_tile = composite_frame.tiles().at(subtile_index);
            auto color_set =
                build_color_set_from_tile(composite_tile, params.color_map_, params.extrinsic_transparency_);
            regular_tiles.emplace_back(PackableTile::AnimId{anim_name, subtile_index}, color_set);
        }
    }

    // === STEP 2: Convert hints to PackableTile vector ===
    std::vector<PackableTile> hint_tiles;
    hint_tiles.reserve(params.hints_.size());
    for (const PaletteHint &hint : params.hints_) {
        auto color_set = build_color_set_from_hint_pal(hint.pal(), params.color_map_);
        hint_tiles.emplace_back(PackableTile::HintId{hint.name()}, color_set);
    }

    // === STEP 3: Convert input prefilled palettes to PrefilledPalette set ===
    std::set<PrefilledPalette> prefilled_palettes;
    for (std::size_t i = 0; i < params.prefilled_pals_.size(); ++i) {
        if (!params.prefilled_pals_[i].has_value()) {
            continue;
        }
        auto [color_set, occupied_slots] =
            build_color_set_from_pal(params.prefilled_pals_[i].value(), params.color_map_);
        prefilled_palettes.insert(PrefilledPalette::partially_locked(i, color_set, occupied_slots));
    }

    // === STEP 4: Create PackingInput and call low-level pack() ===
    PackingInput packing_input{
        std::move(regular_tiles),
        std::move(hint_tiles),
        std::move(prefilled_palettes),
        PalettePool{params.available_pals_}};

    // Build animation keyframe exclusion set (once, before any shape group analysis)
    const auto anim_keyframe_tiles = build_anim_keyframe_set(params.anims_, params.extrinsic_transparency_);

    if (params.tile_sharing_packing_ == TileSharingPacking::optimal) {
        panic("Tile sharing packing 'optimal' is not yet implemented.");
    }
    if (params.tile_sharing_alignment_ == TileSharingAlignment::optimal) {
        panic("Tile sharing alignment 'optimal' is not yet implemented.");
    }

    /*
     * For biased packing, build shape group metadata BEFORE the initial pack call so the strategy
     * receives sharing-aware input. For off packing, pack normally first (metadata is only needed
     * post-packing for diagnostics and alignment).
     */
    std::vector<ShapeGroup<Rgba32>> shape_groups_for_biased;
    CombinedTiles combined_for_biased;
    if (params.tile_sharing_packing_ == TileSharingPacking::biased) {
        combined_for_biased = build_combined_tiles(params, anim_keyframe_tiles);
        shape_groups_for_biased = analyze_shape_groups(combined_for_biased.tiles, params.extrinsic_transparency_);

        if (!shape_groups_for_biased.empty()) {
            auto metadata = build_shape_group_metadata(shape_groups_for_biased, combined_for_biased.index_to_id);
            packing_input.shape_group_metadata_ = std::move(metadata);
        }
    }

    auto pack_result = strategy_->pack(packing_input);

    PT_TRY_ASSIGN_CHAIN_ERR(
        packing_output, std::move(pack_result), "Low-level palette packing failed.", PalettePacking);

    // === STEP 5: Convert PackingOutput back to PalettePacking ===
    PalettePacking packing{};

    // Helper to populate tile_to_pal from a PackingOutput
    auto populate_tile_to_pal = [&packing](const PackingOutput &output) {
        packing.tile_to_pal_.clear();
        for (const auto &[tile_id, pal_index] : output.tile_to_pal_) {
            std::visit(
                [&packing, pal_index]<typename IdVariant>(IdVariant &&id) {
                    using Id = std::decay_t<IdVariant>;
                    if constexpr (std::is_same_v<Id, PackableTile::RegularId>) {
                        packing.tile_to_pal_[id.index] = pal_index;
                    }
                    else if constexpr (std::is_same_v<Id, PackableTile::AnimId>) {
                        // Anim tiles are packed for palette assignment but excluded from tile_to_pal_
                    }
                    else if constexpr (std::is_same_v<Id, PackableTile::HintId>) {
                        // We don't currently care to store where hints got assigned
                    }
                    else if constexpr (std::is_same_v<Id, PackableTile::PrefilledPaletteId>) {
                        // Nothing to do here, we only had these PackableTiles for internal bookkeeping
                    }
                    else {
                        panic("unimplemented std::visit branch");
                    }
                },
                tile_id);
        }
    };

    // 5a: build the tile_to_pal maps for PalettePacking
    populate_tile_to_pal(packing_output);

    /*
     * 5b: Always compute shape groups for three-phase diagnostics, then conditionally build Indirect links.
     *
     * Shape group analysis and Phase 1/2 data are always computed regardless of config, so diagnostics can be
     * emitted with appropriate caveats. Indirect link generation (alignment) is conditioned on
     * tile_sharing_alignment_.
     *
     * The Indirect approach:
     *  - Step 1: Build base palettes (sequential fill, no links) — needed for slot mapping in reference selection
     *  - Step 2: Build tile_pal_assignments from authoritative packing assignments
     *  - Step 3: Generate Indirect links (color-to-color references, not absolute slot targets)
     *  - Step 4: Build final palettes with Indirect links applied
     *  - Step 5: Build sharing diagnostics against final palettes (verified alignment)
     */
    std::vector<IndirectLink> indirect_links;

    /*
     * Always compute shape groups for three-phase diagnostics. Reuse pre-pack results when packing is
     * biased (they were already computed for metadata injection).
     */
    CombinedTiles combined;
    std::vector<ShapeGroup<Rgba32>> shape_groups;

    if (params.tile_sharing_packing_ == TileSharingPacking::biased) {
        combined = std::move(combined_for_biased);
        shape_groups = std::move(shape_groups_for_biased);
    }
    else {
        combined = build_combined_tiles(params, anim_keyframe_tiles);
        shape_groups = analyze_shape_groups(combined.tiles, params.extrinsic_transparency_);
    }

    // Phase 1: Detect sharing opportunities and emit diagnostics (always, regardless of config)
    for (std::size_t shape_group_index = 0; shape_group_index < shape_groups.size(); ++shape_group_index) {
        const auto &shape_group = shape_groups.at(shape_group_index);
        std::vector<std::size_t> color_version_tile_indices;
        std::vector<std::size_t> all_tile_indices;
        std::set<std::map<ShapeMask, Rgba32>> seen_colors;
        for (const auto &member : shape_group.members) {
            auto regular_index = std::get<PackableTile::RegularId>(combined.index_to_id.at(member.tile_index)).index;
            all_tile_indices.push_back(regular_index);
            if (!seen_colors.contains(member.colors)) {
                seen_colors.insert(member.colors);
                color_version_tile_indices.push_back(regular_index);
            }
        }

        // Emit Phase 1 diagnostic
        auto phase1_tag = std::format("tile-sharing-shareable-tiles-{}", shape_group_index);

        std::vector<std::string> remark_lines;
        remark_lines.emplace_back(format_->format(
            "Detected sharing opportunity (group id '{}') with '{}' color version(s) across '{}' tilemap entries.",
            FormatParam{shape_group_index, Style::bold},
            FormatParam{color_version_tile_indices.size(), Style::bold},
            FormatParam{all_tile_indices.size(), Style::bold}));
        std::ranges::copy(
            build_tile_sharing_color_version_tile_lines(
                *format_, *tile_printer_, params.tiles_, params.extrinsic_transparency_, color_version_tile_indices),
            std::back_inserter(remark_lines));
        remark_lines.emplace_back(
            format_->format("All tilemap entries ('{}' total):", FormatParam{all_tile_indices.size(), Style::bold}));
        std::ranges::copy(build_truncated_tile_ref_lines(*format_, all_tile_indices), std::back_inserter(remark_lines));
        diag_->remark(phase1_tag, remark_lines);
    }

    // Phase 1 summary
    diag_->remark(
        "tile-sharing-shareable-tiles-summary",
        "Tile sharing detection: '{}' shareable shape group(s) found.",
        FormatParam{shape_groups.size(), Style::bold});

    // Phase 2: Compute packing partition groups and emit diagnostics (always, regardless of config)
    // Use index-based loop to track which shape_groups indices become eligible partition groups
    std::set<std::size_t> eligible_shape_indices;
    std::vector<PartitionGroup> partition_groups;
    std::map<std::size_t, std::size_t> shape_to_partition_index;
    for (std::size_t sg_index = 0; sg_index < shape_groups.size(); ++sg_index) {
        const auto &group = shape_groups.at(sg_index);
        std::set<std::size_t> distinct_pals_set;
        std::vector<SharingGroupMember> members;
        std::set<std::map<ShapeMask, Rgba32>> seen_colors;
        std::vector<std::size_t> color_version_tile_indices;

        for (const auto &member : group.members) {
            auto regular_index = std::get<PackableTile::RegularId>(combined.index_to_id.at(member.tile_index)).index;
            if (packing.tile_to_pal_.contains(regular_index)) {
                auto pal_index = packing.tile_to_pal_.at(regular_index);
                members.push_back(SharingGroupMember{regular_index, pal_index});
                distinct_pals_set.insert(pal_index);
                if (!seen_colors.contains(member.colors)) {
                    seen_colors.insert(member.colors);
                    color_version_tile_indices.push_back(regular_index);
                }
            }
        }

        if (members.size() >= 2 && distinct_pals_set.size() >= 2) {
            eligible_shape_indices.insert(sg_index);

            // Emit Phase 2 diagnostic
            auto phase2_tag = std::format("tile-sharing-palette-partition-{}", sg_index);

            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(format_->format(
                "After palette packing (group id '{}'), '{}' color versions across '{}' tilemap entries eligible for "
                "sharing.",
                FormatParam{sg_index, Style::bold},
                FormatParam{color_version_tile_indices.size(), Style::bold},
                FormatParam{members.size(), Style::bold}));

            std::map<std::size_t, std::vector<std::size_t>> members_by_pal;
            for (const auto &member : members) {
                members_by_pal[member.pal_index].push_back(member.tile_index);
            }

            std::ranges::copy(
                build_tile_sharing_color_version_tile_lines(
                    *format_,
                    *tile_printer_,
                    params.tiles_,
                    params.extrinsic_transparency_,
                    color_version_tile_indices),
                std::back_inserter(remark_lines));
            std::ranges::copy(
                build_per_palette_tile_ref_lines(*format_, members_by_pal), std::back_inserter(remark_lines));

            if (params.tile_sharing_packing_ == TileSharingPacking::off) {
                remark_lines.emplace_back("");
                remark_lines.emplace_back("--------");
                remark_lines.emplace_back("");
                remark_lines.emplace_back("Caveat: the palette partition shown above may be coincidental.");
                remark_lines.emplace_back(format_->format(
                    "Packing '{}' does not actively separate shape-group siblings across palettes.",
                    FormatParam{to_string(params.tile_sharing_packing_.value()), Style::bold}));
                remark_lines.emplace_back("");
                std::ranges::copy(
                    format_config_note(*format_, params.tile_sharing_packing_), std::back_inserter(remark_lines));
            }
            diag_->remark(phase2_tag, remark_lines);

            PartitionGroup partition;
            partition.members = std::move(members);
            partition.color_version_tile_indices = std::move(color_version_tile_indices);
            shape_to_partition_index[sg_index] = partition_groups.size();
            partition_groups.push_back(std::move(partition));
        }
    }

    // Phase 2 summary
    diag_->remark(
        "tile-sharing-palette-partition-summary",
        "Tile sharing partition: '{}' of '{}' shape group(s) eligible for sharing after palette packing.",
        FormatParam{eligible_shape_indices.size(), Style::bold},
        FormatParam{shape_groups.size(), Style::bold});

    /*
     * Build combined tile index → hw palette index map from the authoritative packing assignments. This map is
     * used by both the link builder and the sharing diagnostics verifier, ensuring consistent palette assignments
     * throughout the pipeline. We use packing assignments (tile_to_pal_) rather than re-matching via
     * match_or_best, because match_or_best can return a different palette than the packer assigned when multiple
     * palettes cover the same tile's colors.
     */
    std::map<std::size_t, std::size_t> tile_pal_assignments;
    if (!shape_groups.empty()) {
        for (const auto &group : shape_groups) {
            for (const auto &member : group.members) {
                const auto &id = combined.index_to_id.at(member.tile_index);
                if (std::holds_alternative<PackableTile::RegularId>(id)) {
                    auto regular_index = std::get<PackableTile::RegularId>(id).index;
                    if (packing.tile_to_pal_.contains(regular_index)) {
                        tile_pal_assignments[member.tile_index] = packing.tile_to_pal_.at(regular_index);
                    }
                }
            }
        }
    }

    // Conditionally build Indirect links based on alignment
    if (params.tile_sharing_alignment_ == TileSharingAlignment::greedy && !shape_groups.empty()) {
        // Step 1: Build base palettes (sequential fill, no links) — needed for slot mapping in reference selection
        auto base_pals = build_all_output_palettes(
            packing_output.pals_,
            params.prefilled_pals_,
            params.color_map_,
            params.extrinsic_transparency_,
            {} /* empty links */);

        // Step 2: Generate Indirect links using authoritative palette assignments
        indirect_links = build_indirect_links(shape_groups, tile_pal_assignments, base_pals, params.prefilled_pals_);
    }

    // Build final output palettes with Indirect links applied (or empty links for off mode)
    AlignmentFailureCounts failure_counts{};
    auto final_pals = build_all_output_palettes(
        packing_output.pals_,
        params.prefilled_pals_,
        params.color_map_,
        params.extrinsic_transparency_,
        indirect_links,
        !indirect_links.empty() ? &failure_counts : nullptr);

    // Phase 3: Verify sharing alignment against FINAL palettes and emit diagnostics
    struct VerifiedMember {
        std::size_t regular_index;
        std::size_t hw_pal;
        CanonicalPixelTile<IndexPixel> canonical_indexed;
        std::map<ShapeMask, Rgba32> colors;
    };

    std::size_t aligned_count = 0;
    std::size_t fully_aligned_count = 0;
    std::size_t partially_aligned_count = 0;
    std::vector<PartialAlignmentInfo> partial_alignment_infos;
    std::vector<std::pair<std::size_t, PartitionGroup>> unaligned_partition_groups;
    if (!shape_groups.empty()) {
        std::set<std::size_t> aligned_shape_indices;

        for (std::size_t sg_index = 0; sg_index < shape_groups.size(); ++sg_index) {
            if (!eligible_shape_indices.contains(sg_index)) {
                continue;
            }
            const auto &group = shape_groups.at(sg_index);
            std::vector<VerifiedMember> verified;

            for (const auto &member : group.members) {
                if (!tile_pal_assignments.contains(member.tile_index)) {
                    continue;
                }
                const auto &id = combined.index_to_id.at(member.tile_index);
                const auto regular_index = std::get<PackableTile::RegularId>(id).index;
                std::size_t hw = tile_pal_assignments.at(member.tile_index);

                /*
                 * Verify that the tile actually produces the same indexed form when indexed against its
                 * assigned final palette. If indirect link resolution failed silently, colors may end up at
                 * different slot indices, producing different indexed tiles that won't deduplicate.
                 */
                const auto &tile = combined.tiles.at(member.tile_index);
                auto indexed_tile =
                    index_tile_from_color_tile(tile, final_pals.at(hw).value(), params.extrinsic_transparency_);
                CanonicalPixelTile<IndexPixel> canonical{indexed_tile};
                verified.push_back(VerifiedMember{regular_index, hw, std::move(canonical), member.colors});
            }

            if (verified.size() < 2) {
                continue;
            }

            /*
             * Use the first member's canonical indexed tile as reference. Only include members whose
             * canonical indexed tile matches. This filters out members where indirect link resolution
             * failed, causing their palette slot assignments to diverge.
             */
            const auto &reference = verified.at(0).canonical_indexed;
            std::vector<SharingGroupMember> result_members;
            std::set<std::size_t> distinct_pals_set;

            std::set<std::map<ShapeMask, Rgba32>> dropped_color_versions;
            for (const auto &member : verified) {
                const auto &member_as_tile = static_cast<const PixelTile<IndexPixel> &>(member.canonical_indexed);
                const auto &ref_as_tile = static_cast<const PixelTile<IndexPixel> &>(reference);
                if (member_as_tile != ref_as_tile) {
                    dropped_color_versions.insert(member.colors);
                    continue;
                }
                result_members.push_back(SharingGroupMember{member.regular_index, member.hw_pal});
                distinct_pals_set.insert(member.hw_pal);
            }
            const std::size_t dropped_color_version_count = dropped_color_versions.size();

            if (result_members.size() >= 2 && distinct_pals_set.size() >= 2) {
                aligned_shape_indices.insert(sg_index);

                const bool is_partial = dropped_color_version_count > 0;
                if (is_partial) {
                    ++partially_aligned_count;
                    partial_alignment_infos.push_back(
                        PartialAlignmentInfo{
                            sg_index, verified.size(), result_members.size(), dropped_color_version_count});
                }
                else {
                    ++fully_aligned_count;
                }

                // Emit Phase 3 diagnostic
                auto phase3_tag = std::format("tile-sharing-result-{}", sg_index);

                std::map<std::size_t, std::vector<std::size_t>> members_by_pal;
                for (const auto &member : result_members) {
                    members_by_pal[member.pal_index].push_back(member.tile_index);
                }

                std::vector<std::string> remark_lines;
                if (is_partial) {
                    remark_lines.emplace_back(format_->format(
                        "Tile sharing partially succeeded (group id '{}'): '{}' participating palette(s) (referenced "
                        "in '{}' tilemap entries).",
                        FormatParam{sg_index, Style::bold},
                        FormatParam{members_by_pal.size(), Style::bold},
                        FormatParam{result_members.size(), Style::bold}));
                    remark_lines.emplace_back(format_->format(
                        "'{}' color version(s) dropped due to alignment divergence.",
                        FormatParam{dropped_color_version_count, Style::bold}));
                }
                else {
                    remark_lines.emplace_back(format_->format(
                        "Tile sharing succeeded (group id '{}'): '{}' participating palette(s) (referenced in '{}' "
                        "tilemap entries).",
                        FormatParam{sg_index, Style::bold},
                        FormatParam{members_by_pal.size(), Style::bold},
                        FormatParam{result_members.size(), Style::bold}));
                }
                std::ranges::copy(
                    build_representative_tile_per_palette_lines(
                        *format_, *tile_printer_, params.tiles_, params.extrinsic_transparency_, members_by_pal),
                    std::back_inserter(remark_lines));
                std::ranges::copy(
                    build_per_palette_tile_ref_lines(*format_, members_by_pal), std::back_inserter(remark_lines));

                if (is_partial) {
                    remark_lines.emplace_back("");
                    remark_lines.emplace_back("--------");
                    remark_lines.emplace_back("");
                    remark_lines.emplace_back(format_->format(
                        "'{}' color version(s) (of '{}' verified members) diverged from the reference and were "
                        "dropped.",
                        FormatParam{dropped_color_version_count, Style::bold},
                        FormatParam{verified.size(), Style::bold}));
                }

                if (params.tile_sharing_alignment_ == TileSharingAlignment::off) {
                    remark_lines.emplace_back("");
                    remark_lines.emplace_back("--------");
                    remark_lines.emplace_back("");
                    remark_lines.emplace_back("Caveat: the slot alignment shown above may be coincidental.");
                    remark_lines.emplace_back(format_->format(
                        "Alignment '{}' does not actively align palette slots for tile sharing.",
                        FormatParam{to_string(params.tile_sharing_alignment_.value()), Style::bold}));
                    remark_lines.emplace_back("");
                    std::ranges::copy(
                        format_config_note(*format_, params.tile_sharing_alignment_), std::back_inserter(remark_lines));
                }
                diag_->remark(phase3_tag, remark_lines);
            }
        }

        aligned_count = aligned_shape_indices.size();

        // Compute unaligned partition groups: eligible groups that did not produce a sharing result
        for (const auto sg_index : eligible_shape_indices) {
            if (!aligned_shape_indices.contains(sg_index)) {
                unaligned_partition_groups.emplace_back(
                    sg_index, partition_groups.at(shape_to_partition_index.at(sg_index)));
            }
        }
    }

    // Tile sharing summary: emit when biased packing or greedy alignment is active
    if (params.tile_sharing_packing_ == TileSharingPacking::biased ||
        params.tile_sharing_alignment_ == TileSharingAlignment::greedy) {
        const auto eligible = partition_groups.size();
        const auto &fc = failure_counts;

        constexpr auto tag = "tile-sharing-result-summary";

        std::vector<std::string> remark_lines;
        if (partially_aligned_count > 0) {
            remark_lines.emplace_back("Tile sharing summary:");
            remark_lines.emplace_back(format_->format(
                "'{}' detected → '{}' eligible after packing → '{}' aligned ('{}' fully, '{}' "
                "partially).",
                FormatParam{shape_groups.size(), Style::bold},
                FormatParam{eligible, Style::bold},
                FormatParam{aligned_count, Style::bold},
                FormatParam{fully_aligned_count, Style::bold},
                FormatParam{partially_aligned_count, Style::bold}));
        }
        else {
            remark_lines.emplace_back("Tile sharing summary:");
            remark_lines.emplace_back(format_->format(
                "'{}' detected → '{}' eligible after packing → '{}' aligned.",
                FormatParam{shape_groups.size(), Style::bold},
                FormatParam{eligible, Style::bold},
                FormatParam{aligned_count, Style::bold}));
        }

        // Partially aligned groups listing
        if (partially_aligned_count > 0) {
            remark_lines.emplace_back("");
            remark_lines.emplace_back("Partially aligned groups:");
            for (const auto &info : partial_alignment_infos) {
                remark_lines.emplace_back(format_->format(
                    "  Group '{}': '{}' of '{}' verified member(s) aligned, '{}' color version(s) dropped.",
                    FormatParam{info.group_id, Style::bold},
                    FormatParam{info.matching_count, Style::bold},
                    FormatParam{info.verified_count, Style::bold},
                    FormatParam{info.dropped_color_version_count, Style::bold}));
            }
            remark_lines.emplace_back("");
        }

        const auto unaligned = eligible - aligned_count;
        if (unaligned > 0) {
            if (fc.total() == 0 && params.tile_sharing_alignment_ != TileSharingAlignment::off) {
                /*
                 * In rare cases, the summary may report unaligned groups but the failure breakdown shows zero counts
                 * across all categories. This is nonsensical, so we panic.
                 *
                 * Previously, the most common cause of this was prefilled source conflicts — source colors locked in
                 * their palettes that prevented link application. These are now tracked and reported as Prefilled
                 * Source Conflicts in the failure breakdown.
                 *
                 * If you still encounter zero-failure divergence, it may indicate an unidentified edge case. Please
                 * report it so we can investigate.
                 */
                panic("Hit 'Divergence with Zero Failures' case");
            }

            remark_lines.emplace_back(
                format_->format("'{}' eligible group(s) could not be aligned:", FormatParam{unaligned, Style::bold}));

            for (const auto &[group_id, group] : unaligned_partition_groups) {
                remark_lines.emplace_back();
                remark_lines.emplace_back(format_->format(
                    "Unaligned group (group id '{}') — '{}' color version(s):",
                    FormatParam{group_id, Style::bold},
                    FormatParam{group.color_version_tile_indices.size(), Style::bold}));
                std::ranges::copy(
                    build_tile_sharing_color_version_tile_lines(
                        *format_,
                        *tile_printer_,
                        params.tiles_,
                        params.extrinsic_transparency_,
                        group.color_version_tile_indices),
                    std::back_inserter(remark_lines));

                // Show palette assignments for each color version
                for (std::size_t v = 0; v < group.color_version_tile_indices.size(); ++v) {
                    const auto cv_tile_index = group.color_version_tile_indices.at(v);
                    std::set<std::size_t> pals_for_version;
                    for (const auto &member : group.members) {
                        if (member.tile_index == cv_tile_index) {
                            pals_for_version.insert(member.pal_index);
                        }
                    }
                    if (pals_for_version.empty()) {
                        continue;
                    }
                    std::string pal_list;
                    for (const auto pal : pals_for_version) {
                        if (!pal_list.empty()) {
                            pal_list += ", ";
                        }
                        pal_list += pal_filename(pal);
                    }
                    remark_lines.emplace_back(format_->format(
                        "  Version '{}' assigned to palette(s): '{}'.",
                        FormatParam{v + 1, Style::bold},
                        FormatParam{pal_list, Style::bold}));
                }

                // Show per-group failure details inline (only when alignment is active)
                if (params.tile_sharing_alignment_ != TileSharingAlignment::off) {
                    const auto group_broken = std::ranges::count_if(
                        fc.broken_chain_details, [&](const auto &d) { return d.source_group_index == group_id; });
                    const auto group_prefilled_dest =
                        std::ranges::count_if(fc.prefilled_destination_conflict_details, [&](const auto &d) {
                            return d.source_group_index == group_id;
                        });
                    const auto group_prefilled_src =
                        std::ranges::count_if(fc.prefilled_source_conflict_details, [&](const auto &d) {
                            return d.source_group_index == group_id;
                        });
                    const auto group_no_free = std::ranges::count_if(
                        fc.no_free_slot_details, [&](const auto &d) { return d.source_group_index == group_id; });
                    const auto group_fww = std::ranges::count_if(
                        fc.first_writer_wins_details, [&](const auto &d) { return d.source_group_index == group_id; });
                    const auto group_total =
                        group_broken + group_prefilled_dest + group_prefilled_src + group_no_free + group_fww;

                    if (group_total > 0) {
                        remark_lines.emplace_back(format_->format(
                            "  '{}' link failure(s) for this group:", FormatParam{group_total, Style::bold}));
                        if (group_broken > 0) {
                            remark_lines.emplace_back(
                                format_->format("    Broken chain: '{}'.", FormatParam{group_broken, Style::bold}));
                        }
                        if (group_prefilled_dest > 0) {
                            remark_lines.emplace_back(format_->format(
                                "    Prefilled destination conflict: '{}'.",
                                FormatParam{group_prefilled_dest, Style::bold}));
                            for (const auto &detail : fc.prefilled_destination_conflict_details) {
                                if (detail.source_group_index != group_id) {
                                    continue;
                                }
                                remark_lines.emplace_back(format_->format(
                                    "      Palette '{}' slot '{}': color '{}' blocked by locked color '{}'.",
                                    FormatParam{pal_filename(detail.palette_index), Style::bold},
                                    FormatParam{detail.target_slot, Style::bold},
                                    FormatParam{detail.blocked_color, Style::bold},
                                    FormatParam{detail.locked_color, Style::bold}));
                            }
                        }
                        if (group_no_free > 0) {
                            remark_lines.emplace_back(format_->format(
                                "    No free slot for eviction: '{}'.", FormatParam{group_no_free, Style::bold}));
                        }
                        if (group_fww > 0) {
                            remark_lines.emplace_back(format_->format(
                                "    Shared color conflict: '{}'.", FormatParam{group_fww, Style::bold}));
                        }
                        if (group_prefilled_src > 0) {
                            remark_lines.emplace_back(format_->format(
                                "    Prefilled source conflict: '{}'.", FormatParam{group_prefilled_src, Style::bold}));
                            for (const auto &detail : fc.prefilled_source_conflict_details) {
                                if (detail.source_group_index != group_id) {
                                    continue;
                                }
                                remark_lines.emplace_back(format_->format(
                                    "      Palette '{}': prefilled color '{}' could not be linked to color '{}' in "
                                    "palette '{}'.",
                                    FormatParam{pal_filename(detail.source_pal_index), Style::bold},
                                    FormatParam{detail.source_color, Style::bold},
                                    FormatParam{detail.ref_color, Style::bold},
                                    FormatParam{pal_filename(detail.ref_pal_index), Style::bold}));
                            }
                        }
                    }
                }
            }

            // Aggregate failure summary
            remark_lines.emplace_back();
            if (params.tile_sharing_alignment_ == TileSharingAlignment::off) {
                remark_lines.emplace_back("Alignment is 'off' — no slot alignment was attempted.");
                remark_lines.emplace_back("Switch to 'greedy' to enable palette slot alignment for eligible groups.");
            }
            else {
                remark_lines.emplace_back(format_->format(
                    "'{}' total link resolution failure(s) across all unaligned groups.",
                    FormatParam{fc.total(), Style::bold}));
            }

            // Actionable suggestions (skip for off mode — the suggestion is already above)
            remark_lines.emplace_back();
            if (partially_aligned_count > 0) {
                remark_lines.emplace_back(format_->format(
                    "Tile sharing is best-effort — '{}' of '{}' eligible group(s) aligned ('{}' fully, '{}' "
                    "partially).",
                    FormatParam{aligned_count, Style::bold},
                    FormatParam{eligible, Style::bold},
                    FormatParam{fully_aligned_count, Style::bold},
                    FormatParam{partially_aligned_count, Style::bold}));
            }
            else {
                remark_lines.emplace_back(format_->format(
                    "Tile sharing is best-effort — '{}' of '{}' aligned group(s) may be sufficient for your needs.",
                    FormatParam{aligned_count, Style::bold},
                    FormatParam{eligible, Style::bold}));
            }
            if (params.tile_sharing_alignment_ != TileSharingAlignment::off) {
                if (!fc.prefilled_destination_conflict_details.empty()) {
                    remark_lines.emplace_back(
                        "Adjusting the prefilled destination slots listed above may improve tile sharing.");
                }
                if (!fc.prefilled_source_conflict_details.empty()) {
                    remark_lines.emplace_back(
                        "Some link source colors are prefilled (locked) in their palettes and could not be "
                        "reassigned.");
                    remark_lines.emplace_back(
                        "Rearranging or wildcarding those prefilled slots may improve tile sharing.");
                }
                if (!fc.no_free_slot_details.empty()) {
                    remark_lines.emplace_back(format_->format(
                        "'{}' color(s) could not be placed due to full palettes. Reducing unique colors per metatile "
                        "may help.",
                        FormatParam{fc.no_free_slot_details.size(), Style::bold}));
                }
            }
            remark_lines.emplace_back("For details on tile sharing, see:");
            remark_lines.emplace_back("  https://grunt-lucas.github.io/porytiles-user-docs/tile-sharing.html");
        }

        diag_->remark(tag, remark_lines);
    }

    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        if (final_pals.at(i).has_value()) {
            packing.pals_.at(i) = final_pals.at(i);
        }
    }

    // Emit diagnostic remarks for packed palettes
    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        const auto &maybe_packed_pal = packing.pals_.at(i);
        if (maybe_packed_pal.has_value()) {
            constexpr auto pal_tag = "palette-packing-result";
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(
                format_->format("Palette '{}' packing result:", FormatParam{pal_filename(i), Style::bold}));
            remark_lines.emplace_back();
            std::ranges::copy(pal_printer_->print_rgba_pal(maybe_packed_pal.value()), std::back_inserter(remark_lines));
            diag_->remark(pal_tag, remark_lines);
        }
    }

    /*
     * TODO: above, we built output pals from all the pals that got explicitly packed. But what about pals that got
     * filled via Porytiles pal overrides? We need to think about this very carefully. We'll need to think through how
     * we populate the bitset for the PalettePool above.
     *
     * Here's what I think we should do. PalettePacker should only handle palettes that were explicitly enabled for
     * packing via PalettePool. Any palette not turned on in PalettePool will get sent back to the caller as a
     * std::nullopt. The caller can then decide what to do (e.g. fill in with Porytiles override, fill in with default
     * values, etc).
     *
     * Callers MUST explicitly enable all pals they want the packer to access. This means that if the caller is
     * compiling a secondary set, they should enable bits 0-5 in the available_pals_ bitset in PackingParams, and then
     * pass the packer the primary palettes via the prefilled_pals_ input param. The prefilled 0-5 will have no
     * wildcards since they are completely fixed by the primary. The calling code should also probably throw a warning
     * if the user specified Porytiles overrides for those pals since they will be ignored in favor of the palettes for
     * paired primary (maybe this can be configurable?). The calling code should also throw a warning when there are
     * Porytiles overrides that don't correspond to one of the pals enabled for packing. E.g. if we are compiling
     * primary, so bits 0-5 are set in the PalettePool, but user has supplied 12.pal in the Porytiles palettes, warn
     * user that they've supplied an out-of-band palette due to configuration. Then say that this palette will still be
     * copied over and all wildcards will receive default values (or allow user to specify an alternate behavior via
     * configuration).
     */

    return packing;
}

} // namespace porytiles2
