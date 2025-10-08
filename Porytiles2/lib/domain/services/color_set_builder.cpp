#include "porytiles2/domain/services/color_set_builder.hpp"

#include <map>

#include "porytiles2/domain/model/color_set.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

// ColorSet
// ColorSetBuilder::build(const NormalizedPal<Rgba32> &pal, const std::map<Rgba32, unsigned int> &color_index_map) const
// {
//     /*
//      * Set a ColorSet based on a given palette. Each bit in the ColorSet represents if the color at the given index
//      in
//      * the supplied color map was present in the palette. E.g., suppose the color map has 12 unique colors. The
//      supplied
//      * palette has two colors in it, which correspond to index 2 and index 11. The ColorSet bitset would be:
//      * 0010 0000 0001
//      */
//     ColorSet color_set{};
//     for (const auto &color : pal.colors()) {
//         if (!color_index_map.contains(color)) {
//             panic(format_->format("color_index_map did not contain requested color: {}", FormatParam{color}));
//         }
//         color_set.set(color_index_map.at(color));
//     }
//     return color_set;
// }

} // namespace porytiles2
