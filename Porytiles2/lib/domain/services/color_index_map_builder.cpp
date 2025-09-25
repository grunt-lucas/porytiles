#include "porytiles2/domain/services/color_index_map_builder.hpp"

#include <map>
#include <vector>

#include "porytiles2/domain/model/normalized_tile.hpp"
#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

std::map<Rgba32, unsigned int> ColorIndexMapBuilder::build_map(const std::vector<NormalizedTile<Rgba32>> &tiles) const
{
    return {};
}

} // namespace porytiles2
