#pragma once

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/ordered_pal.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/tile.hpp"

namespace porytiles2 {

/**
 * @brief A canonical tile representation designed to detect isomorphism under flip transformations.
 *
 * @details
 * IsoFlipTile represents an 8x8 indexed tile with an associated ordered palette and flip state. This class is
 * designed to facilitate the detection of tiles that are isomorphic under flip transformations - that is, tiles that
 * can be transformed into one another through a sequence of horizontal and/or vertical flips.
 *
 * TODO: describe normalization technique needed to properly construct an IsoFlipTile.
 *
 * This normalization ensures that tiles which are isomorphic under flip transformations will have identical pixel data
 * and identical palettes (as sets), allowing them to share the same GBA tile representation with different flip bits.
 */
class IsoFlipTile final : public Tile<IndexPixel> {
  public:
    IsoFlipTile(bool h_flip, bool v_flip, const Rgba32 &extrinsic) : pal_{extrinsic}, h_flip_{h_flip}, v_flip_{v_flip}
    {
    }

    /**
     * @brief Checks if this tile is isomorphic to another tile under flip transformations.
     *
     * @details
     * Two IsoFlipTiles are isomorphic under flip transformations if there exists a sequence of flip operations that
     * can transform one tile into the other. This method checks if the normalized tile pixels and palettes are
     * identical, ignoring the flip bits. Tiles that are isomorphic under flip transformations can share the same GBA
     * tile representation.
     *
     * @param other The other IsoFlipTile to compare against
     * @return True if the tiles are isomorphic under flip transformations, false otherwise
     */
    [[nodiscard]] bool is_isomorphic(const IsoFlipTile &other) const;

    /**
     * @brief Returns the horizontal flip state of the tile.
     *
     * @return True if the tile is horizontally flipped, false otherwise
     */
    [[nodiscard]] bool h_flip() const
    {
        return h_flip_;
    }

    /**
     * @brief Returns the vertical flip state of the tile.
     *
     * @return True if the tile is vertically flipped, false otherwise
     */
    [[nodiscard]] bool v_flip() const
    {
        return v_flip_;
    }

  private:
    OrderedPal pal_;
    bool h_flip_;
    bool v_flip_;
};

} // namespace porytiles2
