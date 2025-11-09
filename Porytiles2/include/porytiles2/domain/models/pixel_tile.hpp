#pragma once

#include <algorithm>
#include <array>
#include <set>

#include "porytiles2/domain/models/supports_transparency.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

namespace tile {

inline constexpr std::size_t side_length_pix = 8;
inline constexpr std::size_t size_pix = side_length_pix * side_length_pix;

/**
 * @brief Converts a linear index to row and column coordinates.
 *
 * @details
 * Takes a linear index in the range [0, size_pix) and returns the corresponding row and column coordinates in an 8x8
 * tile grid. The row is the first element of the returned pair, and the column is the second element.
 *
 * @param index The linear index to convert
 * @return A pair containing (row, col) coordinates
 */
[[nodiscard]] constexpr std::pair<std::size_t, std::size_t> index_to_row_col(std::size_t index)
{
    return {index / side_length_pix, index % side_length_pix};
}

/**
 * @brief Converts row and column coordinates to a linear index.
 *
 * @details
 * Takes row and column coordinates in an 8x8 tile grid and returns the corresponding linear index in the range [0,
 * size_pix).
 *
 * @param row The row coordinate
 * @param col The column coordinate
 * @return The linear index
 */
[[nodiscard]] constexpr std::size_t row_col_to_index(std::size_t row, std::size_t col)
{
    return row * side_length_pix + col;
}

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
template <SupportsTransparency PixelType>
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
        return is_transparent_impl([](const PixelType &pixel) { return pixel.is_transparent(); });
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
        return is_transparent_impl([&extrinsic](const PixelType &pixel) { return pixel.is_transparent(extrinsic); });
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
        return equals_ignoring_transparency_impl(other, [](const PixelType &pixel) { return pixel.is_transparent(); });
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
        return equals_ignoring_transparency_impl(
            other, [&extrinsic](const PixelType &pixel) { return pixel.is_transparent(extrinsic); });
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
        return unique_nontransparent_colors_impl([](const PixelType &pixel) { return pixel.is_transparent(); });
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
        return unique_nontransparent_colors_impl(
            [&extrinsic](const PixelType &pixel) { return pixel.is_transparent(extrinsic); });
    }

    [[nodiscard]] const std::array<PixelType, tile::size_pix> &pix() const
    {
        return pix_;
    }

  private:
    /**
     * @brief Helper method implementing the core transparency checking logic.
     *
     * @details
     * This private helper contains the common transparency checking logic shared by both is_transparent() overloads.
     * It accepts a transparency predicate that determines whether a pixel is transparent, allowing the same
     * implementation to work with both intrinsic and extrinsic transparency checking.
     *
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return True if all pixels satisfy the transparency predicate, false otherwise
     */
    template <typename TransparencyPredicate>
    [[nodiscard]] bool is_transparent_impl(TransparencyPredicate is_transparent_pred) const
    {
        return std::ranges::all_of(pix(), is_transparent_pred);
    }

    /**
     * @brief Helper method implementing the core transparency-ignoring comparison logic.
     *
     * @details
     * This private helper contains the common comparison logic shared by both equals_ignoring_transparency() overloads.
     * It accepts a transparency predicate that determines whether a pixel is transparent, allowing the same
     * implementation to work with both intrinsic and extrinsic transparency checking.
     *
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param other The PixelTile to compare against
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return True if the tiles are equivalent under transparency-ignoring semantics, false otherwise
     */
    template <typename TransparencyPredicate>
    [[nodiscard]] bool
    equals_ignoring_transparency_impl(const PixelTile &other, TransparencyPredicate is_transparent_pred) const
    {
        for (std::size_t i = 0; i < tile::size_pix; ++i) {
            const auto &pixel1 = pix_.at(i);
            const auto &pixel2 = other.pix_.at(i);

            if (is_transparent_pred(pixel1) && is_transparent_pred(pixel2)) {
                continue; // Both transparent, consider equal
            }
            if (pixel1 != pixel2) {
                return false; // Different pixels
            }
        }
        return true;
    }

    /**
     * @brief Helper method implementing the core unique color extraction logic.
     *
     * @details
     * This private helper contains the common color extraction logic shared by both unique_nontransparent_colors()
     * overloads. It accepts a transparency predicate that determines whether a pixel is transparent, allowing the
     * same implementation to work with both intrinsic and extrinsic transparency checking.
     *
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return A std::set containing all unique non-transparent pixel values in this tile
     */
    template <typename TransparencyPredicate>
    [[nodiscard]] std::set<PixelType> unique_nontransparent_colors_impl(TransparencyPredicate is_transparent_pred) const
    {
        std::set<PixelType> colors;
        for (const auto &pixel : pix_) {
            if (!is_transparent_pred(pixel)) {
                colors.insert(pixel);
            }
        }
        return colors;
    }

    std::array<PixelType, tile::size_pix> pix_;
};

} // namespace porytiles2
