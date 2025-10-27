#pragma once

#include <algorithm>
#include <array>

#include "porytiles2/domain/models/supports_transparency.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

namespace tile {

inline constexpr std::size_t side_length_pix = 8;
inline constexpr std::size_t size_pix = side_length_pix * side_length_pix;

} // namespace tile

/**
 * @brief An 8x8 tile backed by literal-array-based per-pixel storage of an arbitrary pixel type.
 *
 * @details
 * PixelTile represents tiles using direct per-pixel storage in a 64-element array (8x8 grid). Each pixel position
 * in the tile explicitly stores a PixelType value, providing a straightforward one-to-one mapping between tile
 * coordinates and pixel data. This contrasts with ShapeTile, which uses a mask-based representation that maps shape
 * regions to pixel values.
 *
 * This representation is ideal for:
 * - Direct pixel-level manipulation and access operations
 * - Simple tile transformations like flipping that operate on individual pixels
 * - Straightforward conversion from image data formats
 *
 * The PixelType template parameter must satisfy the SupportsTransparency concept, meaning it must provide methods
 * for checking whether a pixel is transparent. This enables transparency checking at both the intrinsic level (for
 * pixel types like IndexPixel that have built-in transparency) and extrinsic level (for pixel types like Rgba32
 * where transparency is determined by comparison with a reference value).
 *
 * Key features:
 * - Direct indexed access via at() methods supporting both linear and 2D coordinate access
 * - Boundary-checked access with panic on out-of-bounds indices
 * - Flipping operations that create new tiles with transformed pixel positions
 * - Transparency checking with overloads for both intrinsic and extrinsic transparency
 * - Full equality and comparison support via defaulted spaceship operator
 *
 * @invariant Default-constructed PixelTile is transparent (satisfies SupportsTransparency design invariant). That is,
 * `PixelTile<PixelType>{}` produces a tile where all pixels are default-constructed (and thus transparent, assuming
 * PixelType itself satisfies the invariant).
 *
 * @tparam PixelType The pixel type of this tile; must satisfy SupportsTransparency concept
 */
template <typename PixelType>
    requires SupportsTransparency<PixelType>
class PixelTile {
  public:
    virtual ~PixelTile() = default;

    PixelTile() : pix_{} {}

    explicit PixelTile(const std::array<PixelType, tile::size_pix> &pix) : pix_{pix} {}

    auto operator<=>(const PixelTile &) const = default;

    /**
     * @brief Checks if this entire PixelTile is transparent (intrinsic transparency only).
     *
     * @details
     * A PixelTile is transparent if all of its pixels are intrinsically transparent. This overload is only available
     * for pixel types that support parameterless is_transparent() (e.g., IndexPixel).
     *
     * @return True if all pixels in the PixelTile are transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent() const
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        return std::ranges::all_of(pix(), [](const auto &pixel) { return pixel.is_transparent(); });
    }

    /**
     * @brief Checks if this entire PixelTile is transparent.
     *
     * @details
     * A PixelTile is transparent if all of its pixels are either intrinsically or extrinsically transparent, according
     * to the provided extrinsic transparency value. This overload is only available for pixel types that support
     * extrinsic transparency (e.g., Rgba32).
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return True if all pixels in the PixelTile are transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent(const PixelType &extrinsic) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel.is_transparent(extrinsic); });
    }

    [[nodiscard]] PixelType at(std::size_t i) const
    {
        if (i >= tile::size_pix) {
            panic("index out of bounds: " + std::to_string(i));
        }
        return pix_[i];
    }

    [[nodiscard]] PixelType at(std::size_t row, std::size_t col) const
    {
        if (row >= tile::side_length_pix) {
            panic("row index out of bounds: " + std::to_string(row));
        }
        if (col >= tile::side_length_pix) {
            panic("col index out of bounds: " + std::to_string(col));
        }
        return pix_[row * tile::side_length_pix + col];
    }

    void set(std::size_t i, const PixelType &p)
    {
        if (i >= tile::size_pix) {
            panic("index out of bounds: " + std::to_string(i));
        }
        pix_[i] = p;
    }

    void set(std::size_t row, std::size_t col, const PixelType &p)
    {
        if (row >= tile::side_length_pix) {
            panic("row index out of bounds: " + std::to_string(row));
        }
        if (col >= tile::side_length_pix) {
            panic("col index out of bounds: " + std::to_string(col));
        }
        pix_[row * tile::side_length_pix + col] = p;
    }

    /**
     * @brief Creates a flipped copy of this PixelTile.
     *
     * @details
     * Returns a new PixelTile flipped according to the specified parameters. Horizontal flip reflects the tile across a
     * vertical axis, vertical flip reflects across a horizontal axis.
     *
     * @param h_flip Whether to flip horizontally
     * @param v_flip Whether to flip vertically
     * @return A new PixelTile with the specified flips applied
     */
    [[nodiscard]] PixelTile flip(bool h_flip, bool v_flip) const
    {
        PixelTile flipped_tile{};
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const std::size_t src_row = v_flip ? (tile::side_length_pix - 1 - row) : row;
                const std::size_t src_col = h_flip ? (tile::side_length_pix - 1 - col) : col;
                flipped_tile.set(row, col, at(src_row, src_col));
            }
        }
        return flipped_tile;
    }

    [[nodiscard]] const std::array<PixelType, tile::size_pix> &pix() const
    {
        return pix_;
    }

  private:
    std::array<PixelType, tile::size_pix> pix_;
};

} // namespace porytiles2
