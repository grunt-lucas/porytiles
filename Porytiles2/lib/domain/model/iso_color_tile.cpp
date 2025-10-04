#include "porytiles2/domain/model/iso_color_tile.hpp"

#include <optional>
#include <unordered_map>

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

bool IsoColorTile::is_isomorphic(const IsoColorTile &other) const
{
    // Compare tile pixels using parent class comparison
    const auto &parent_this = static_cast<const Tile &>(*this);
    const auto &parent_other = static_cast<const Tile &>(other);

    if (parent_this != parent_other) {
        return false;
    }

    /*
     * TODO: does this make sense? Theoretically, the domain allows for different extrinsic transparencies. But in
     * practice, this should never happen. Porytiles should enforce a single, uniform extrinsic transparency during
     * compilation.
     */
    // Compare palettes (extrinsic transparency)
    if (pal_.extrinsic_transparency() != other.pal_.extrinsic_transparency()) {
        panic("unmatched extrinsic transparencies");
    }

    // For iso-under-color, palettes must NOT be simple reorderings of each other
    // If they have identical content (same colors, different order), it's iso-under-flip instead
    if (pal_.has_identical_content(other.pal_)) {
        return false;
    }

    return true;
}

std::optional<std::unordered_map<Rgba32, Rgba32>> IsoColorTile::get_isomorphism(const IsoColorTile &other) const
{
    if (!is_isomorphic(other)) {
        return std::nullopt;
    }

    std::unordered_map<Rgba32, Rgba32> mapping;
    const auto &this_colors = pal_.colors();
    const auto &other_colors = other.pal_.colors();

    // Build the color mapping by aligning palette indices
    for (std::size_t i = 0; i < this_colors.size(); ++i) {
        mapping[this_colors[i]] = other_colors[i];
    }

    return mapping;
}

} // namespace porytiles2