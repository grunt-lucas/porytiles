#pragma once

#include <map>
#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief A builder service for creating color-to-index mappings from \link NormalizedTile NormalizedTiles \endlink.
 *
 * @details
 * The ColorIndexMapBuilder processes collections of normalized tiles to generate mappings from RGBA color values to
 * their corresponding global color index. The global color indices are used to construct bins for the VM packing step.
 *
 * The builder validates that all colors are opaque and not transparent according to the specified extrinsic
 * transparency color before creating the mapping. It will panic if any values aren't purely opaque, so previous
 * compilation steps must ensure colors are regularized before constructing the color index map.
 */
class ColorIndexMapBuilder {
  public:
    /**
     * @brief Builds a color-to-index mapping from a collection of normalized tiles.
     *
     * @details
     * Processes all colors from the palettes of the provided tiles and assigns sequential indices to unique colors.
     * Colors are validated to ensure they are opaque and not matching the extrinsic transparency color. The resulting
     * map can be used to construct bins for the VM packing step.
     *
     * @param tiles The collection of normalized tiles to process
     * @param extrinsic The extrinsic transparency color to check against
     * @return A mapping from RGBA colors to their assigned palette indices
     * @throws panic if any color is transparent or matches the extrinsic color
     */
    // [[nodiscard]] std::map<Rgba32, unsigned int>
    // build_map(const std::vector<NormalizedTile<Rgba32>> &tiles, const Rgba32 &extrinsic) const;
};

} // namespace porytiles2
