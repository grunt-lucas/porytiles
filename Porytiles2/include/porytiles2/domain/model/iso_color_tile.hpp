#pragma once

#include <optional>
#include <unordered_map>

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/tile.hpp"
#include "porytiles2/domain/model/unordered_pal.hpp"

namespace porytiles2 {

/**
 * @brief A canonical tile representation designed to detect isomorphism under color transformations.
 *
 * @details
 * IsoColorTile represents an 8x8 indexed tile with an associated unordered palette and flip state. This class is
 * designed to facilitate the detection of tiles that are isomorphic under color transformations - that is, tiles that
 * have the same pixel pattern but with different color mappings.
 *
 * TODO: describe normalization technique needed to properly construct an IsoColorTile.
 *
 * This normalization ensures that tiles which are isomorphic under color transformations can be identified by checking
 * if they have identical pixel data but different palettes (where the palettes are not merely reorderings of each
 * other). These tiles represent "sibling" tiles - classic examples include Pokémon Center and Poké Mart roof tiles
 * which share the same shape but use different color schemes.
 */
class IsoColorTile final : public Tile<IndexPixel> {
  public:
    IsoColorTile(bool h_flip, bool v_flip, const Rgba32 &extrinsic) : pal_{extrinsic}, h_flip_{h_flip}, v_flip_{v_flip}
    {
    }

    /**
     * @brief Checks if this tile is isomorphic to another tile under color transformations.
     *
     * @details
     * Two IsoColorTiles are isomorphic under color transformations if they have identical normalized pixel patterns
     * but different color palettes. The palettes must not be simple reorderings of each other (which would indicate
     * isomorphism under flip transformations instead). This method checks if there exists a color mapping function F
     * that can transform one tile's colors into the other's.
     *
     * @param other The other IsoColorTile to compare against
     * @return True if the tiles are isomorphic under color transformations, false otherwise
     */
    [[nodiscard]] bool is_isomorphic(const IsoColorTile &other) const;

    /**
     * @brief Returns the color mapping (isomorphism) from this tile to another tile.
     *
     * @details
     * If this tile and the other tile are isomorphic under color transformations, this function computes and returns
     * the mapping function F such that applying F to each color in this tile's palette produces the corresponding
     * color in the other tile's palette. For example, if this tile's palette is [R, G, B] and the other tile's
     * palette is [C, M, Y], the returned mapping will contain F(R)=C, F(G)=M, F(B)=Y.
     *
     * If the tiles are not isomorphic under color transformations, returns std::nullopt.
     *
     * @param other The other IsoColorTile to compute the mapping to
     * @return A mapping from this tile's colors to the other tile's colors, or std::nullopt if not isomorphic
     */
    [[nodiscard]] std::optional<std::unordered_map<Rgba32, Rgba32>> get_isomorphism(const IsoColorTile &other) const;

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
    UnorderedPal pal_;
    bool h_flip_;
    bool v_flip_;
};

} // namespace porytiles2