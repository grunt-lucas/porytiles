#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>

namespace porytiles {

/// @brief Represents an indexed color pixel.
///
/// @details
/// In indexed color mode, each pixel stores an index into a palette rather than a direct color value. By convention,
/// palette index 0 is always the transparent color.
///
/// @invariant Default-constructed IndexPixel is transparent (satisfies SupportsTransparency design invariant). That is,
/// `IndexPixel{}` produces a transparent pixel value with index 0.
class IndexPixel {
  public:
    IndexPixel() : index_{0} {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    IndexPixel(std::size_t index) : index_{index}
    {
        // No invariant check on `index_ < palette::max_size` here, because in true-color output mode we accept up to
        // 8-bit index pixels. Even though pokeemerald only cares about the lower four bits, the upper four bits encode
        // the palette index, which is what lets us save a non-greyscale png.
    }

    bool operator==(const IndexPixel &other) const = default;

    auto operator<=>(const IndexPixel &) const = default;

    /// @brief Checks if this indexed pixel is transparent.
    ///
    /// @details
    /// In indexed color mode, color index 0 is conventionally the transparent color. For true-color encoded pixels
    /// (where the full 8-bit value encodes both palette and color index), this checks only the lower 4 bits (the color
    /// index within the palette), since that's what determines transparency in the GBA hardware.
    ///
    /// @return True if the color index (lower 4 bits) is 0, false otherwise
    [[nodiscard]] bool is_transparent() const
    {
        return color_index() == 0;
    }

    /// @brief Returns the raw index value.
    ///
    /// @details
    /// For standard 4-bit indexed pixels, this returns the palette color index (0-15). For true-color encoded pixels,
    /// this returns the full 8-bit value where upper 4 bits are the palette index and lower 4 bits are the color index.
    ///
    /// @return The raw index value
    [[nodiscard]] std::size_t index() const
    {
        return index_;
    }

    /// @brief Returns the color index within a palette (lower 4 bits).
    ///
    /// @details
    /// In true-color mode, the full 8-bit index encodes both the palette index (upper 4 bits) and the color index
    /// within that palette (lower 4 bits). This method extracts just the color index, which is what GBA hardware uses
    /// to look up colors in the selected palette.
    ///
    /// For standard 4-bit indexed pixels (values 0-15), this returns the same value as index().
    ///
    /// @return The color index within the palette (0-15)
    [[nodiscard]] std::size_t color_index() const
    {
        return index_ & 0x0F;
    }

    /// @brief Returns the palette index (upper 4 bits).
    ///
    /// @details
    /// In true-color mode, the full 8-bit index encodes both the palette index (upper 4 bits) and the color index
    /// within that palette (lower 4 bits). This method extracts the palette index, which identifies which of the 16
    /// possible palettes this pixel's color belongs to.
    ///
    /// For standard 4-bit indexed pixels (values 0-15), this always returns 0.
    ///
    /// @return The palette index (0-15)
    [[nodiscard]] std::size_t palette_index() const
    {
        return index_ >> 4;
    }

  private:
    std::size_t index_;
};

} // namespace porytiles