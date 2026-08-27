#pragma once

/// @file packing_metrics.hpp
/// @brief Metrics for Bin Packing with Overlapping Items (Pagination problem).
///
/// @details
/// This module provides metrics for the "Bin Packing with Overlapping Items" problem (also called the "Pagination"
/// problem), as described in:
///
/// > Grange, Kacem, Martin. "Algorithms for the Bin Packing Problem with
/// > Overlapping Items" (2017) - https://arxiv.org/abs/1605.00558
///
/// @section global_vs_local CRITICAL DISTINCTION: Global vs Palette-Local Multiplicity
///
/// @subsection global_mult 1. GLOBAL MULTIPLICITY (build_global_multiplicity_map, compute_average_multiplicity)
/// - Counts how many tiles across ALL INPUT TILES contain each color
/// - Computed ONCE at the start of packing
/// - DOES NOT vary per palette
/// - Use cases:
///   - **Problem difficulty estimation**: Average multiplicity (Card(T)/|A|) correlates strongly (r=0.784) with
///     problem difficulty per Section 4.3 of Grange et al.
///   - **Algorithm selection**: Different algorithms perform better at different multiplicity ranges (Section 4.4.2):
///     - Low multiplicity (<15): Overload-and-Remove competitive with genetic algorithms
///     - Medium multiplicity (15-35): Overload-and-Remove decent, genetic algorithms better
///     - High multiplicity (35-40): Greedy algorithms become competitive
///     - Very high multiplicity (>40): Best Fusion optimal for speed/quality tradeoff
///
/// @subsection local_mult 2. PALETTE-LOCAL MULTIPLICITY (build_palette_local_multiplicity)
/// - Counts how many tiles within a SPECIFIC PALETTE contain each color
/// - Computed PER-PALETTE, dynamically as tiles are added/removed
/// - VARIES per palette (each palette has different tiles assigned)
/// - Use case: Measuring color overlap when placing a tile in a palette
///
/// @warning The Best Fusion and Overload-and-Remove algorithms require PALETTE-LOCAL multiplicity to make placement
/// decisions. Using global multiplicity would cause all palettes to appear equally good, defeating the algorithm's
/// purpose.
///
/// @see BestFusionStrategy Uses palette-local cost for tile placement
/// @see OverloadAndRemoveStrategy Uses palette-local cost and efficiency for placement/removal

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles/domain/models/color_set.hpp"
#include "porytiles/domain/packing/models/packable_tile.hpp"
#include "porytiles/domain/packing/models/packed_palette.hpp"

namespace porytiles {

/// @brief Builds a GLOBAL multiplicity map from all input tiles.
///
/// @details
/// Computes μ(α) = count of tiles containing color α, across ALL input tiles. This is the multiplicity metric from
/// Definition 2.5 of Grange et al. (2017).
///
/// Example: If color index 42 appears in 5 different input tiles, then map[42] = 5.
///
/// Primary use cases (see Section 4.3-4.4 of Grange et al.):
///   - Computing average multiplicity for problem difficulty estimation
///   - Algorithm selection based on instance characteristics
///
/// @warning This function computes GLOBAL multiplicity across all tiles. For palette placement decisions, use
/// build_palette_local_multiplicity() instead. Global multiplicity does NOT help distinguish between palettes.
///
/// @param tiles The regular tiles to count colors from
/// @param hints The hint tiles to count colors from (processed same as regular tiles)
/// @return Map from color index to count of tiles containing that color
///
/// @see compute_average_multiplicity Convenience function that computes the scalar difficulty metric
/// @see build_palette_local_multiplicity For palette-specific multiplicity used in placement decisions
[[nodiscard]] std::map<std::size_t, std::size_t>
build_global_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints);

/// @brief Computes the average multiplicity of the input tiles (problem difficulty metric).
///
/// @details
/// Computes Card(T) / |A|, where:
///   - Card(T) = sum of all tile sizes (total color references across all tiles)
///   - |A| = number of unique colors
///
/// This metric is proposed in Section 4.3 of Grange et al. (2017) as a predictor of problem difficulty, with
/// r=0.784 correlation to actual algorithm performance variance.
///
/// Interpretation:
///   - Low values (~1-5): Problem approaches standard Bin Packing (little color sharing)
///   - Medium values (~5-20): Moderate color sharing, typical Pagination instances
///   - High values (>20): Dense color sharing, more challenging for heuristics
///
/// Algorithm selection guidance (Section 4.4.2):
///   - avg_mult < 15: Overload-and-Remove competitive with genetic algorithms
///   - 15 <= avg_mult < 35: Genetic algorithms preferred, Overload-and-Remove acceptable
///   - 35 <= avg_mult < 40: Greedy algorithms become competitive
///   - avg_mult >= 40: Best Fusion optimal for speed/quality tradeoff
///
/// @param tiles The regular tiles to analyze
/// @param hints The hint tiles to analyze
/// @return Average multiplicity (Card(T) / |A|), or 0.0 if no colors exist
///
/// @see build_global_multiplicity_map Which this function uses internally
[[nodiscard]] double
compute_average_multiplicity(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints);

