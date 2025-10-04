#include "porytiles2/domain/model/iso_flip_tile.hpp"

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

bool IsoFlipTile::is_isomorphic(const IsoFlipTile &other) const
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
    // Compare palettes (extrinsic transparency and colors)
    if (pal_.extrinsic_transparency() != other.pal_.extrinsic_transparency()) {
        panic("unmatched extrinsic transparencies");
    }

    return pal_.colors() == other.pal_.colors();
}

} // namespace porytiles2
