#include "porytiles2/domain/packing/algorithms/multiplicity_map_builder.hpp"

namespace porytiles2 {

std::map<std::size_t, std::size_t>
build_multiplicity_map(const std::vector<PackableTile> &tiles, const std::vector<PackableTile> &hints)
{
    std::map<std::size_t, std::size_t> multiplicity;

    auto count_colors = [&multiplicity](const std::vector<PackableTile> &tile_list) {
        for (const auto &tile : tile_list) {
            for_each_color(tile.color_set(), [&multiplicity](std::size_t color_idx) { multiplicity[color_idx]++; });
        }
    };

    count_colors(tiles);
    count_colors(hints);

    return multiplicity;
}

} // namespace porytiles2
