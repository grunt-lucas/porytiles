#include "porytiles2/domain/packing/algorithms/packing_metrics.hpp"

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

double compute_relative_size(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &multiplicity)
{
    double relative_size = 0.0;

    for_each_color(tile_colors, [&relative_size, &multiplicity](std::size_t color_idx) {
        auto it = multiplicity.find(color_idx);
        std::size_t mult = (it != multiplicity.end()) ? it->second : 1;
        relative_size += 1.0 / static_cast<double>(mult);
    });

    return relative_size;
}

double compute_efficiency(const ColorSet &tile_colors, const std::map<std::size_t, std::size_t> &multiplicity)
{
    std::size_t color_count = color_set_count(tile_colors);
    if (color_count == 0) {
        return 1.0;
    }

    double relative_size = compute_relative_size(tile_colors, multiplicity);
    return 1.0 - (relative_size / static_cast<double>(color_count));
}

} // namespace porytiles2
