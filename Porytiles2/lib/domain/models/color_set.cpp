#include "porytiles2/domain/models/color_set.hpp"

namespace porytiles2 {

bool ColorSet::test(ColorIndex index) const
{
    return colors_.test(index.index());
}

void ColorSet::set(ColorIndex index, bool value)
{
    colors_.set(index.index(), value);
}

void ColorSet::reset(ColorIndex index)
{
    colors_.reset(index.index());
}

ColorSet color_set_union(const ColorSet &a, const ColorSet &b)
{
    ColorSet result;
    auto union_bits = a.colors() | b.colors();
    for (std::size_t i = 0; i < num_colors; ++i) {
        if (union_bits.test(i)) {
            result.set(ColorIndex{static_cast<std::uint8_t>(i)});
        }
    }
    return result;
}

ColorSet color_set_intersection(const ColorSet &a, const ColorSet &b)
{
    ColorSet result;
    auto intersection_bits = a.colors() & b.colors();
    for (std::size_t i = 0; i < num_colors; ++i) {
        if (intersection_bits.test(i)) {
            result.set(ColorIndex{static_cast<std::uint8_t>(i)});
        }
    }
    return result;
}

std::size_t color_set_count(const ColorSet &set)
{
    return set.colors().count();
}

bool is_subset(const ColorSet &a, const ColorSet &b)
{
    // a is a subset of b if every bit in a is also in b
    // This is equivalent to: (a & ~b) == 0
    // Or: (a & b) == a
    auto intersection = a.colors() & b.colors();
    return intersection == a.colors();
}

std::size_t intersection_size(const ColorSet &a, const ColorSet &b)
{
    return color_set_count(color_set_intersection(a, b));
}

std::size_t union_size(const ColorSet &a, const ColorSet &b)
{
    return color_set_count(color_set_union(a, b));
}

} // namespace porytiles2
