#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace porytiles2 {

/**
 * @brief Builds a map from color index to multiplicity (how many tiles use that color).
 *
 * @details
 * In bin packing with overlapping items (the "Pagination" problem), the multiplicity μ(α) of a symbol α counts the
 * number of tiles that contain that symbol. This metric is fundamental to the packing heuristics described in:
 *   Grange, Kacem, Martin. "Algorithms for the Bin Packing Problem with Overlapping Items" (2017)
 *   https://arxiv.org/abs/1605.00558
 *
 * The multiplicity is used to compute both relative_size and efficiency metrics (see below).
 *
 * For example, if color index 42 appears in 5 different tiles, then multiplicity[42] = 5.
 *
 * @param tiles The regular tiles to count colors from.
 * @param hints The hint tiles to count colors from (processed the same as regular tiles).
 * @return Map from color index to the number of tiles containing that color.
 *
 * @see compute_relative_size
 * @see compute_efficiency
 */
[[nodiscard]] std::map<std::size_t, std::size_t>
build_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints);

/**
 * @brief Computes the relative size of a tile based on color multiplicity.
 *
 * @details
 * This implements the "relative size" metric from Definition 2.5 of:
 *   Grange, Kacem, Martin. "Algorithms for the Bin Packing Problem with Overlapping Items" (2017)
 *   https://arxiv.org/abs/1605.00558
 *
 * Relative size |t|_p = sum(1 / μ(α)) for each color α in the tile, where μ(α) is the multiplicity
 * (number of tiles containing that color). A tile with all unique colors has relative_size == size.
 * A tile sharing all colors has relative_size approaching 0.
 *
 * Used in the Overload-and-Remove algorithm (Section 3.3) for placement decisions: tiles are placed
 * on the palette where their relative size is minimal, maximizing color sharing benefit.
 *
 * @param tile_colors The color set of the tile to evaluate.
 * @param multiplicity Map from color index to its multiplicity (occurrence count across all tiles).
 * @return The relative size of the tile (lower means better sharing potential).
 *
 * @see build_multiplicity_map
 * @see compute_efficiency
 */
[[nodiscard]] double
compute_relative_size(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &multiplicity);

/**
 * @brief Computes the efficiency of a tile in a palette.
 *
 * @details
 * This implements the "actualEfficiencies" metric from the reference implementation:
 *   https://github.com/pagination-problem/pagination (solver_tools.py, line 94)
 *
 * Efficiency = 1 - (relative_size / tile_size) = 1 - |t|_p / |t|
 *
 * This is a normalized metric derived from the relative size (Definition 2.5 of Grange et al.):
 *   - Efficiency = 0: No sharing (all colors unique, relative_size == tile_size)
 *   - Efficiency = 1: Perfect sharing (all colors infinitely shared, relative_size -> 0)
 *
 * Used in the Overload-and-Remove algorithm (Section 3.3) for removal decisions: when a palette
 * is overloaded, the tile with the LOWEST efficiency (worst color sharing) is removed first.
 * This corresponds to the paper's criterion of minimizing |t|/|t|_p ratio.
 *
 * @param tile_colors The color set of the tile to evaluate.
 * @param multiplicity Map from color index to its multiplicity (occurrence count across all tiles).
 * @return The efficiency of the tile in [0, 1] (higher means better sharing).
 *
 * @see build_multiplicity_map
 * @see compute_relative_size
 */
[[nodiscard]] double
compute_efficiency(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &multiplicity);

} // namespace porytiles2
