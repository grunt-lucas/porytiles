#include "porytiles2/domain/services/color_index_map_builder.hpp"

#include <map>
#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

// std::map<Rgba32, unsigned int>
// ColorIndexMapBuilder::build_map(const std::vector<NormalizedTile<Rgba32>> &tiles, const Rgba32 &extrinsic) const
// {
//     std::map<Rgba32, unsigned int> rgba_indexes{};
//
//     unsigned int color_index = 0;
//     for (const auto &tile : tiles) {
//         for (const auto &rgb : tile.palette().colors()) {
//             if (rgb.is_transparent(extrinsic) || rgb.alpha() != Rgba32::alpha_opaque) {
//                 panic("invalid rgba");
//             }
//             if (rgba_indexes.insert({rgb, color_index}).second) {
//                 color_index++;
//             }
//         }
//     }
//
//     return rgba_indexes;
// }

} // namespace porytiles2
