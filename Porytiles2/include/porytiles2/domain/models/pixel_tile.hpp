#pragma once

#include <algorithm>
#include <array>
#include <set>

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

    explicit PixelTile(std::array<PixelType, tile::size_pix> pix) : pix_{std::move(pix)} {}

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

    /**
     * @brief Compares this PixelTile with another, treating all transparent pixels as equal.
     *
     * @details
     * This method provides a semantic equality comparison that differs from operator== in that it considers all
     * transparent pixels to be equal, regardless of their underlying representation. Two pixels are considered equal
     * if:
     * - Both are transparent (via intrinsic is_transparent()), OR
     * - Neither is transparent and they compare equal via operator==
     *
     * This is useful when comparing tiles where only the logical transparency state matters, not the specific pixel
     * values. The practical application depends on the pixel type's transparency semantics. For IndexPixel, since only
     * index 0 is intrinsically transparent, this method behaves like operator== in typical usage. However, for pixel
     * types with more complex intrinsic transparency (e.g., hypothetical future types supporting multiple transparent
     * representations), this method would treat all such representations as equal.
     *
     * This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
     *
     * @param other The PixelTile to compare against
     * @return True if the tiles are equivalent under transparency-ignoring semantics, false otherwise
     */
    [[nodiscard]] bool equals_ignoring_transparency(const PixelTile &other) const
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        for (std::size_t i = 0; i < tile::size_pix; ++i) {
            const auto &pixel1 = pix_.at(i);
            const auto &pixel2 = other.pix_.at(i);

            if (pixel1.is_transparent() && pixel2.is_transparent()) {
                continue; // Both transparent, consider equal
            }
            if (pixel1 != pixel2) {
                return false; // Different pixels
            }
        }
        return true;
    }

    /**
     * @brief Compares this PixelTile with another, treating all transparent pixels as equal.
     *
     * @details
     * This method provides a semantic equality comparison that differs from operator== in that it considers all
     * transparent pixels to be equal, regardless of their underlying representation. Two pixels are considered equal
     * if:
     * - Both are transparent (via extrinsic is_transparent(extrinsic)), OR
     * - Neither is transparent and they compare equal via operator==
     *
     * This is useful when comparing tiles where the specific representation of transparent pixels doesn't matter,
     * only the logical transparency state. For example, a tile with Rgba32{255,0,255} (magenta, considered
     * extrinsically transparent under default settings) and another with Rgba32{0,0,0,0} (black with alpha=0, also
     * transparent) would be considered equal at those positions when using the appropriate extrinsic transparency
     * value.
     *
     * This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32).
     *
     * @param other The PixelTile to compare against
     * @param extrinsic The extrinsic transparency value to use when checking pixel transparency
     * @return True if the tiles are equivalent under transparency-ignoring semantics, false otherwise
     */
    [[nodiscard]] bool equals_ignoring_transparency(const PixelTile &other, const PixelType &extrinsic) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        for (std::size_t i = 0; i < tile::size_pix; ++i) {
            const auto &pixel1 = pix_.at(i);
            const auto &pixel2 = other.pix_.at(i);

            if (pixel1.is_transparent(extrinsic) && pixel2.is_transparent(extrinsic)) {
                continue; // Both transparent, consider equal
            }
            if (pixel1 != pixel2) {
                return false; // Different pixels
            }
        }
        return true;
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

    /**
     * @brief Returns the set of unique non-transparent colors present in this PixelTile (intrinsic transparency only).
     *
     * @details
     * This method constructs and returns a std::set containing all unique non-transparent PixelType values found in the
     * tile. Pixels are filtered using their intrinsic transparency (via parameterless is_transparent()), and only
     * non-transparent pixels are included in the result. The set automatically handles uniqueness, so duplicate pixel
     * values will only appear once.
     *
     * This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel). For
     * IndexPixel tiles, this means index 0 (intrinsically transparent) will be excluded from the returned set.
     *
     * @return A std::set containing all unique non-transparent pixel values in this tile
     */
    [[nodiscard]] std::set<PixelType> unique_nontransparent_colors() const
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        std::set<PixelType> colors;
        for (const auto &pixel : pix_) {
            if (!pixel.is_transparent()) {
                colors.insert(pixel);
            }
        }
        return colors;
    }

    /**
     * @brief Returns the set of unique non-transparent colors present in this PixelTile.
     *
     * @details
     * This method constructs and returns a std::set containing all unique non-transparent PixelType values found in the
     * tile. Pixels are filtered using both intrinsic and extrinsic transparency (via is_transparent(extrinsic)), and
     * only non-transparent pixels are included in the result. The set automatically handles uniqueness, so duplicate
     * pixel values will only appear once.
     *
     * This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32). For Rgba32
     * tiles:
     * - Pixels with alpha=0 (intrinsically transparent) are excluded
     * - Pixels matching the extrinsic transparency value (e.g., magenta) are excluded
     * - Only opaque, non-transparent pixels are included in the result
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return A std::set containing all unique non-transparent pixel values in this tile
     */
    [[nodiscard]] std::set<PixelType> unique_nontransparent_colors(const PixelType &extrinsic) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        std::set<PixelType> colors;
        for (const auto &pixel : pix_) {
            if (!pixel.is_transparent(extrinsic)) {
                colors.insert(pixel);
            }
        }
        return colors;
    }

    [[nodiscard]] const std::array<PixelType, tile::size_pix> &pix() const
    {
        return pix_;
    }

  private:
    std::array<PixelType, tile::size_pix> pix_;
};

} // namespace porytiles2
