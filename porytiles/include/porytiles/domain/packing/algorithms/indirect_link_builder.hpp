#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <vector>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/shape_group.hpp"
#include "porytiles/domain/packing/models/color_position.hpp"

namespace porytiles {

/// @brief Builds Indirect links from shape groups and pre-computed palette assignments.
///
/// @details
/// For each shape group whose members span multiple palettes, generates IndirectLink instructions that tie
/// corresponding colors together. The links reference colors (not absolute slots), so they remain valid even when
/// final palette slot positions differ from the base palettes used for slot mapping.
///
/// This algorithm is inspired by the @c account_for_palette_swaps function in borytiles by ishax-kos
/// (https://github.com/ishax-kos/borytiles), specifically its @c compilation.rs module. Borytiles groups tiles by
/// shape, detects same-shape tiles in different palettes, and sets @c Indirect links pairing corresponding colors.
/// Porytiles extends this with a conflict-minimization heuristic for reference member selection and integrates the
/// link generation into a multi-phase pipeline with diagnostic reporting.
///
/// Algorithm per shape group:
/// 1. Look up each member's palette from @p tile_pal_assignments (authoritative packing assignments)
/// 2. Skip groups where fewer than 2 members resolve or all members are in the same palette
/// 3. Pick a reference member using a conflict-minimization heuristic (fewest prefilled-slot conflicts)
/// 4. For each non-reference member in a different palette, emit IndirectLink for each corresponding color pair
///    (identified by matching ShapeMask keys)
///
/// @param shape_groups The analyzed shape groups from ShapeGroupAnalyzer.
/// @param tile_pal_assignments Pre-computed mapping from combined tile index to hardware palette index. Built by the
///     packer from the authoritative packing assignments, ensuring consistency with eligibility determination.
/// @param base_pals Base palettes built with sequential fill only (no links), used for slot mapping during reference
///     member selection heuristic.
/// @param prefilled_pals The original prefilled input palettes (to detect locked slots during reference selection).
/// @return Vector of IndirectLink instructions, potentially empty if no sharing opportunities exist.
[[nodiscard]] std::vector<IndirectLink> build_indirect_links(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups,
    const std::map<std::size_t, std::size_t> &tile_pal_assignments,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &base_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &prefilled_pals);

} // namespace porytiles
