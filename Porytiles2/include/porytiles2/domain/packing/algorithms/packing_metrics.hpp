#pragma once

/**
 * @file packing_metrics.hpp
 * @brief Metrics for Bin Packing with Overlapping Items (Pagination problem).
 *
 * @details
 * This module provides metrics for the "Bin Packing with Overlapping Items" problem (also called the "Pagination"
 * problem), as described in:
 *
 * > Grange, Kacem, Martin. "Algorithms for the Bin Packing Problem with
 * > Overlapping Items" (2017) - https://arxiv.org/abs/1605.00558
 *
 * @section global_vs_local CRITICAL DISTINCTION: Global vs Palette-Local Multiplicity
 *
 * @subsection global_mult 1. GLOBAL MULTIPLICITY (build_global_multiplicity_map)
 * - Counts how many tiles across ALL INPUT TILES contain each color
 * - Computed ONCE at the start of packing
 * - DOES NOT vary per palette
 * - Use case: Prioritizing rare vs common colors across the entire input
 *
 * @subsection local_mult 2. PALETTE-LOCAL MULTIPLICITY (build_palette_local_multiplicity)
 * - Counts how many tiles within a SPECIFIC PALETTE contain each color
 * - Computed PER-PALETTE, dynamically as tiles are added/removed
 * - VARIES per palette (each palette has different tiles assigned)
 * - Use case: Measuring color overlap when placing a tile in a palette
 *
 * @warning The Best Fusion and Overload-and-Remove algorithms require PALETTE-LOCAL multiplicity to make placement
 * decisions. Using global multiplicity would cause all palettes to appear equally good, defeating the algorithm's
 * purpose.
 *
 * @see BestFusionStrategy Uses palette-local cost for tile placement
 * @see OverloadAndRemoveStrategy Uses palette-local cost and efficiency for placement/removal
 */

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"
#include "porytiles2/domain/packing/models/packed_palette.hpp"

namespace porytiles2 {

/**
 * @brief Builds a GLOBAL multiplicity map from all input tiles.
 *
 * @details
 * Computes μ(α) = count of tiles containing color α, across ALL input tiles. This is the multiplicity metric from
 * Definition 2.5 of Grange et al. (2017).
 *
 * Example: If color index 42 appears in 5 different input tiles, then map[42] = 5.
 *
 * @warning This function computes GLOBAL multiplicity across all tiles. For palette
 * placement decisions, use build_palette_local_multiplicity() instead.
 *
 * @param tiles The regular tiles to count colors from
 * @param hints The hint tiles to count colors from (processed same as regular tiles)
 * @return Map from color index to count of tiles containing that color
 *
 * @see build_palette_local_multiplicity For palette-specific multiplicity
 */
[[nodiscard]] std::map<std::size_t, std::size_t>
build_global_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints);

/**
 * @brief Builds a PALETTE-LOCAL multiplicity map for a specific palette.
 *
 * @details
 * Computes μ_p(α) = count of tiles in palette p containing color α.
 *
 * This is the key metric for the "weightBySymbol" calculation in the reference implementation (solver_tools.py, line
 * 92). Unlike global multiplicity, this varies per palette based on which tiles are currently assigned to it.
 *
 * Example: If palette P has 3 assigned tiles, and 2 of them contain color 42, then the returned map has map[42] = 2.
 *
 * @note The tile_colors_map must include entries for:
 *   - All regular tiles (keyed by TileIndex)
 *   - All hint tiles (keyed by TileIndex)
 *   - All prefilled palette system tiles (keyed by PrefilledPaletteId)
 *
 * @param palette The palette to analyze
 * @param tile_colors_map Map from tile ID to ColorSet for ALL known tiles
 * @return Map from color index to count of tiles in THIS palette containing that color
 *
 * @see compute_weighted_cost_in_palette Which uses this for placement decisions
 * @see compute_palette_local_efficiency Which uses this for removal decisions
 */
[[nodiscard]] std::map<std::size_t, std::size_t> build_palette_local_multiplicity(
    const PackedPalette &palette, const std::map<PackableTile::Id, ColorSet> &tile_colors_map);

/**
 * @brief Computes the weighted cost of placing a tile in a specific palette.
 *
 * @details
 * This implements the "weightedCostIn" metric from the reference implementation (solver_tools.py, lines 61-66):
 *
 *   weightedCost = sum(1 / (1 + μ_p(α))) for each color α in the tile
 *
 * Where μ_p(α) is the PALETTE-LOCAL multiplicity (count of tiles in this palette containing color α).
 *
 * Interpretation:
 *   - Colors already well-represented in the palette (high μ_p) contribute less cost
 *   - Colors new to the palette (μ_p = 0) contribute maximum cost (1.0 each)
 *   - Lower total cost means better color overlap with the palette
 *
 * Used by both Best Fusion and Overload-and-Remove for placement decisions: tiles are placed on the palette with the
 * LOWEST weighted cost.
 *
 * @param tile_colors The ColorSet of the tile to be placed
 * @param palette The candidate palette to evaluate
 * @param tile_colors_map Map from tile ID to ColorSet for ALL known tiles
 * @return Weighted cost (lower is better, minimum is tile_size when perfect overlap)
 *
 * @see build_palette_local_multiplicity Which provides the μ_p values
 */
[[nodiscard]] double compute_weighted_cost_in_palette(
    const ColorSet &tile_colors,
    const PackedPalette &palette,
    const std::map<PackableTile::Id, ColorSet> &tile_colors_map);

/**
 * @brief Computes the palette-local efficiency of a tile within its palette.
 *
 * @details
 * This implements the "actualEfficiencies" metric from the reference implementation (solver_tools.py, line 94):
 *
 *   efficiency = 1 - (weightedCost / tile_size)
 *
 * Where weightedCost uses PALETTE-LOCAL multiplicity.
 *
 * Interpretation:
 *   - Efficiency = 0: No sharing (all colors unique to this tile in the palette)
 *   - Efficiency = 1: Perfect sharing (all colors shared with many other tiles)
 *
 * Used by Overload-and-Remove for REMOVAL decisions: when a palette is overloaded, the tile with the LOWEST efficiency
 * (worst color sharing within THIS palette) is removed first. This corresponds to the paper's criterion of minimizing
 * the |t|/|t|_p ratio.
 *
 * @param tile_colors The ColorSet of the tile to evaluate
 * @param local_mult The palette-local multiplicity map (from build_palette_local_multiplicity)
 * @return Efficiency in [0, 1] (higher means better sharing within this palette)
 *
 * @see build_palette_local_multiplicity Which provides the local_mult parameter
 */
[[nodiscard]] double
compute_palette_local_efficiency(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &local_mult);

} // namespace porytiles2
