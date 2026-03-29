#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <optional>
#include <vector>

#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/color_position.hpp"
#include "porytiles2/domain/packing/models/packed_palette.hpp"

namespace porytiles2 {

/**
 * @brief Detail record for a single prefilled destination conflict during Indirect chain resolution.
 *
 * @details
 * Captured in Phase 4 when an Indirect color's resolved target slot is occupied by a prefilled (locked) palette slot.
 * Users can inspect these records to identify which prefilled destination slots are blocking tile sharing alignment and
 * potentially rearrange them.
 */
struct PrefilledDestinationConflictDetail {
    std::size_t source_group_index;
    std::size_t palette_index;
    std::size_t target_slot;
    Rgba32 blocked_color;
    Rgba32 locked_color;

    auto operator<=>(const PrefilledDestinationConflictDetail &) const = default;
    bool operator==(const PrefilledDestinationConflictDetail &) const = default;
};

/**
 * @brief Detail record for a single prefilled source conflict during Indirect link application.
 *
 * @details
 * Captured in Phase 2 when an IndirectLink's source color is already prefilled (locked) in the source palette. Phase 1
 * marks prefilled colors as AbsolutePosition, so the link cannot override them. Users can inspect these records to
 * identify which prefilled source colors are preventing link application and potentially rearrange or wildcard them.
 */
struct PrefilledSourceConflictDetail {
    std::size_t source_group_index;
    std::size_t source_pal_index;
    Rgba32 source_color;
    std::size_t ref_pal_index;
    Rgba32 ref_color;

    auto operator<=>(const PrefilledSourceConflictDetail &) const = default;
    bool operator==(const PrefilledSourceConflictDetail &) const = default;
};

/**
 * @brief Detail record for a post-resolution slot mismatch.
 *
 * @details
 * Captured after Phase 5 when an IndirectLink's source color and reference color end up at different final slots,
 * despite the link being successfully applied in Phase 2 and resolved in Phase 4. This occurs when Phase 4 eviction
 * (either intra-palette or cross-palette) displaces a color from its resolved target slot after resolution.
 */
struct PostResolutionMismatchDetail {
    std::size_t source_group_index;
    std::size_t source_pal_index;
    Rgba32 source_color;
    std::size_t source_final_slot;
    std::size_t ref_pal_index;
    Rgba32 ref_color;
    std::size_t ref_final_slot;

    auto operator<=>(const PostResolutionMismatchDetail &) const = default;
    bool operator==(const PostResolutionMismatchDetail &) const = default;
};

/**
 * @brief Detail record for a single first-writer-wins conflict during Indirect link application.
 *
 * @details
 * Captured in Phase 2 when an IndirectLink targets a source color that already has an IndirectPosition from a previous
 * link with a *different* reference. The second link is silently dropped. Compatible links (same @c ref_pal and
 * @c ref_color as the existing IndirectPosition) are detected and skipped without recording a failure.
 *
 * Records both sides of the conflict: the winning group's reference (already applied) and the losing group's wanted
 * reference (dropped).
 */
struct FirstWriterWinsDetail {
    std::size_t source_group_index;
    std::size_t source_pal_index;
    Rgba32 source_color;
    std::size_t winning_group_index;
    std::size_t winning_ref_pal_index;
    Rgba32 winning_ref_color;
    std::size_t losing_ref_pal_index;
    Rgba32 losing_ref_color;

    auto operator<=>(const FirstWriterWinsDetail &) const = default;
    bool operator==(const FirstWriterWinsDetail &) const = default;
};

/**
 * @brief Detailed records of alignment failures during Indirect link application and chain resolution.
 *
 * @details
 * Each detail vector tracks a distinct reason why an Indirect link could not be applied or resolved, with per-record
 * attribution to the originating shape group. These records are populated by @c build_all_output_palettes when a
 * non-null pointer is provided, and are used for diagnostic remarks that explain why specific shape groups failed to
 * align. The @c total() method returns the aggregate failure count across all categories.
 */
struct AlignmentFailureCounts {
    std::vector<PrefilledDestinationConflictDetail> prefilled_destination_conflict_details;
    std::vector<PrefilledSourceConflictDetail> prefilled_source_conflict_details;
    std::vector<FirstWriterWinsDetail> first_writer_wins_details;
    std::vector<PostResolutionMismatchDetail> post_resolution_mismatch_details;

