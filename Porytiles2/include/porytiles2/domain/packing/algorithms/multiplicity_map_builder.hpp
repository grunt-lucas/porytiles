#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace porytiles2 {

/**
 * @brief Builds a map from color index to multiplicity (how many tiles use that color).
 *
 * @details
 * In bin packing with overlapping items (the "Pagination" problem from Grange et al. 2017), the multiplicity μ(α) of a
 * symbol α counts the number of tiles that contain that symbol. This metric is fundamental to several packing
 * heuristics:
 *
 * - **Best Fusion**: Uses 1/multiplicity as a weighting factor when computing the cost of adding a tile to a palette.
 *   Colors with high multiplicity (shared by many tiles) have lower individual cost because they're likely to be reused
 *   when other tiles are added to the same palette.
 *
 * - **Overload-and-Remove**: Uses the size/relative_size ratio (where relative_size = Σ(1/μ(α)) for colors α in the
 *   tile) to decide which tiles to remove from an overloaded palette.
 *
 * For example, if color index 42 appears in 5 different tiles, then multiplicity[42] = 5.
 *
 * @param tiles The regular tiles to count colors from
 * @param hints The hint tiles to count colors from (processed the same as regular tiles)
 * @return Map from color index to the number of tiles containing that color
 */
[[nodiscard]] std::map<std::size_t, std::size_t>
build_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints);

} // namespace porytiles2
