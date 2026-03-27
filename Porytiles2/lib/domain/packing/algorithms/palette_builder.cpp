#include "porytiles2/domain/packing/algorithms/palette_builder.hpp"

#include <map>
#include <set>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Per-palette state during the build process.
 */
struct PaletteBuildState {
    std::size_t hw_index;
    std::map<Rgba32, ColorPosition> color_positions;
    std::set<std::size_t> prefilled_slots;
};

/**
 * @brief Attempts to resolve an Indirect chain to an Absolute slot.
 *
 * @details
 * Follows the chain of IndirectPosition references across palettes until hitting an AbsolutePosition. Returns the
 * resolved slot on success, std::nullopt if the chain hits an Undetermined position (reference palette not yet
 * filled) or a broken reference. Panics on cycles (capped at pal::num_pals iterations).
 */
[[nodiscard]] std::optional<std::size_t> try_resolve_indirect(
    const IndirectPosition &start, const std::array<std::optional<PaletteBuildState>, pal::num_pals> &states)
{
    IndirectPosition current = start;
    for (std::size_t iter = 0; iter < pal::num_pals; ++iter) {
        if (!states.at(current.ref_pal_index).has_value()) {
            return std::nullopt;
        }
        const auto &ref_state = states.at(current.ref_pal_index).value();
        if (!ref_state.color_positions.contains(current.ref_color)) {
            return std::nullopt;
        }
        const auto &ref_position = ref_state.color_positions.at(current.ref_color);

        if (std::holds_alternative<AbsolutePosition>(ref_position)) {
            return std::get<AbsolutePosition>(ref_position).slot;
        }
        else if (std::holds_alternative<IndirectPosition>(ref_position)) {
            current = std::get<IndirectPosition>(ref_position);
        }
        else {
            // Undetermined — reference palette not yet sequentially filled
            return std::nullopt;
        }
    }
    panic("Indirect chain resolution exceeded maximum iterations (cycle detected)");
}

} // namespace