/// @brief Builds a PALETTE-LOCAL multiplicity map for a specific palette.
///
/// @details
/// Computes μ_p(α) = count of tiles in palette p containing color α.
///
/// This is the key metric for the "weightBySymbol" calculation in the reference implementation (solver_tools.py, line
/// 92). Unlike global multiplicity, this varies per palette based on which tiles are currently assigned to it.
///
/// Example: If palette P has 3 assigned tiles, and 2 of them contain color 42, then the returned map has map[42] = 2.
///
/// @note The tile_colors_map must include entries for:
///   - All regular tiles (keyed by TileIndex)
///   - All hint tiles (keyed by TileIndex)
///   - All prefilled palette system tiles (keyed by PrefilledPaletteId)
///
/// @param palette The palette to analyze
/// @param tile_colors_map Map from tile ID to ColorSet for ALL known tiles
/// @return Map from color index to count of tiles in THIS palette containing that color
///
/// @see compute_weighted_cost_in_palette Which uses this for placement decisions
/// @see compute_palette_local_efficiency Which uses this for removal decisions
[[nodiscard]] std::map<std::size_t, std::size_t> build_palette_local_multiplicity(
    const PackedPalette &palette, const std::map<PackableTile::Id, ColorSet> &tile_colors_map);

/// @brief Computes the weighted cost of placing a tile in a specific palette.
///
/// @details
/// This implements the "weightedCostIn" metric from the reference implementation (solver_tools.py, lines 61-66):
///
///   weightedCost = sum(1 / (1 + μ_p(α))) for each color α in the tile
///
/// Where μ_p(α) is the PALETTE-LOCAL multiplicity (count of tiles in this palette containing color α).
///
/// Interpretation:
///   - Colors already well-represented in the palette (high μ_p) contribute less cost
///   - Colors new to the palette (μ_p = 0) contribute maximum cost (1.0 each)
///   - Lower total cost means better color overlap with the palette
///
/// Used by both Best Fusion and Overload-and-Remove for placement decisions: tiles are placed on the palette with the
/// LOWEST weighted cost.
///
/// @param tile_colors The ColorSet of the tile to be placed
/// @param palette The candidate palette to evaluate
/// @param tile_colors_map Map from tile ID to ColorSet for ALL known tiles
/// @return Weighted cost (lower is better, minimum is tile_size when perfect overlap)
///
/// @see build_palette_local_multiplicity Which provides the μ_p values
[[nodiscard]] double compute_weighted_cost_in_palette(
    const ColorSet &tile_colors,
    const PackedPalette &palette,
    const std::map<PackableTile::Id, ColorSet> &tile_colors_map);

/// @brief Computes the palette-local efficiency of a tile within its palette.
///
/// @details
/// This implements the "actualEfficiencies" metric from the reference implementation (solver_tools.py, line 94):
///
///   efficiency = 1 - (weightedCost / tile_size)
///
/// Where weightedCost uses PALETTE-LOCAL multiplicity.
///
/// Interpretation:
///   - Efficiency = 0: No sharing (all colors unique to this tile in the palette)
///   - Efficiency = 1: Perfect sharing (all colors shared with many other tiles)
///
/// Used by Overload-and-Remove for REMOVAL decisions: when a palette is overloaded, the tile with the LOWEST efficiency
/// (worst color sharing within THIS palette) is removed first. This corresponds to the paper's criterion of minimizing
/// the |t|/|t|_p ratio.
///
/// @param tile_colors The ColorSet of the tile to evaluate
/// @param local_mult The palette-local multiplicity map (from build_palette_local_multiplicity)
/// @return Efficiency in [0, 1] (higher means better sharing within this palette)
///
/// @see build_palette_local_multiplicity Which provides the local_mult parameter
[[nodiscard]] double
compute_palette_local_efficiency(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &local_mult);

/// @brief Computes the weighted cost of placing a tile in a palette using cached counts.
///
/// @details
/// This is an optimized version of compute_weighted_cost_in_palette() that uses the PackedPalette's cached color counts
/// array instead of rebuilding the multiplicity map. This provides O(colors) complexity instead of O(tiles × colors).
///
/// The formula is the same: weightedCost = sum(1 / (1 + μ_p(α))) for each color α in the tile.
///
/// @param tile_colors The ColorSet of the tile to be placed
/// @param palette The candidate palette (must have up-to-date color_counts)
/// @return Weighted cost (lower is better)
///
/// @see compute_weighted_cost_in_palette The non-cached version
[[nodiscard]] double compute_weighted_cost_in_palette_fast(const ColorSet &tile_colors, const PackedPalette &palette);

/// @brief Computes the palette-local efficiency of a tile using cached counts.
///
/// @details
/// This is an optimized version of compute_palette_local_efficiency() that uses the PackedPalette's cached color counts
/// array instead of a separately-built multiplicity map.
///
/// The formula is the same: efficiency = 1 - (weightedCost / tile_size).
///
/// @param tile_colors The ColorSet of the tile to evaluate
/// @param palette The palette containing the tile (must have up-to-date color_counts)
/// @return Efficiency in [0, 1] (higher means better sharing)
///
/// @see compute_palette_local_efficiency The non-cached version
[[nodiscard]] double compute_palette_local_efficiency_fast(const ColorSet &tile_colors, const PackedPalette &palette);

} // namespace porytiles