    [[nodiscard]] std::size_t total() const
    {
        return prefilled_destination_conflict_details.size() + prefilled_source_conflict_details.size() +
               first_writer_wins_details.size() + post_resolution_mismatch_details.size();
    }
};

/**
 * @brief Builds all output palettes from packed palettes and Indirect links in a single call.
 *
 * @details
 * Constructs final Rgba32 palettes from PackedPalette color sets using a six-phase algorithm. This function serves
 * both @c TileSharingAlignment::off and @c TileSharingAlignment::greedy modes. When @p indirect_links is empty (off
 * mode), Phases 2, 4, and 5 are no-ops and the function degenerates to: initialize positions, sequential fill,
 * materialize output. The @c TileSharingAlignment::optimal mode will use a separate CSP-based algorithm and will not
 * call this function.
 *
 * The multi-phase approach — sequential fill skipping Indirect colors, then resolving Indirect chains to Absolute
 * positions — is inspired by the Indirection resolution in the @c assign_palettes function from borytiles by
 * ishax-kos (https://github.com/ishax-kos/borytiles), specifically its @c compilation.rs module. Porytiles2
 * separates the phases more explicitly and adds eviction logic for slot conflicts with non-prefilled colors.
 *
 * **Phase 1 — Initialize position maps**: For each palette, slot 0 gets its prefilled or default color. Prefilled
 * non-wildcard slots become AbsolutePosition. Remaining colors from the PackedPalette start as UndeterminedPosition.
 *
 * **Phase 2 — Apply Indirect links**: For each IndirectLink, if the source color is still Undetermined, set it to
 * IndirectPosition{ref_pal, ref_color}. Already-Absolute or already-Indirect colors are skipped (first-writer-wins
 * prevents cycles). No-op when @p indirect_links is empty.
 *
 * **Phase 3 — Sequential fill (skip Indirect)**: For each palette, collect slots already used by Absolute positions.
 * Assign each remaining Undetermined color to the next free slot as Absolute. Indirect colors are skipped — they
 * don't compete for slots. After this phase, all reference colors (which are Undetermined, not Indirect) have stable
 * Absolute positions, enabling Indirect chain resolution in Phase 4.
 *
 * **Phase 4 — Resolve Indirect chains with eviction**: For each Indirect color, follow the chain
 * ref_pal[ref_color] until hitting an Absolute position. Cap at @c pal::num_pals iterations for cycle detection
 * (panics on cycle or broken chain — both are internal invariant violations). Place the color at the resolved slot.
 * If the target slot is occupied by a non-prefilled color, evict the occupant to the next free slot (panics if no
 * free slot exists — Phase 3 guarantees one free slot per Indirect color). If the slot conflicts with a prefilled
 * position, skip (best-effort). No-op when @p indirect_links is empty.
 *
 * **Phase 5 — Fallback fill for unresolved Indirects**: Any Indirect colors that failed resolution in Phase 4
 * (prefilled destination conflict) are assigned sequential free slots, identical to Phase 3's logic but targeting
 * IndirectPosition instead of UndeterminedPosition. Ensures all colors get placed even if Indirect alignment fails.
 * No-op when @p indirect_links is empty.
 *
 * **Phase 6 — Build final palettes**: Materializes all AbsolutePosition colors into Palette<Rgba32> output objects.
 * Places prefilled slots first, then all resolved colors at their Absolute positions.
 *
 * The sequential fill iteration order matches the existing @c for_each_color + @c color_map path from the old
 * @c build_output_palette to preserve identical palette layouts when no links are present (manual mode compatibility).
 *
 * @param packed_pals The packed palette results from the packer.
 * @param prefilled_pals The original prefilled input palettes (locked slots).
 * @param color_map The color-to-index mapping for reverse lookup.
 * @param default_slot_zero The default color for slot 0 if no prefilled palette exists.
 * @param indirect_links The Indirect link instructions (empty for off mode).
 * @param failure_counts Optional output pointer for alignment failure counters. When non-null, incremented at each
 *     Phase 2, Phase 4, and Phase 5 skip point. Caller retains ownership.
 * @return Array of optional palettes, indexed by hardware palette index.
 */
[[nodiscard]] std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> build_all_output_palettes(
    const std::vector<PackedPalette> &packed_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &prefilled_pals,
    const ColorIndexMap<Rgba32> &color_map,
    const Rgba32 &default_slot_zero,
    const std::vector<IndirectLink> &indirect_links,
    AlignmentFailureCounts *failure_counts = nullptr);

} // namespace porytiles2