std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> build_all_output_palettes(
    const std::vector<PackedPalette> &packed_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &prefilled_pals,
    const ColorIndexMap<Rgba32> &color_map,
    const Rgba32 &default_slot_zero,
    const std::vector<IndirectLink> &indirect_links,
    AlignmentFailureCounts *failure_counts)
{
    // Build per-palette state indexed by hardware index
    std::array<std::optional<PaletteBuildState>, pal::num_pals> states{};

    // === Phase 1: Initialize position maps ===
    for (const PackedPalette &packed_pal : packed_pals) {
        const std::size_t hw = packed_pal.hardware_index();
        if (hw >= pal::num_pals) {
            panic("invalid hardware index " + std::to_string(hw) + ": out of range");
        }

        PaletteBuildState state;
        state.hw_index = hw;

        const Palette<Rgba32, pal::max_size> *prefilled_ptr =
            prefilled_pals.at(hw).has_value() ? &prefilled_pals.at(hw).value() : nullptr;

        // Track prefilled (locked) slots
        if (prefilled_ptr != nullptr) {
            for (std::size_t i = 1; i < pal::max_size; ++i) {
                if (!prefilled_ptr->is_wildcard(i)) {
                    state.prefilled_slots.insert(i);
                }
            }
        }

        // Collect colors from PackedPalette that need to be placed.
        // Use for_each_color to iterate in the same order as the old build_output_palette.
        std::set<Rgba32> already_placed_colors;

        // Colors from prefilled slots are already placed
        if (prefilled_ptr != nullptr) {
            for (std::size_t i = 1; i < pal::max_size; ++i) {
                if (!prefilled_ptr->is_wildcard(i)) {
                    already_placed_colors.insert(prefilled_ptr->at(i));
                    // Prefilled colors get Absolute positions at their locked slots
                    state.color_positions[prefilled_ptr->at(i)] = AbsolutePosition{i};
                }
            }
        }

        // Remaining colors from the packed palette start as Undetermined
        for_each_color(packed_pal.color_set(), [&](const std::size_t color_index) {
            const auto color_opt = color_map.color_at_index(ColorIndex{color_index});
            if (!color_opt.has_value()) {
                panic("color_index " + std::to_string(color_index) + " not found in color map");
            }
            const auto &color = color_opt.value();
            if (!already_placed_colors.contains(color)) {
                state.color_positions[color] = UndeterminedPosition{};
            }
        });

        states.at(hw) = std::move(state);
    }

    // Track successfully applied indirect links for post-resolution verification
    struct AppliedIndirect {
        std::size_t source_pal;
        Rgba32 source_color;
        std::size_t ref_pal;
        Rgba32 ref_color;
        std::size_t source_group_index;
    };
    std::vector<AppliedIndirect> applied_indirects;

    // === Phase 2: Apply Indirect links ===
    for (const auto &link : indirect_links) {
        if (!states.at(link.source_pal).has_value()) {
            continue;
        }
        auto &state = states.at(link.source_pal).value();

        if (!state.color_positions.contains(link.source_color)) {
            continue;
        }

        auto &position = state.color_positions.at(link.source_color);
        // First-writer-wins: only set Indirect on Undetermined positions (prevents cycles)
        if (std::holds_alternative<UndeterminedPosition>(position)) {
            position = IndirectPosition{link.ref_pal, link.ref_color, link.source_group_index};
            // Don't record here — Phase 4 will record if resolution succeeds
        }
        else if (std::holds_alternative<IndirectPosition>(position)) {
            const auto &existing = std::get<IndirectPosition>(position);
            // Compatible: same reference — no actual conflict
            if (existing.ref_pal_index == link.ref_pal && existing.ref_color == link.ref_color) {
                // The existing IndirectPosition already satisfies this link — record for post-verification
                // (Phase 4 will only record the winning group, so compatible groups need recording here)
                applied_indirects.push_back(
                    AppliedIndirect{
                        link.source_pal, link.source_color, link.ref_pal, link.ref_color, link.source_group_index});
                continue;
            }
            if (failure_counts != nullptr) {
                failure_counts->first_writer_wins_details.push_back(
                    FirstWriterWinsDetail{
                        .source_group_index = link.source_group_index,
                        .source_pal_index = link.source_pal,
                        .source_color = link.source_color,
                        .winning_group_index = existing.source_group_index,
                        .winning_ref_pal_index = existing.ref_pal_index,
                        .winning_ref_color = existing.ref_color,
                        .losing_ref_pal_index = link.ref_pal,
                        .losing_ref_color = link.ref_color});
            }
        }
        else if (std::holds_alternative<AbsolutePosition>(position)) {
            // Link dropped: source color is prefilled (locked) — Phase 1 set it to AbsolutePosition.
            // However, if the ref color in the ref palette is also AbsolutePosition at the same slot,
            // alignment is naturally satisfied — no conflict to report.
            bool naturally_aligned = false;
            const auto source_slot = std::get<AbsolutePosition>(position).slot;
            if (states.at(link.ref_pal).has_value()) {
                const auto &ref_state = states.at(link.ref_pal).value();
                if (ref_state.color_positions.contains(link.ref_color)) {
                    const auto &ref_position = ref_state.color_positions.at(link.ref_color);
                    if (std::holds_alternative<AbsolutePosition>(ref_position) &&
                        std::get<AbsolutePosition>(ref_position).slot == source_slot) {
                        naturally_aligned = true;
                    }
                }
            }
            if (!naturally_aligned && failure_counts != nullptr) {
                failure_counts->prefilled_source_conflict_details.push_back(
                    PrefilledSourceConflictDetail{
                        .source_group_index = link.source_group_index,
                        .source_pal_index = link.source_pal,
                        .source_color = link.source_color,
                        .ref_pal_index = link.ref_pal,
                        .ref_color = link.ref_color});
            }
        }
    }

    /*
     * === Phase 3: Sequential fill ALL palettes (skipping Indirect) ===
     *
     * Every Undetermined color gets an Absolute slot. Indirect colors are left untouched — they'll be resolved in
     * Phase 4. After this phase, all reference colors (which are Undetermined, not Indirect) have stable Absolute
     * positions, enabling Indirect chain resolution.
     */
    for (auto &state_opt : states) {
        if (!state_opt.has_value()) {
            continue;
        }
        auto &state = state_opt.value();

        // Collect slots already used by Absolute positions (prefilled)
        std::set<std::size_t> used_slots;
        used_slots.insert(0); // Slot 0 is always reserved
        for (const auto &[color, position] : state.color_positions) {
            if (std::holds_alternative<AbsolutePosition>(position)) {
                used_slots.insert(std::get<AbsolutePosition>(position).slot);
            }
        }

        // Assign next free slot to each Undetermined color; skip Indirect colors
        std::size_t next_slot = 1;
        for (auto &[color, position] : state.color_positions) {
            if (!std::holds_alternative<UndeterminedPosition>(position)) {
                continue;
            }
            while (next_slot < pal::max_size && used_slots.contains(next_slot)) {
                ++next_slot;
            }
            if (next_slot < pal::max_size) {
                position = AbsolutePosition{next_slot};
                used_slots.insert(next_slot);
                ++next_slot;
            }
            else {
                panic("ran out of palette slots during sequential fill for palette " + std::to_string(state.hw_index));
            }
        }
    }

    /*
     * === Phase 4: Resolve Indirect chains with eviction ===
     *
     * Now all reference colors have Absolute positions (from Phase 3). Resolve each Indirect color to the reference
     * color's slot. If the target slot is already occupied by a sequential-fill color, evict the occupant to the next
     * free slot. Prefilled slots are never evicted.
     *
     * This handles cross-palette Indirect dependencies (where palette A links to B and B links to A for different
     * shape groups) without deadlocking, because the reference colors are always Undetermined (not Indirect) and
     * were assigned Absolute positions in Phase 3.
     */
    for (std::size_t pal_index = 0; pal_index < states.size(); ++pal_index) {
        if (!states.at(pal_index).has_value()) {
            continue;
        }
        auto &state = states.at(pal_index).value();

        // Collect all Indirect colors and their resolved target slots
        struct IndirectResolution {
            Rgba32 color;
            std::size_t target_slot;
            std::size_t source_group_index;
            std::size_t ref_pal_index;
            Rgba32 ref_color;
        };
        std::vector<IndirectResolution> resolutions;

        for (const auto &[color, position] : state.color_positions) {
            if (!std::holds_alternative<IndirectPosition>(position)) {
                continue;
            }

            const auto &indirect_pos = std::get<IndirectPosition>(position);
            auto resolved = try_resolve_indirect(indirect_pos, states);
            if (!resolved.has_value()) {
                // Broken chain — skip this Indirect color (best-effort)
                if (failure_counts != nullptr) {
                    failure_counts->broken_chain_details.push_back(
                        BrokenChainDetail{
                            .source_group_index = indirect_pos.source_group_index,
                            .palette_index = pal_index,
                            .color = color});
                }
                continue;
            }

            // Skip if the target slot is prefilled (can't evict prefilled)
            if (state.prefilled_slots.contains(resolved.value())) {
                if (failure_counts != nullptr) {
                    // Capture detail: find the color occupying the locked slot
                    Rgba32 locked_color{};
                    for (const auto &[c, p] : state.color_positions) {
                        if (std::holds_alternative<AbsolutePosition>(p) &&
                            std::get<AbsolutePosition>(p).slot == resolved.value()) {
                            locked_color = c;
                            break;
                        }
                    }
                    failure_counts->prefilled_destination_conflict_details.push_back(
                        PrefilledDestinationConflictDetail{
                            .source_group_index = indirect_pos.source_group_index,
                            .palette_index = pal_index,
                            .target_slot = resolved.value(),
                            .blocked_color = color,
                            .locked_color = locked_color});
                }
                continue;
            }

            resolutions.push_back(
                IndirectResolution{
                    color,
                    resolved.value(),
                    indirect_pos.source_group_index,
                    indirect_pos.ref_pal_index,
                    indirect_pos.ref_color});
        }

        // Apply resolutions with eviction
        for (const auto &[indirect_color, target_slot, source_group_index, res_ref_pal, res_ref_color] : resolutions) {
            // Check if the target slot is occupied by a sequential-fill color
            Rgba32 evicted_color{};
            bool needs_eviction = false;

            for (auto &[color, position] : state.color_positions) {
                if (color == indirect_color) {
                    continue;
                }
                if (std::holds_alternative<AbsolutePosition>(position) &&
                    std::get<AbsolutePosition>(position).slot == target_slot &&
                    !state.prefilled_slots.contains(target_slot)) {
                    evicted_color = color;
                    needs_eviction = true;
                    break;
                }
            }

            if (needs_eviction) {
                /*
                 * Find next free slot for the evicted color. Note: target_slot stays in all_used so the free
                 * slot search won't pick target_slot itself (that slot is being claimed by the Indirect color).
                 */
                std::set<std::size_t> all_used;
                all_used.insert(0);
                for (const auto &[c, p] : state.color_positions) {
                    if (std::holds_alternative<AbsolutePosition>(p)) {
                        all_used.insert(std::get<AbsolutePosition>(p).slot);
                    }
                }
                std::size_t free_slot = 1;
                while (free_slot < pal::max_size && all_used.contains(free_slot)) {
                    ++free_slot;
                }
                if (free_slot < pal::max_size) {
                    state.color_positions.at(evicted_color) = AbsolutePosition{free_slot};
                }
                // else: no free slot — eviction impossible, skip this resolution
                else {
                    if (failure_counts != nullptr) {
                        failure_counts->no_free_slot_details.push_back(
                            NoFreeSlotDetail{
                                .source_group_index = source_group_index,
                                .palette_index = pal_index,
                                .color = indirect_color});
                    }
                    continue;
                }
            }

            // Place the Indirect color at the target slot
            state.color_positions.at(indirect_color) = AbsolutePosition{target_slot};

            // Record successfully resolved link for post-resolution verification
            applied_indirects.push_back(
                AppliedIndirect{pal_index, indirect_color, res_ref_pal, res_ref_color, source_group_index});
        }
    }

    /*
     * === Phase 5: Fallback — assign free slots to unresolved Indirect colors ===
     *
     * Phase 4 may leave colors in IndirectPosition if resolution failed (broken chain, prefilled destination conflict,
     * or no free slot for eviction). These colors still need placement in the final palette, so assign them sequential
     * free slots — identical to Phase 3's logic but targeting IndirectPosition instead of UndeterminedPosition.
     */
    for (auto &state_opt : states) {
        if (!state_opt.has_value()) {
            continue;
        }
        auto &state = state_opt.value();

        // Collect slots already used by Absolute positions
        std::set<std::size_t> used_slots;
        used_slots.insert(0); // Slot 0 is always reserved
        for (const auto &[color, position] : state.color_positions) {
            if (std::holds_alternative<AbsolutePosition>(position)) {
                used_slots.insert(std::get<AbsolutePosition>(position).slot);
            }
        }

        // Assign next free slot to each remaining Indirect color
        std::size_t next_slot = 1;
        for (auto &[color, position] : state.color_positions) {
            if (!std::holds_alternative<IndirectPosition>(position)) {
                continue;
            }
            while (next_slot < pal::max_size && used_slots.contains(next_slot)) {
                ++next_slot;
            }
            if (next_slot < pal::max_size) {
                position = AbsolutePosition{next_slot};
                used_slots.insert(next_slot);
                ++next_slot;
            }
            else {
                panic(
                    "ran out of palette slots during Indirect fallback fill for palette " +
                    std::to_string(state.hw_index));
            }
        }
    }

    // === Post-resolution verification: detect eviction displacement ===
    if (failure_counts != nullptr) {
        for (const auto &ai : applied_indirects) {
            if (!states.at(ai.source_pal).has_value() || !states.at(ai.ref_pal).has_value()) {
                continue;
            }
            const auto &source_state = states.at(ai.source_pal).value();
            const auto &ref_state = states.at(ai.ref_pal).value();

            if (!source_state.color_positions.contains(ai.source_color) ||
                !ref_state.color_positions.contains(ai.ref_color)) {
                continue;
            }

            const auto &source_pos = source_state.color_positions.at(ai.source_color);
            const auto &ref_pos = ref_state.color_positions.at(ai.ref_color);

            if (!std::holds_alternative<AbsolutePosition>(source_pos) ||
                !std::holds_alternative<AbsolutePosition>(ref_pos)) {
                continue;
            }

            const auto source_slot = std::get<AbsolutePosition>(source_pos).slot;
            const auto ref_slot = std::get<AbsolutePosition>(ref_pos).slot;

            if (source_slot != ref_slot) {
                failure_counts->post_resolution_mismatch_details.push_back(
                    PostResolutionMismatchDetail{
                        .source_group_index = ai.source_group_index,
                        .source_pal_index = ai.source_pal,
                        .source_color = ai.source_color,
                        .source_final_slot = source_slot,
                        .ref_pal_index = ai.ref_pal,
                        .ref_color = ai.ref_color,
                        .ref_final_slot = ref_slot});
            }
        }
    }

    // === Phase 6: Build final palettes from resolved positions ===
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> result{};

    for (const auto &state_opt : states) {
        if (!state_opt.has_value()) {
            continue;
        }
        const auto &state = state_opt.value();
        const std::size_t hw = state.hw_index;

        Palette<Rgba32, pal::max_size> output{Rgba32{0, 0, 0, Rgba32::alpha_opaque}};

        // Set slot 0
        const Palette<Rgba32, pal::max_size> *prefilled_ptr =
            prefilled_pals.at(hw).has_value() ? &prefilled_pals.at(hw).value() : nullptr;
        if (prefilled_ptr != nullptr && !prefilled_ptr->is_wildcard(0)) {
            output.set(0, prefilled_ptr->at(0));
        }
        else {
            output.set(0, default_slot_zero);
        }

        // Place prefilled slots
        if (prefilled_ptr != nullptr) {
            for (std::size_t i = 1; i < pal::max_size; ++i) {
                if (!prefilled_ptr->is_wildcard(i)) {
                    output.set(i, prefilled_ptr->at(i));
                }
            }
        }

        // Place all Absolute-position colors
        for (const auto &[color, position] : state.color_positions) {
            if (std::holds_alternative<AbsolutePosition>(position)) {
                const std::size_t slot = std::get<AbsolutePosition>(position).slot;
                if (!state.prefilled_slots.contains(slot)) {
                    output.set(slot, color);
                }
            }
        }

        result.at(hw) = output;
    }

    return result;
}

} // namespace porytiles2
