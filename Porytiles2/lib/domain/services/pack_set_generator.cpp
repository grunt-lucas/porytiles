#include "porytiles2/domain/services/pack_set_generator.hpp"

#include <map>
#include <vector>

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/pack_set.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/services/color_set_builder.hpp"

namespace porytiles2 {

[[nodiscard]] std::vector<PackSet> PackSetGenerator::generate(
    const std::vector<NormalizedTile<Rgba32>> &norm_tiles, const std::map<Rgba32, unsigned int> &color_index_map) const
{
    std::vector<PackSet> assignable_tiles{};
    unsigned int tile_index = 0;
    for (const auto &norm_tile : norm_tiles) {
        const auto color_set = color_set_builder_->build(norm_tile.palette(), color_index_map);
        assignable_tiles.emplace_back(tile_index++, color_set);
    }
    return assignable_tiles;
}

} // namespace porytiles2
