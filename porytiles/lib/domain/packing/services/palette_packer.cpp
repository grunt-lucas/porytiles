#include "porytiles/domain/packing/services/palette_packer.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "porytiles/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles/domain/algorithms/shape_group_analyzer.hpp"
#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/packing/algorithms/indirect_link_builder.hpp"
#include "porytiles/domain/packing/algorithms/palette_builder.hpp"
#include "porytiles/domain/packing/models/palette_pool.hpp"
#include "porytiles/domain/packing/models/shape_group_metadata.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles;

/// @brief Creates a FormatParam for an Rgba32 color displayed as "R, G, B" with bold true-color ANSI styling.
FormatParam color_param(const Rgba32 &color)
{
    auto text =
        std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue());
    return FormatParam{std::move(text), rgb_fg_style(color.red(), color.green(), color.blue()) | Style::bold};
}

/// @brief Result of extracting colors from a palette, tracking both unique colors and occupied slots.
struct ColorSetWithOccupancy {
    ColorSet color_set;
    std::size_t occupied_slots{};
};

/// @brief Converts a \link PixelTile PixelTile's\endlink colors to a ColorSet.
///
/// @details
/// Extracts unique non-transparent colors from the tile and looks up each color in the ColorIndexMap to build a
/// ColorSet bitset.
///
/// @param tile The pixel tile to convert
/// @param color_map The color-to-index mapping
/// @param extrinsic The extrinsic transparency color
/// @pre All colors in the tile are present in color_map
/// @return ColorSet containing all non-transparent tile colors
[[nodiscard]] ColorSet build_color_set_from_tile(
    const PixelTile<Rgba32> &tile, const ColorIndexMap<Rgba32> &color_map, const Rgba32 &extrinsic)
{
    ColorSet color_set{};
    const auto unique_colors = tile.unique_nontransparent_colors(extrinsic);

    for (const auto &color : unique_colors) {
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            // This will throw if a hint contains the extrinsic transparency color, since the ColorIndexMap won't
            // contain any transparency colors. Callers of the packer service should have used the PaletteValidator
            // service to validate input palettes and generate good user diagnostics.
            panic("tile color " + to_string(color) + " not found in color index map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/// @brief Converts a fixed-size palette's non-wildcard colors to a ColorSet.
///
/// @details
/// Extracts colors from slots 1-15 (skipping slot 0 which is transparency) and looks up each in the ColorIndexMap.
/// Wildcards are skipped. Returns both the ColorSet of unique colors and the count of occupied slots.
///
/// @param palette The palette to convert
/// @param color_map The color-to-index mapping
/// @pre All colors in the palette are present in color_map
/// @return ColorSetWithOccupancy containing unique colors and occupied slot count
[[nodiscard]] ColorSetWithOccupancy
build_color_set_from_palette(const Palette<Rgba32, palette::max_size> &palette, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};
    std::size_t occupied_slots = 0;

    // Start from slot 1 (slot 0 is transparency)
    for (std::size_t i = 1; i < palette.size(); ++i) {
        if (palette.is_wildcard(i)) {
            continue;
        }
        ++occupied_slots;
        const auto color = palette.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            // This will throw if a palette contains the extrinsic transparency color, since the ColorIndexMap won't
            // contain any transparency colors. Callers of the packer service should have used the PaletteValidator
            // service to validate input palettes and generate good user diagnostics.
            panic("palette color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return ColorSetWithOccupancy{color_set, occupied_slots};
}

/// @brief Converts a hint palette's colors to a ColorSet.
///
/// @details
/// Extracts colors from the hint palette and looks up each in the ColorIndexMap. Wildcards are skipped.
///
/// @param palette The dynamic palette to convert
/// @param color_map The color-to-index mapping
/// @pre All colors in the palette are present in color_map
/// @pre Palette contains no wildcards
/// @return ColorSet containing all non-transparent palette colors
[[nodiscard]] ColorSet
build_color_set_from_hint_palette(const Palette<Rgba32> &palette, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};

    for (std::size_t i = 0; i < palette.size(); ++i) {
        if (palette.is_wildcard(i)) {
            panic("build_color_set_from_hint_palette palette contained unexpected wildcard");
        }
        const auto color = palette.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            panic("hint color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/// @brief Builds a set of canonical key frame tiles from animation data.
///
/// @details
/// Mirrors the representative frame selection logic in @c AnimTileMatcher::register_animation(): if the animation has a
/// key frame, use it; otherwise fall back to the first regular frame. Each non-transparent tile is canonicalized and
/// added to the result set. This set is used to exclude animation key frame tiles from shape group analysis, since
/// @c AnimTileMatcher::find_match() redirects these tiles to pre-placed animation tile indices during tile assignment,
/// making any sharing alignment computed for them illusory.
///
/// @param anims The animation map from PackingParams
/// @param extrinsic The extrinsic transparency color
/// @return Set of canonical key frame tiles (as PixelTile<Rgba32>)
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

/// @brief Builds a combined tile vector of regular tiles and a parallel ID mapping.
///
/// @details
/// The combined vector is used as input to @c analyze_shape_groups(). The parallel @c combined_index_to_id vector maps
/// each combined index back to its PackableTile::RegularId, allowing shape group membership to be expressed in terms of
/// PackableTile IDs. Anim tiles are excluded because they occupy DMA-overwritten VRAM slots and cannot share with
/// regular tiles. Additionally, tiles whose canonical form matches an animation key frame tile are excluded, since
/// @c AnimTileMatcher will redirect them to animation tile indices during tile assignment.
struct CombinedTiles {
    std::vector<PixelTile<Rgba32>> tiles;
    std::vector<PackableTile::Id> index_to_id;
};

[[nodiscard]] CombinedTiles
build_combined_tiles(const PackingParams &params, const std::set<PixelTile<Rgba32>> &anim_keyframe_tiles)
{
    CombinedTiles combined;
    combined.tiles.reserve(params.tiles_.size() + params.primary_tiles_.size());
    combined.index_to_id.reserve(params.tiles_.size() + params.primary_tiles_.size());

    for (std::size_t i = 0; i < params.tiles_.size(); ++i) {
        CanonicalPixelTile<Rgba32> canonical{params.tiles_.at(i)};
        if (anim_keyframe_tiles.contains(static_cast<const PixelTile<Rgba32> &>(canonical))) {
            continue;
        }
        combined.tiles.push_back(params.tiles_.at(i));
        combined.index_to_id.push_back(PackableTile::RegularId{i});
    }

    for (std::size_t i = 0; i < params.primary_tiles_.size(); ++i) {
        const auto &primary_ref = params.primary_tiles_.at(i);
        combined.tiles.push_back(primary_ref.tile);
        combined.index_to_id.push_back(PackableTile::PrimaryTileId{i, primary_ref.palette_index});
    }

    return combined;
}

/// @brief Describes a single member of a sharing group for diagnostic purposes.
struct SharingGroupMember {
    std::size_t tile_index;
    std::size_t palette_index;
    bool is_primary{false};
};

/// @brief Describes a shape group whose members landed in distinct palettes after packing.
///
/// @details
/// Used locally within pack_tiles() for Phase 2/3/4 diagnostic emission and summary computation.
struct PartitionGroup {
    std::vector<SharingGroupMember> members;

    /// @brief Tile indices for each distinct color version (secondary tiles only).
    ///
    /// @details
    /// Primary tile members contribute to the seen_colors dedup set but are not added here, since these indices are
    /// used to index into params.tiles_ for diagnostic display. Safe to pass directly to diagnostic helpers.
    std::vector<std::size_t> color_version_tile_indices;

    /// @brief Primary tile color version entries (tile_index into primary_tiles_, palette_index).
    ///
    /// @details
    /// Collected during partition group computation for primary tiles that introduce a new color version. Used for
    /// diagnostic display of primary tile art in Phase 1 and Phase 2.
    std::vector<std::pair<std::size_t, std::size_t>> primary_color_version_entries;
};

/// @brief Tracks per-group statistics for partially aligned sharing groups.
///
/// @details
/// Populated during Phase 3 verification when a sharing group has some members that aligned with the reference indexed
/// tile and others that diverged. Used to emit summary diagnostics showing the full/partial breakdown.
struct PartialAlignmentInfo {
    std::size_t group_id;
    std::size_t dropped_color_version_count;
};

/// @brief Result of converting PackingParams inputs into packer-internal forms.
struct PackingInputs {
    std::vector<PackableTile> regular_tiles;
    std::vector<PackableTile> hint_tiles;
    std::set<PrefilledPalette> prefilled_palettes;
};

/// @brief Result of Phase 2 partition group computation.
///
/// @details
/// Captures which shape groups are eligible for sharing (2+ members in 2+ distinct palettes), the corresponding
/// partition groups, and the mapping from shape group index to partition group index.
struct Phase2Result {
    std::set<std::size_t> eligible_shape_indices;
    std::vector<PartitionGroup> partition_groups;
    std::map<std::size_t, std::size_t> shape_to_partition_index;
};

/// @brief Per-group verification detail for Phase 3 diagnostics.
struct Phase3GroupResult {
    std::size_t shape_group_index;
    bool is_partial;
    std::size_t dropped_color_version_count;
    std::size_t matching_count;
    std::map<std::size_t, std::vector<std::size_t>> members_by_palette;
    bool has_primary_members{false};
    std::map<std::size_t, std::vector<std::size_t>> primary_members_by_palette;
};

/// @brief Result of Phase 3 alignment verification.
///
/// @details
/// Contains all counts, partial alignment info, unaligned groups, and per-group detail data needed for diagnostic
/// emission.
struct Phase3Result {
    std::size_t aligned_count{};
    std::size_t fully_aligned_count{};
    std::size_t partially_aligned_count{};
    std::vector<PartialAlignmentInfo> partial_alignment_infos;
    std::vector<std::pair<std::size_t, PartitionGroup>> unaligned_partition_groups;
    std::vector<Phase3GroupResult> aligned_groups;
};

/// @brief Describes a verified member of a sharing group for Phase 3 alignment checking.
struct VerifiedMember {
    std::size_t regular_index;
    std::size_t hw_palette;
    CanonicalPixelTile<IndexPixel> canonical_indexed;
    std::map<ShapeMask, Rgba32> colors;
    bool is_primary{false};
};

/// @brief Converts PackingParams inputs into packer-internal forms (Steps 1-3).
///
/// @details
/// Converts regular tiles + anim composite tiles to PackableTile, hint palettes to PackableTile, and input prefilled
/// palettes to PrefilledPalette. Pure conversion, no side effects.
///
/// @param params The packing input parameters
/// @return PackingInputs containing the converted forms
[[nodiscard]] PackingInputs build_packing_inputs(const PackingParams &params)
{
    // Step 1: Convert regular tiles and anims to PackableTile vector
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

    // Step 2: Convert hints to PackableTile vector
    std::vector<PackableTile> hint_tiles;
    hint_tiles.reserve(params.hints_.size());
    for (const PaletteHint &hint : params.hints_) {
        auto color_set = build_color_set_from_hint_palette(hint.palette(), params.color_map_);
        hint_tiles.emplace_back(PackableTile::HintId{hint.name()}, color_set);
    }

    // Step 3: Convert input prefilled palettes to PrefilledPalette set
    std::set<PrefilledPalette> prefilled_palettes;
    for (std::size_t i = 0; i < params.prefilled_palettes_.size(); ++i) {
        if (!params.prefilled_palettes_[i].has_value()) {
            continue;
        }
        auto [color_set, occupied_slots] =
            build_color_set_from_palette(params.prefilled_palettes_[i].value(), params.color_map_);
        prefilled_palettes.insert(PrefilledPalette::partially_locked(i, color_set, occupied_slots));
    }

    return PackingInputs{std::move(regular_tiles), std::move(hint_tiles), std::move(prefilled_palettes)};
}

/// @brief Populates PalettePacking::tile_to_palette_ from a PackingOutput.
///
/// @details
/// Visits each tile ID in the PackingOutput's tile_to_palette_ and records the palette assignment for RegularId entries
/// only. AnimId, HintId, and PrefilledPaletteId entries are silently skipped.
///
/// @param packing The PalettePacking to populate
/// @param output The low-level packing output containing tile-to-palette assignments
void populate_tile_to_palette(PalettePacking &packing, const PackingOutput &output)
{
    packing.tile_to_palette_.clear();
    for (const auto &[tile_id, palette_index] : output.tile_to_palette_) {
        std::visit(
            [&packing, palette_index]<typename IdVariant>(IdVariant &&id) {
                using Id = std::decay_t<IdVariant>;
                if constexpr (std::is_same_v<Id, PackableTile::RegularId>) {
                    packing.tile_to_palette_[id.index] = palette_index;
                }
                else if constexpr (std::is_same_v<Id, PackableTile::AnimId>) {
                    // Anim tiles are packed for palette assignment but excluded from tile_to_palette_
                }
                else if constexpr (std::is_same_v<Id, PackableTile::HintId>) {
                    // We don't currently care to store where hints got assigned
                }
                else if constexpr (std::is_same_v<Id, PackableTile::PrefilledPaletteId>) {
                    // Nothing to do here, we only had these PackableTiles for internal bookkeeping
                }
                else if constexpr (std::is_same_v<Id, PackableTile::PrimaryTileId>) {
                    // Primary tiles are never in the packer's output; no-op for exhaustive visit
                }
                else {
                    static_assert(sizeof(Id) == 0, "Unhandled PackableTile::Id variant alternative");
                }
            },
            tile_id);
    }
}

/// @brief Builds a combined-tile-index to hw-palette-index map from authoritative packing assignments.
///
/// @details
/// Iterates shape group members and maps each combined tile index to its assigned hardware palette index via the
/// packing's tile_to_palette_. Used by both the link builder and the sharing diagnostics verifier.
///
/// @param shape_groups The shape groups to iterate
/// @param combined The combined tiles with index-to-id mapping
/// @param tile_to_palette The authoritative tile-to-palette assignments from packing
/// @return Map from combined tile index to hardware palette index
[[nodiscard]] std::map<std::size_t, std::size_t> build_tile_palette_assignments(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const CombinedTiles &combined,
    const std::map<std::size_t, std::size_t> &tile_to_palette)
{
    std::map<std::size_t, std::size_t> tile_palette_assignments;
    for (const auto &group : shape_groups) {
        for (const auto &member : group.members) {
            const auto &id = combined.index_to_id.at(member.tile_index);
            if (std::holds_alternative<PackableTile::RegularId>(id)) {
                auto regular_index = std::get<PackableTile::RegularId>(id).index;
                if (tile_to_palette.contains(regular_index)) {
                    tile_palette_assignments[member.tile_index] = tile_to_palette.at(regular_index);
                }
            }
            else if (std::holds_alternative<PackableTile::PrimaryTileId>(id)) {
                auto primary_id = std::get<PackableTile::PrimaryTileId>(id);
                tile_palette_assignments[member.tile_index] = primary_id.palette_index;
            }
        }
    }
    return tile_palette_assignments;
}

/// @brief Computes Phase 2 partition groups from shape groups and packing assignments.
///
/// @details
/// Iterates shape groups and identifies those with 2+ members assigned to 2+ distinct palettes. Returns eligible
/// indices, partition groups, and shape-to-partition mapping. No diagnostic emission.
///
/// @param shape_groups The shape groups to analyze
/// @param combined The combined tiles with index-to-id mapping
/// @param tile_to_palette The authoritative tile-to-palette assignments from packing
/// @return Phase2Result with eligible indices, partition groups, and shape-to-partition index map
[[nodiscard]] Phase2Result compute_partition_groups(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const CombinedTiles &combined,
    const std::map<std::size_t, std::size_t> &tile_to_palette)
{
    Phase2Result result;
    for (std::size_t sg_index = 0; sg_index < shape_groups.size(); ++sg_index) {
        const auto &group = shape_groups.at(sg_index);
        std::set<std::size_t> distinct_palettes_set;
        std::vector<SharingGroupMember> members;
        std::set<std::map<ShapeMask, Rgba32>> seen_colors;
        std::vector<std::size_t> color_version_tile_indices;
        std::vector<std::pair<std::size_t, std::size_t>> primary_color_version_entries;

        for (const auto &member : group.members) {
            const auto &id = combined.index_to_id.at(member.tile_index);
            std::size_t display_index{};
            std::optional<std::size_t> palette_opt;

            bool is_primary = false;
            if (std::holds_alternative<PackableTile::RegularId>(id)) {
                auto regular_index = std::get<PackableTile::RegularId>(id).index;
                display_index = regular_index;
                if (tile_to_palette.contains(regular_index)) {
                    palette_opt = tile_to_palette.at(regular_index);
                }
            }
            else if (std::holds_alternative<PackableTile::PrimaryTileId>(id)) {
                auto primary_id = std::get<PackableTile::PrimaryTileId>(id);
                display_index = primary_id.tile_index;
                palette_opt = primary_id.palette_index;
                is_primary = true;
            }

            if (palette_opt.has_value()) {
                members.push_back(SharingGroupMember{display_index, palette_opt.value(), is_primary});
                distinct_palettes_set.insert(palette_opt.value());
                if (!seen_colors.contains(member.colors)) {
                    seen_colors.insert(member.colors);
                    if (!is_primary) {
                        color_version_tile_indices.push_back(display_index);
                    }
                    else {
                        primary_color_version_entries.emplace_back(display_index, palette_opt.value());
                    }
                }
            }
        }

        if (members.size() >= 2 && distinct_palettes_set.size() >= 2) {
            result.eligible_shape_indices.insert(sg_index);
            PartitionGroup partition;
            partition.members = std::move(members);
            partition.color_version_tile_indices = std::move(color_version_tile_indices);
            partition.primary_color_version_entries = std::move(primary_color_version_entries);
            result.shape_to_partition_index[sg_index] = result.partition_groups.size();
            result.partition_groups.push_back(std::move(partition));
        }
    }
    return result;
}

/// @brief Verifies sharing alignment against final palettes (Phase 3 computation).
///
/// @details
/// For each eligible shape group, verifies that members produce the same canonical indexed tile when indexed against
/// their assigned final palettes. Returns counts, partial alignment info, unaligned groups, and per-group detail data.
/// No diagnostic emission.
///
/// @param shape_groups The shape groups to verify
/// @param combined The combined tiles with index-to-id mapping
/// @param tile_palette_assignments The combined-tile-index to hw-palette-index map
/// @param phase2 The Phase 2 result with eligible indices and partition groups
/// @param final_palettes The final output palettes with indirect links applied
/// @param extrinsic The extrinsic transparency color
/// @return Phase3Result with alignment counts and per-group verification data
[[nodiscard]] Phase3Result verify_sharing_alignment(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const CombinedTiles &combined,
    const std::map<std::size_t, std::size_t> &tile_palette_assignments,
    const Phase2Result &phase2,
    const std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> &final_palettes,
    const Rgba32 &extrinsic)
{
    Phase3Result result;
    std::set<std::size_t> aligned_shape_indices;

    for (std::size_t sg_index = 0; sg_index < shape_groups.size(); ++sg_index) {
        if (!phase2.eligible_shape_indices.contains(sg_index)) {
            continue;
        }
        const auto &group = shape_groups.at(sg_index);
        std::vector<VerifiedMember> verified;

        for (const auto &member : group.members) {
            if (!tile_palette_assignments.contains(member.tile_index)) {
                continue;
            }
            const auto &id = combined.index_to_id.at(member.tile_index);
            std::size_t display_index{};
            if (std::holds_alternative<PackableTile::RegularId>(id)) {
                display_index = std::get<PackableTile::RegularId>(id).index;
            }
            else if (std::holds_alternative<PackableTile::PrimaryTileId>(id)) {
                display_index = std::get<PackableTile::PrimaryTileId>(id).tile_index;
            }
            else {
                continue;
            }
            std::size_t hw = tile_palette_assignments.at(member.tile_index);

            bool is_primary_member = std::holds_alternative<PackableTile::PrimaryTileId>(id);

            const auto &tile = combined.tiles.at(member.tile_index);
            auto indexed_tile = index_tile_from_color_tile(tile, final_palettes.at(hw).value(), extrinsic);
            CanonicalPixelTile<IndexPixel> canonical{indexed_tile};
            verified.push_back(
                VerifiedMember{display_index, hw, std::move(canonical), member.colors, is_primary_member});
        }

        if (verified.size() < 2) {
            continue;
        }

        const auto &reference = verified.at(0).canonical_indexed;
        std::vector<SharingGroupMember> result_members;
        std::set<std::size_t> distinct_palettes_set;

        std::set<std::map<ShapeMask, Rgba32>> dropped_color_versions;
        for (const auto &member : verified) {
            const auto &member_as_tile = static_cast<const PixelTile<IndexPixel> &>(member.canonical_indexed);
            const auto &ref_as_tile = static_cast<const PixelTile<IndexPixel> &>(reference);
            if (member_as_tile != ref_as_tile) {
                dropped_color_versions.insert(member.colors);
                continue;
            }
            result_members.push_back(SharingGroupMember{member.regular_index, member.hw_palette, member.is_primary});
            distinct_palettes_set.insert(member.hw_palette);
        }
        const std::size_t dropped_color_version_count = dropped_color_versions.size();

        if (result_members.size() >= 2 && distinct_palettes_set.size() >= 2) {
            aligned_shape_indices.insert(sg_index);

            const bool is_partial = dropped_color_version_count > 0;
            if (is_partial) {
                ++result.partially_aligned_count;
                result.partial_alignment_infos.push_back(PartialAlignmentInfo{sg_index, dropped_color_version_count});
            }
            else {
                ++result.fully_aligned_count;
            }

            std::map<std::size_t, std::vector<std::size_t>> members_by_palette;
            std::map<std::size_t, std::vector<std::size_t>> primary_members_by_palette;
            bool has_primary = false;
            for (const auto &member : result_members) {
                if (member.is_primary) {
                    has_primary = true;
                    primary_members_by_palette[member.palette_index].push_back(member.tile_index);
                    continue;
                }
                members_by_palette[member.palette_index].push_back(member.tile_index);
            }

            result.aligned_groups.push_back(
                Phase3GroupResult{
                    sg_index,
                    is_partial,
                    dropped_color_version_count,
                    result_members.size(),
                    std::move(members_by_palette),
                    has_primary,
                    std::move(primary_members_by_palette)});
        }
    }

    result.aligned_count = aligned_shape_indices.size();

    // Compute unaligned partition groups: eligible groups that did not produce a sharing result
    for (const auto sg_index : phase2.eligible_shape_indices) {
        if (!aligned_shape_indices.contains(sg_index)) {
            result.unaligned_partition_groups.emplace_back(
                sg_index, phase2.partition_groups.at(phase2.shape_to_partition_index.at(sg_index)));
        }
    }

    return result;
}

/// @brief Emits Phase 1 sharing opportunity diagnostics.
///
/// @details
/// For each shape group, emits a remark showing detected color versions (including primary tile art) and all tilemap
/// entries. Emits a summary count.
///
/// @param format TextFormatter for building diagnostic output
/// @param diag UserDiagnostics for emitting remarks
/// @param tile_printer TilePrinter for rendering tile ASCII art
/// @param shape_groups The shape groups detected
/// @param combined The combined tiles with index-to-id mapping
/// @param params The packing parameters (for tiles, primary tiles, and extrinsic transparency)
void emit_phase1_diagnostics(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const TilePrinter &tile_printer,
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const CombinedTiles &combined,
    const PackingParams &params)
{
    for (std::size_t shape_group_index = 0; shape_group_index < shape_groups.size(); ++shape_group_index) {
        const auto &shape_group = shape_groups.at(shape_group_index);
        std::vector<std::size_t> color_version_tile_indices;
        std::vector<std::size_t> all_tile_indices;
        std::vector<std::pair<std::size_t, std::size_t>> primary_color_version_entries;
        std::set<std::map<ShapeMask, Rgba32>> seen_colors;
        for (const auto &member : shape_group.members) {
            const auto &id = combined.index_to_id.at(member.tile_index);
            if (std::holds_alternative<PackableTile::RegularId>(id)) {
                auto regular_index = std::get<PackableTile::RegularId>(id).index;
                all_tile_indices.push_back(regular_index);
                if (!seen_colors.contains(member.colors)) {
                    seen_colors.insert(member.colors);
                    color_version_tile_indices.push_back(regular_index);
                }
            }
            else if (std::holds_alternative<PackableTile::PrimaryTileId>(id)) {
                auto primary_id = std::get<PackableTile::PrimaryTileId>(id);
                if (!seen_colors.contains(member.colors)) {
                    seen_colors.insert(member.colors);
                    primary_color_version_entries.emplace_back(primary_id.tile_index, primary_id.palette_index);
                }
            }
        }

        const auto total_color_versions = color_version_tile_indices.size() + primary_color_version_entries.size();
        auto phase1_tag = std::format("tile-sharing-shareable-tiles-{}", shape_group_index);

        std::vector<std::string> remark_lines;
        remark_lines.emplace_back(format.format(
            "Detected sharing opportunity (group id '{}') with '{}' color version(s) across '{}' tilemap entries.",
            FormatParam{shape_group_index, Style::bold},
            FormatParam{total_color_versions, Style::bold},
            FormatParam{all_tile_indices.size(), Style::bold}));
        if (!primary_color_version_entries.empty()) {
            remark_lines.emplace_back(format.format(
                "'{}' color version(s) from paired primary.",
                FormatParam{primary_color_version_entries.size(), Style::bold}));
        }
        if (!color_version_tile_indices.empty()) {
            remark_lines.append_range(build_tile_sharing_color_version_tile_lines(
                format, tile_printer, params.tiles_, params.extrinsic_transparency_, color_version_tile_indices));
        }
        if (!primary_color_version_entries.empty()) {
            remark_lines.append_range(build_primary_tile_color_version_lines(
                format,
                tile_printer,
                params.primary_tiles_,
                params.extrinsic_transparency_,
                primary_color_version_entries,
                color_version_tile_indices.size()));
        }
        remark_lines.emplace_back(
            format.format("All tilemap entries ('{}' total):", FormatParam{all_tile_indices.size(), Style::bold}));
        remark_lines.append_range(build_truncated_tile_ref_lines(format, all_tile_indices));
        diag.remark(phase1_tag, remark_lines);
    }

    diag.remark(
        "tile-sharing-shareable-tiles-summary",
        "Tile sharing detection: '{}' shareable shape group(s) found.",
        FormatParam{shape_groups.size(), Style::bold});
}

/// @brief Emits Phase 2 partition group diagnostics.
///
/// @details
/// For each eligible partition group, emits a remark showing color versions, per-palette tile assignments, and packing
/// caveat when packing is off. Emits a summary count.
///
/// @param format TextFormatter for building diagnostic output
/// @param diag UserDiagnostics for emitting remarks
/// @param tile_printer TilePrinter for rendering tile ASCII art
/// @param shape_groups The shape groups
/// @param phase2 The Phase 2 computation result
/// @param params The packing parameters (for tiles, extrinsic, and packing config)
void emit_phase2_diagnostics(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const TilePrinter &tile_printer,
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const Phase2Result &phase2,
    const PackingParams &params)
{
    for (std::size_t sg_index = 0; sg_index < shape_groups.size(); ++sg_index) {
        if (!phase2.eligible_shape_indices.contains(sg_index)) {
            continue;
        }
        const auto &partition = phase2.partition_groups.at(phase2.shape_to_partition_index.at(sg_index));

        auto phase2_tag = std::format("tile-sharing-palette-partition-{}", sg_index);

        // Separate primary and secondary members for diagnostic display
        bool has_primary_members = false;
        std::map<std::size_t, std::vector<std::size_t>> members_by_palette;
        for (const auto &member : partition.members) {
            if (member.is_primary) {
                has_primary_members = true;
                continue;
            }
            members_by_palette[member.palette_index].push_back(member.tile_index);
        }

        std::vector<std::string> remark_lines;
        remark_lines.emplace_back(format.format(
            "After palette packing (group id '{}'), '{}' color versions across '{}' tilemap entries eligible for "
            "sharing.",
            FormatParam{sg_index, Style::bold},
            FormatParam{partition.color_version_tile_indices.size(), Style::bold},
            FormatParam{partition.members.size(), Style::bold}));
        if (has_primary_members) {
            remark_lines.emplace_back("Includes cross-tileset member(s) from paired primary.");
        }

        if (!partition.color_version_tile_indices.empty()) {
            remark_lines.append_range(build_tile_sharing_color_version_tile_lines(
                format,
                tile_printer,
                params.tiles_,
                params.extrinsic_transparency_,
                partition.color_version_tile_indices));
        }
        if (!partition.primary_color_version_entries.empty()) {
            remark_lines.append_range(build_primary_tile_color_version_lines(
                format,
                tile_printer,
                params.primary_tiles_,
                params.extrinsic_transparency_,
                partition.primary_color_version_entries,
                partition.color_version_tile_indices.size()));
        }
        remark_lines.append_range(build_per_palette_tile_ref_lines(format, members_by_palette));

        if (params.tile_sharing_packing_ == TileSharingPacking::off) {
            remark_lines.emplace_back("");
            remark_lines.emplace_back("--------");
            remark_lines.emplace_back("");
            remark_lines.emplace_back("Caveat: the palette partition shown above may be coincidental.");
            remark_lines.emplace_back(format.format(
                "Packing '{}' does not actively separate shape-group siblings across palettes.",
                FormatParam{to_string(params.tile_sharing_packing_.value()), Style::bold}));
            remark_lines.emplace_back("");
            remark_lines.append_range(format_config_note(format, params.tile_sharing_packing_));
        }
        diag.remark(phase2_tag, remark_lines);
    }

    diag.remark(
        "tile-sharing-palette-partition-summary",
        "Tile sharing partition: '{}' of '{}' shape group(s) eligible for sharing after palette packing.",
        FormatParam{phase2.eligible_shape_indices.size(), Style::bold},
        FormatParam{shape_groups.size(), Style::bold});
}

/// @brief Emits Phase 3 per-group alignment success/partial diagnostics.
///
/// @details
/// For each aligned group in the Phase3Result, emits a remark showing palette participation, per-palette tile
/// assignments (including primary tile representative art), and caveats for partial alignment or off alignment mode.
///
/// @param format TextFormatter for building diagnostic output
/// @param diag UserDiagnostics for emitting remarks
/// @param tile_printer TilePrinter for rendering tile ASCII art
/// @param phase3 The Phase 3 verification result
/// @param params The packing parameters (for tiles, primary tiles, extrinsic transparency, and alignment config)
void emit_phase3_diagnostics(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const TilePrinter &tile_printer,
    const Phase3Result &phase3,
    const PackingParams &params)
{
    for (const auto &group : phase3.aligned_groups) {
        auto phase3_tag = std::format("tile-sharing-result-{}", group.shape_group_index);

        std::vector<std::string> remark_lines;
        if (group.is_partial) {
            remark_lines.emplace_back(format.format(
                "Tile sharing partially succeeded (group id '{}'): '{}' participating palette(s) (referenced "
                "in '{}' tilemap entries).",
                FormatParam{group.shape_group_index, Style::bold},
                FormatParam{group.members_by_palette.size(), Style::bold},
                FormatParam{group.matching_count, Style::bold}));
            remark_lines.emplace_back(format.format(
                "'{}' color version(s) dropped due to alignment divergence.",
                FormatParam{group.dropped_color_version_count, Style::bold}));
        }
        else {
            remark_lines.emplace_back(format.format(
                "Tile sharing succeeded (group id '{}'): '{}' participating palette(s) (referenced in '{}' "
                "tilemap entries).",
                FormatParam{group.shape_group_index, Style::bold},
                FormatParam{group.members_by_palette.size(), Style::bold},
                FormatParam{group.matching_count, Style::bold}));
        }
        if (group.has_primary_members) {
            remark_lines.emplace_back("Includes cross-tileset member(s) from paired primary.");
        }
        if (!group.members_by_palette.empty()) {
            remark_lines.append_range(build_representative_tile_per_palette_lines(
                format, tile_printer, params.tiles_, params.extrinsic_transparency_, group.members_by_palette));
            remark_lines.append_range(build_per_palette_tile_ref_lines(format, group.members_by_palette));
        }
        if (!group.primary_members_by_palette.empty()) {
            remark_lines.append_range(build_primary_representative_tile_per_palette_lines(
                format,
                tile_printer,
                params.primary_tiles_,
                params.extrinsic_transparency_,
                group.primary_members_by_palette));
        }

        if (group.is_partial) {
            remark_lines.emplace_back("");
            remark_lines.emplace_back("--------");
            remark_lines.emplace_back("");
            remark_lines.emplace_back(format.format(
                "'{}' color version(s) diverged from the reference and were dropped.",
                FormatParam{group.dropped_color_version_count, Style::bold}));
        }

        if (params.tile_sharing_alignment_ == TileSharingAlignment::off) {
            remark_lines.emplace_back("");
            remark_lines.emplace_back("--------");
            remark_lines.emplace_back("");
            remark_lines.emplace_back("Caveat: the slot alignment shown above may be coincidental.");
            remark_lines.emplace_back(format.format(
                "Alignment '{}' does not actively align palette slots for tile sharing.",
                FormatParam{to_string(params.tile_sharing_alignment_.value()), Style::bold}));
            remark_lines.emplace_back("");
            remark_lines.append_range(format_config_note(format, params.tile_sharing_alignment_));
        }
        diag.remark(phase3_tag, remark_lines);
    }
}

/// @brief Emits the full tile sharing summary.
///
/// @details
/// Emits detected -> eligible -> aligned breakdown, partial listings, unaligned groups with per-group failure details,
/// aggregate failure summary, and actionable suggestions.
///
/// @param format TextFormatter for building diagnostic output
/// @param diag UserDiagnostics for emitting remarks
/// @param tile_printer TilePrinter for rendering tile ASCII art
/// @param params The packing parameters
/// @param shape_groups The shape groups
/// @param phase2 The Phase 2 result
/// @param phase3 The Phase 3 result
/// @param failure_counts The alignment failure counts from palette building
void emit_sharing_summary(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const TilePrinter &tile_printer,
    const PackingParams &params,
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const Phase2Result &phase2,
    const Phase3Result &phase3,
    const AlignmentFailureCounts &failure_counts)
{
    const auto eligible = phase2.partition_groups.size();
    const auto &fc = failure_counts;

    constexpr auto tag = "tile-sharing-result-summary";

    std::vector<std::string> remark_lines;
    if (phase3.partially_aligned_count > 0) {
        remark_lines.emplace_back("Tile sharing summary:");
        remark_lines.emplace_back(format.format(
            "'{}' detected → '{}' eligible after packing → '{}' aligned ('{}' fully, '{}' "
            "partially).",
            FormatParam{shape_groups.size(), Style::bold},
            FormatParam{eligible, Style::bold},
            FormatParam{phase3.aligned_count, Style::bold},
            FormatParam{phase3.fully_aligned_count, Style::bold},
            FormatParam{phase3.partially_aligned_count, Style::bold}));
    }
    else {
        remark_lines.emplace_back("Tile sharing summary:");
        remark_lines.emplace_back(format.format(
            "'{}' detected → '{}' eligible after packing → '{}' aligned.",
            FormatParam{shape_groups.size(), Style::bold},
            FormatParam{eligible, Style::bold},
            FormatParam{phase3.aligned_count, Style::bold}));
    }

    // Partially aligned groups listing
    if (phase3.partially_aligned_count > 0) {
        remark_lines.emplace_back("");
        remark_lines.emplace_back("Partially aligned groups:");
        for (const auto &info : phase3.partial_alignment_infos) {
            remark_lines.emplace_back(format.format(
                "  Group '{}': '{}' color version(s) dropped.",
                FormatParam{info.group_id, Style::bold},
                FormatParam{info.dropped_color_version_count, Style::bold}));
        }
        remark_lines.emplace_back("");
    }

    const auto unaligned = eligible - phase3.aligned_count;
    if (unaligned > 0) {
        if (fc.total() == 0 && params.tile_sharing_alignment_ != TileSharingAlignment::off) {
            panic("Hit 'Divergence with Zero Failures' case");
        }

        remark_lines.emplace_back(
            format.format("'{}' eligible group(s) could not be aligned:", FormatParam{unaligned, Style::bold}));

        for (const auto &[group_id, group] : phase3.unaligned_partition_groups) {
            remark_lines.emplace_back();
            bool has_primary_in_group = std::ranges::any_of(group.members, [](const auto &m) { return m.is_primary; });
            remark_lines.emplace_back(format.format(
                "Unaligned group (group id '{}'), '{}' color version(s):",
                FormatParam{group_id, Style::bold},
                FormatParam{group.color_version_tile_indices.size(), Style::bold}));
            if (has_primary_in_group) {
                remark_lines.emplace_back("Includes cross-tileset member(s) from paired primary.");
            }
            if (!group.color_version_tile_indices.empty()) {
                remark_lines.append_range(build_tile_sharing_color_version_tile_lines(
                    format,
                    tile_printer,
                    params.tiles_,
                    params.extrinsic_transparency_,
                    group.color_version_tile_indices));
            }

            // Show palette assignments for each color version
            for (std::size_t v = 0; v < group.color_version_tile_indices.size(); ++v) {
                const auto cv_tile_index = group.color_version_tile_indices.at(v);
                std::set<std::size_t> palettes_for_version;
                for (const auto &member : group.members) {
                    if (member.tile_index == cv_tile_index) {
                        palettes_for_version.insert(member.palette_index);
                    }
                }
                if (palettes_for_version.empty()) {
                    continue;
                }
                std::string palette_list;
                for (const auto palette : palettes_for_version) {
                    if (!palette_list.empty()) {
                        palette_list += ", ";
                    }
                    palette_list += palette_filename(palette);
                }
                remark_lines.emplace_back(format.format(
                    "  Version '{}' assigned to palette(s): '{}'.",
                    FormatParam{v + 1, Style::bold},
                    FormatParam{palette_list, Style::bold}));
            }

            // Show per-group failure details inline (only when alignment is active)
            if (params.tile_sharing_alignment_ != TileSharingAlignment::off) {
                const auto group_prefilled_dest =
                    std::ranges::count_if(fc.prefilled_destination_conflict_details, [&](const auto &d) {
                        return d.source_group_index == group_id;
                    });
                const auto group_prefilled_src =
                    std::ranges::count_if(fc.prefilled_source_conflict_details, [&](const auto &d) {
                        return d.source_group_index == group_id;
                    });
                const auto group_fww = std::ranges::count_if(
                    fc.first_writer_wins_details, [&](const auto &d) { return d.source_group_index == group_id; });
                const auto group_mismatch =
                    std::ranges::count_if(fc.post_resolution_mismatch_details, [&](const auto &d) {
                        return d.source_group_index == group_id;
                    });
                const auto group_total = group_prefilled_dest + group_prefilled_src + group_fww + group_mismatch;

                if (group_total > 0) {
                    remark_lines.emplace_back(
                        format.format("  '{}' link failure(s) for this group:", FormatParam{group_total, Style::bold}));
                    if (group_prefilled_dest > 0) {
                        remark_lines.emplace_back(format.format(
                            "    Prefilled destination conflict: '{}'.",
                            FormatParam{group_prefilled_dest, Style::bold}));
                        for (const auto &detail : fc.prefilled_destination_conflict_details) {
                            if (detail.source_group_index != group_id) {
                                continue;
                            }
                            remark_lines.emplace_back(format.format(
                                "      '{}' slot '{}': color '{}' blocked by locked color '{}'.",
                                FormatParam{palette_filename(detail.palette_index), Style::bold},
                                FormatParam{detail.target_slot, Style::bold},
                                color_param(detail.blocked_color),
                                color_param(detail.locked_color)));
                        }
                    }
                    if (group_fww > 0) {
                        remark_lines.emplace_back(
                            format.format("    Shared color conflict: '{}'.", FormatParam{group_fww, Style::bold}));
                        for (const auto &detail : fc.first_writer_wins_details) {
                            if (detail.source_group_index != group_id) {
                                continue;
                            }
                            remark_lines.emplace_back(format.format(
                                "      '{}': color '{}' linked to '{}' by group '{}',",
                                FormatParam{palette_filename(detail.source_palette_index), Style::bold},
                                color_param(detail.source_color),
                                FormatParam{palette_filename(detail.winning_ref_palette_index), Style::bold},
                                FormatParam{detail.winning_group_index, Style::bold}));
                            remark_lines.emplace_back(format.format(
                                "        this group wanted ref color '{}' in '{}'.",
                                color_param(detail.losing_ref_color),
                                FormatParam{palette_filename(detail.losing_ref_palette_index), Style::bold}));
                        }
                    }
                    if (group_prefilled_src > 0) {
                        remark_lines.emplace_back(format.format(
                            "    Prefilled source conflict: '{}'.", FormatParam{group_prefilled_src, Style::bold}));
                        for (const auto &detail : fc.prefilled_source_conflict_details) {
                            if (detail.source_group_index != group_id) {
                                continue;
                            }
                            remark_lines.emplace_back(format.format(
                                "      '{}': prefilled color '{}' could not be linked to color '{}' in "
                                "palette '{}'.",
                                FormatParam{palette_filename(detail.source_palette_index), Style::bold},
                                color_param(detail.source_color),
                                color_param(detail.ref_color),
                                FormatParam{palette_filename(detail.ref_palette_index), Style::bold}));
                        }
                    }
                    if (group_mismatch > 0) {
                        remark_lines.emplace_back(format.format(
                            "    Post-resolution slot mismatch: '{}'.", FormatParam{group_mismatch, Style::bold}));
                        for (const auto &detail : fc.post_resolution_mismatch_details) {
                            if (detail.source_group_index != group_id) {
                                continue;
                            }
                            remark_lines.emplace_back(format.format(
                                "      '{}': color '{}' ended at slot '{}', but ref color '{}' in '{}' is at "
                                "slot '{}'.",
                                FormatParam{palette_filename(detail.source_palette_index), Style::bold},
                                color_param(detail.source_color),
                                FormatParam{detail.source_final_slot, Style::bold},
                                color_param(detail.ref_color),
                                FormatParam{palette_filename(detail.ref_palette_index), Style::bold},
                                FormatParam{detail.ref_final_slot, Style::bold}));
                        }
                    }
                }
            }
        }

        // Aggregate failure summary
        remark_lines.emplace_back();
        if (params.tile_sharing_alignment_ == TileSharingAlignment::off) {
            remark_lines.emplace_back("Alignment is 'off'. No slot alignment was attempted.");
            remark_lines.emplace_back("Switch to 'greedy' to enable palette slot alignment for eligible groups.");
        }
        else {
            remark_lines.emplace_back(format.format(
                "'{}' total link resolution failure(s) across all unaligned groups.",
                FormatParam{fc.total(), Style::bold}));
        }

        // Actionable suggestions (skip for off mode, the suggestion is already above)
        remark_lines.emplace_back();
        if (phase3.partially_aligned_count > 0) {
            remark_lines.emplace_back(format.format(
                "Tile sharing is best-effort. '{}' of '{}' eligible group(s) aligned ('{}' fully, '{}' "
                "partially).",
                FormatParam{phase3.aligned_count, Style::bold},
                FormatParam{eligible, Style::bold},
                FormatParam{phase3.fully_aligned_count, Style::bold},
                FormatParam{phase3.partially_aligned_count, Style::bold}));
        }
        else {
            remark_lines.emplace_back(format.format(
                "Tile sharing is best-effort. '{}' of '{}' eligible group(s) aligned.",
                FormatParam{phase3.aligned_count, Style::bold},
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
                remark_lines.emplace_back("Rearranging or wildcarding those prefilled slots may improve tile sharing.");
            }
            if (!fc.first_writer_wins_details.empty()) {
                remark_lines.emplace_back(format.format(
                    "'{}' shared color conflict(s): multiple shape groups competed for the same color's link.",
                    FormatParam{fc.first_writer_wins_details.size(), Style::bold}));
                remark_lines.emplace_back("This is expected with greedy alignment.");
            }
            if (!fc.post_resolution_mismatch_details.empty()) {
                remark_lines.emplace_back(format.format(
                    "'{}' post-resolution slot mismatch(es): colors were displaced by eviction after successful "
                    "resolution.",
                    FormatParam{fc.post_resolution_mismatch_details.size(), Style::bold}));
                remark_lines.emplace_back("This occurs when multiple groups compete for the same palette slots.");
            }
        }
        remark_lines.emplace_back();
        remark_lines.emplace_back("For details on tile sharing, see:");
        remark_lines.emplace_back("  https://grunt-lucas.github.io/porytiles-user-docs/tile-sharing.html");
    }

    diag.remark(tag, remark_lines);
}

/// @brief Emits per-palette packing result diagnostics.
///
/// @param format TextFormatter for building diagnostic output
/// @param diag UserDiagnostics for emitting remarks
/// @param palette_printer PalettePrinter for rendering palette diagnostics
/// @param palettes The final packed palettes
void emit_palette_diagnostics(
    const TextFormatter &format,
    const UserDiagnostics &diag,
    const PalettePrinter &palette_printer,
    const std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> &palettes)
{
    for (std::size_t i = 0; i < palette::num_palettes; ++i) {
        const auto &maybe_packed_palette = palettes.at(i);
        if (maybe_packed_palette.has_value()) {
            constexpr auto palette_tag = "palette-packing-result";
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(
                format.format("'{}' packing result:", FormatParam{palette_filename(i), Style::bold}));
            remark_lines.emplace_back();
            remark_lines.append_range(palette_printer.print_rgba_palette(maybe_packed_palette.value()));
            diag.remark(palette_tag, remark_lines);
        }
    }
}

} // namespace

namespace porytiles {

ChainableResult<PalettePacking> PalettePacker::pack_tiles(const PackingParams &params) const
{
    // Steps 1-3: Convert inputs to packer-internal forms
    auto [regular_tiles, hint_tiles, prefilled_palettes] = build_packing_inputs(params);

    // Step 4: Create PackingInput and call low-level pack()
    PackingInput packing_input{
        std::move(regular_tiles),
        std::move(hint_tiles),
        std::move(prefilled_palettes),
        PalettePool{params.available_palettes_}};

    // Build animation keyframe exclusion set (once, before any shape group analysis)
    const auto anim_keyframe_tiles = build_anim_keyframe_set(params.anims_, params.extrinsic_transparency_);

    if (params.tile_sharing_packing_ == TileSharingPacking::optimal) {
        std::vector<std::string> err_lines;
        err_lines.emplace_back(format_->format(
            "Tile sharing packing '{}' is not yet implemented.",
            FormatParam{to_string(TileSharingPacking::optimal), Style::bold}));
        err_lines.emplace_back("");
        err_lines.append_range(format_config_note(*format_, params.tile_sharing_packing_));
        return ChainableResult<PalettePacking>{FormattableError{std::move(err_lines)}};
    }
    if (params.tile_sharing_alignment_ == TileSharingAlignment::optimal) {
        std::vector<std::string> err_lines;
        err_lines.emplace_back(format_->format(
            "Tile sharing alignment '{}' is not yet implemented.",
            FormatParam{to_string(TileSharingAlignment::optimal), Style::bold}));
        err_lines.emplace_back("");
        err_lines.append_range(format_config_note(*format_, params.tile_sharing_alignment_));
        return ChainableResult<PalettePacking>{FormattableError{std::move(err_lines)}};
    }

    // For biased packing, build shape group metadata BEFORE the initial pack call so the strategy
    // receives sharing-aware input. For off packing, pack normally first (metadata is only needed
    // post-packing for diagnostics and alignment).
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
        packing_output, std::move(pack_result), PalettePacking, "Low-level palette packing failed.");

    // Step 5a: Build tile_to_palette from PackingOutput
    PalettePacking packing{};
    populate_tile_to_palette(packing, packing_output);

    // 5b: Always compute shape groups for three-phase diagnostics, then conditionally build Indirect links.
    //
    // Shape group analysis and Phase 1/2 data are always computed regardless of config, so diagnostics can be
    // emitted with appropriate caveats. Indirect link generation (alignment) is conditioned on
    // tile_sharing_alignment_.
    std::vector<IndirectLink> indirect_links;

    // Always compute shape groups for three-phase diagnostics. Reuse pre-pack results when packing is
    // biased (they were already computed for metadata injection).
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

    // Phase 1: Detect sharing opportunities and emit diagnostics
    emit_phase1_diagnostics(*format_, *diag_, *tile_printer_, shape_groups, combined, params);

    // Phase 2: Compute partition groups and emit diagnostics
    auto phase2 = compute_partition_groups(shape_groups, combined, packing.tile_to_palette_);
    emit_phase2_diagnostics(*format_, *diag_, *tile_printer_, shape_groups, phase2, params);

    // Build tile-palette assignments from authoritative packing assignments
    std::map<std::size_t, std::size_t> tile_palette_assignments;
    if (!shape_groups.empty()) {
        tile_palette_assignments = build_tile_palette_assignments(shape_groups, combined, packing.tile_to_palette_);
    }

    // Conditionally build Indirect links based on alignment
    if (params.tile_sharing_alignment_ == TileSharingAlignment::greedy && !shape_groups.empty()) {
        auto base_palettes = build_all_output_palettes(
            packing_output.palettes_,
            params.prefilled_palettes_,
            params.color_map_,
            params.extrinsic_transparency_,
            {} /* empty links */);

        indirect_links =
            build_indirect_links(shape_groups, tile_palette_assignments, base_palettes, params.prefilled_palettes_);
    }

    // Build final output palettes with Indirect links applied (or empty links for off mode)
    AlignmentFailureCounts failure_counts{};
    auto final_palettes = build_all_output_palettes(
        packing_output.palettes_,
        params.prefilled_palettes_,
        params.color_map_,
        params.extrinsic_transparency_,
        indirect_links,
        !indirect_links.empty() ? &failure_counts : nullptr);

    // Phase 3: Verify sharing alignment against final palettes
    Phase3Result phase3;
    if (!shape_groups.empty()) {
        phase3 = verify_sharing_alignment(
            shape_groups, combined, tile_palette_assignments, phase2, final_palettes, params.extrinsic_transparency_);
    }
    emit_phase3_diagnostics(*format_, *diag_, *tile_printer_, phase3, params);

    // Tile sharing summary: emit when biased packing or greedy alignment is active
    if (params.tile_sharing_packing_ == TileSharingPacking::biased ||
        params.tile_sharing_alignment_ == TileSharingAlignment::greedy) {
        emit_sharing_summary(*format_, *diag_, *tile_printer_, params, shape_groups, phase2, phase3, failure_counts);
    }

    // Assemble final output palettes
    for (std::size_t i = 0; i < palette::num_palettes; ++i) {
        if (final_palettes.at(i).has_value()) {
            packing.palettes_.at(i) = final_palettes.at(i);
        }
    }

    emit_palette_diagnostics(*format_, *diag_, *palette_printer_, packing.palettes_);

    return packing;
}

} // namespace porytiles
