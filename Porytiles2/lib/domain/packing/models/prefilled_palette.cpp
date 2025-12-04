#include "porytiles2/domain/packing/models/prefilled_palette.hpp"

namespace porytiles2 {

PrefilledPalette::PrefilledPalette(std::size_t hardware_index, ColorSet fixed_colors, std::size_t available_capacity)
    : hardware_index_{hardware_index}, fixed_colors_{std::move(fixed_colors)}, available_capacity_{available_capacity}
{
}

PrefilledPalette PrefilledPalette::fully_locked(std::size_t hardware_index, ColorSet color_set)
{
    return PrefilledPalette{hardware_index, std::move(color_set), 0};
}

PrefilledPalette
PrefilledPalette::partially_locked(std::size_t hardware_index, ColorSet fixed_colors, std::size_t total_capacity)
{
    const std::size_t fixed_count = color_set_count(fixed_colors);
    if (fixed_count > total_capacity) {
        panic("PrefilledPalette::partially_locked fixed_count > total_capacity");
    }
    const std::size_t available = total_capacity - fixed_count;
    return PrefilledPalette{hardware_index, std::move(fixed_colors), available};
}

std::size_t PrefilledPalette::fixed_color_count() const
{
    return color_set_count(fixed_colors_);
}

bool PrefilledPalette::can_accommodate(const ColorSet &tile_colors) const
{
    // If tile colors are a subset of fixed colors, always OK
    if (is_subset(tile_colors, fixed_colors_)) {
        return true;
    }

    // Otherwise, check if new colors fit in available capacity
    // New colors = union size - fixed count
    ColorSet combined = color_set_union(tile_colors, fixed_colors_);
    std::size_t combined_count = color_set_count(combined);
    std::size_t fixed_count = color_set_count(fixed_colors_);
    std::size_t new_colors_needed = combined_count - fixed_count;

    return new_colors_needed <= available_capacity_;
}

} // namespace porytiles2
