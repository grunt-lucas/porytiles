#include "porytiles/domain/packing/models/packed_palette.hpp"

#include <algorithm>
#include <ranges>

#include "porytiles/domain/models/color_set.hpp"

namespace porytiles {

PackedPalette::PackedPalette(std::size_t hardware_index, std::size_t capacity)
    : hardware_index_{hardware_index}, capacity_{capacity}, color_set_{}, assigned_tile_ids_{},
      cached_per_color_multiplicity_{}
{
}

std::size_t PackedPalette::color_count() const
{
    return color_set_count(color_set_);
}

std::size_t PackedPalette::remaining_capacity() const
{
    const std::size_t count = color_count();
    return count >= capacity_ ? 0 : capacity_ - count;
}

bool PackedPalette::can_fit(const ColorSet &tile_colors) const
{
    return union_size(tile_colors) <= capacity_;
}

std::size_t PackedPalette::union_size(const ColorSet &tile_colors) const
{
    ColorSet combined = color_set_union(color_set_, tile_colors);
    return color_set_count(combined);
}

void PackedPalette::add_tile(const PackableTile &tile)
{
    assigned_tile_ids_.push_back(tile.id());
    tile_colors_[tile.id()] = tile.color_set();
    color_set_ = color_set_union(color_set_, tile.color_set());

    // Incrementally update color counts for O(1) multiplicity lookups
    for_each_color(tile.color_set(), [this](std::size_t color_idx) { cached_per_color_multiplicity_[color_idx]++; });
}

void PackedPalette::remove_tile(const PackableTile &tile)
{
    remove_tile(tile.id());
}

void PackedPalette::remove_tile(const PackableTile::Id &tile_id)
{
    // Decrement color counts before removing the tile's colors
    if (const auto it = tile_colors_.find(tile_id); it != tile_colors_.end()) {
        for_each_color(it->second, [this](std::size_t color_idx) { cached_per_color_multiplicity_[color_idx]--; });
    }

    auto iter = std::ranges::find(assigned_tile_ids_, tile_id);
    if (iter != assigned_tile_ids_.end()) {
        assigned_tile_ids_.erase(iter);
    }
    tile_colors_.erase(tile_id);

    // Rebuild color_set_ from remaining tiles
    color_set_ = ColorSet{};
    for (const auto &colors : tile_colors_ | std::views::values) {
        color_set_ = color_set_union(color_set_, colors);
    }
}

} // namespace porytiles
