#include "porytiles2/domain/packing/algorithms/indirect_link_builder.hpp"

#include <limits>
#include <map>
#include <set>

#include "porytiles2/domain/models/shape_mask.hpp"

namespace porytiles2 {

std::vector<IndirectLink> build_indirect_links(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const std::map<std::size_t, std::size_t> &tile_pal_assignments,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &base_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &prefilled_pals)
{
    std::vector<IndirectLink> links;

    for (std::size_t group_idx = 0; group_idx < shape_groups.size(); ++group_idx) {
        const auto &group = shape_groups.at(group_idx);

        // Resolve palette assignment for each member via authoritative packing assignments
        struct ResolvedMember {
            std::size_t member_idx;
            std::size_t hw_pal_index;
            std::map<ShapeMask, Rgba32> colors;
        };
        std::vector<ResolvedMember> resolved;

        for (std::size_t m = 0; m < group.members.size(); ++m) {
            const auto &member = group.members.at(m);

            if (!tile_pal_assignments.contains(member.tile_index)) {
                continue;
            }
            std::size_t hw_index = tile_pal_assignments.at(member.tile_index);
            resolved.push_back(ResolvedMember{m, hw_index, member.colors});
        }

        if (resolved.size() < 2) {
            continue;
        }

        // Check if members span multiple palettes
        std::set<std::size_t> distinct_pals;
        for (const auto &rm : resolved) {
            distinct_pals.insert(rm.hw_pal_index);
        }
        if (distinct_pals.size() < 2) {
            continue;
        }

        /*
         * Pick the best reference member that minimizes conflicts with prefilled slots in other members' palettes.
         * For each candidate reference, we check: if we were to link other members' colors to this reference's colors,
         * how many of those links would conflict with prefilled slots? We pick the candidate with fewest conflicts.
         *
         * Note: unlike the old constraint builder, we don't need to know actual slot positions here. We only need to
         * know whether a prefilled palette has a non-wildcard color at the slot where the ref color sits — this tells
         * us the link would be unable to resolve cleanly. For simplicity, we use the base palette slot positions for
         * this heuristic (same as the old builder did with Pass 1 positions).
         */
        std::size_t best_ref_index = 0;
        std::size_t best_ref_conflicts = std::numeric_limits<std::size_t>::max();

        for (std::size_t candidate_ref = 0; candidate_ref < resolved.size(); ++candidate_ref) {
            const auto &candidate = resolved.at(candidate_ref);
            const auto &candidate_pal = base_pals.at(candidate.hw_pal_index).value();

            // Build candidate reference's color -> slot mapping from base palette
            std::map<ShapeMask, std::size_t> candidate_mask_to_slot;
            for (const auto &[mask, color] : candidate.colors) {
                for (std::size_t slot = 1; slot < pal::max_size; ++slot) {
                    if (!candidate_pal.is_wildcard(slot) && candidate_pal.at(slot) == color) {
                        candidate_mask_to_slot[mask] = slot;
                        break;
                    }
                }
            }

            // Count conflicts: how many links would conflict with prefilled slots in other members' palettes?
            std::size_t conflicts = 0;
            for (std::size_t other = 0; other < resolved.size(); ++other) {
                if (other == candidate_ref || resolved.at(other).hw_pal_index == candidate.hw_pal_index) {
                    continue;
                }
                const auto &other_member = resolved.at(other);
                if (!prefilled_pals.at(other_member.hw_pal_index).has_value()) {
                    continue;
                }
                const auto &prefilled = prefilled_pals.at(other_member.hw_pal_index).value();

                for (const auto &[mask, other_color] : other_member.colors) {
                    if (!candidate_mask_to_slot.contains(mask)) {
                        continue;
                    }
                    std::size_t target_slot = candidate_mask_to_slot.at(mask);
                    if (!prefilled.is_wildcard(target_slot)) {
                        ++conflicts;
                    }
                }
            }

            if (conflicts < best_ref_conflicts) {
                best_ref_conflicts = conflicts;
                best_ref_index = candidate_ref;
            }
        }

        const auto &ref = resolved.at(best_ref_index);

        // For each other member in a different palette, create Indirect links
        for (std::size_t r = 0; r < resolved.size(); ++r) {
            if (r == best_ref_index) {
                continue;
            }
            const auto &other = resolved.at(r);
            if (other.hw_pal_index == ref.hw_pal_index) {
                continue;
            }

            for (const auto &[mask, other_color] : other.colors) {
                // Find corresponding ref_color via same ShapeMask
                if (!ref.colors.contains(mask)) {
                    continue;
                }
                const auto &ref_color = ref.colors.at(mask);

                links.push_back(
                    IndirectLink{
                        .source_pal = other.hw_pal_index,
                        .source_color = other_color,
                        .ref_pal = ref.hw_pal_index,
                        .ref_color = ref_color,
                        .source_group_index = group_idx,
                    });
            }
        }
    }

    return links;
}

} // namespace porytiles2
