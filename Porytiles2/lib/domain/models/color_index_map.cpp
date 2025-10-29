#include "porytiles2/domain/models/color_index_map.hpp"

#include <optional>
#include <vector>

#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

ColorIndexMap::ColorIndexMap(const std::vector<PixelTile<Rgba32>> &tiles, const Rgba32 &extrinsic)
{
    std::map<Rgba32, unsigned int> rgba_indexes{};
    std::map<unsigned int, Rgba32> index_to_color{};

    unsigned int color_index = 0;
    for (const auto &tile : tiles) {
        for (const auto &rgb : tile.unique_nontransparent_colors(extrinsic)) {
            if (rgba_indexes.insert({rgb, color_index}).second) {
                index_to_color.insert({color_index, rgb});
                color_index++;
            }
        }
    }

    index_map_ = rgba_indexes;
    color_map_ = index_to_color;
}

std::size_t ColorIndexMap::size() const
{
    return index_map_.size();
}

bool ColorIndexMap::empty() const
{
    return size() == 0;
}

std::optional<Rgba32> ColorIndexMap::color_at_index(unsigned int index) const
{
    auto it = color_map_.find(index);
    if (it != color_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<unsigned int> ColorIndexMap::index_at_color(const Rgba32 &color) const
{
    auto it = index_map_.find(color);
    if (it != index_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace porytiles2
