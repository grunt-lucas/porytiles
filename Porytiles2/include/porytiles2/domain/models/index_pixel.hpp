#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>

namespace porytiles2 {

/**
 * @brief Represents an indexed color pixel.
 *
 * @details
 * In indexed color mode, each pixel stores an index into a palette rather than a direct color value. By convention,
 * palette index 0 is always the transparent color.
 *
 * @invariant Default-constructed IndexPixel is transparent (satisfies SupportsTransparency design invariant). That is,
 * `IndexPixel{}` produces a transparent pixel value with index 0.
 */
class IndexPixel {
  public:
    IndexPixel() : index_{0} {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    IndexPixel(std::size_t index) : index_{index}
    {
        /*
         * Why don't we check this here? Because in true-color output mode, we want to allow up to 8-bit index pixels.
         * Even though pokeemerald only cares about the lower four bits, the upper four bits contain the pal index which
         * is what allows us to save a non-greyscale png.
         */
        // TODO: make an IndexPixel4 and IndexPixel8 for really clear domain separation
        // if (index_ >= pal::max_size) {
        //     panic("invalid IndexPixel value: " + std::to_string(index_));
        // }
    }

    bool operator==(const IndexPixel &other) const = default;

    auto operator<=>(const IndexPixel &) const = default;

    /**
     * @brief Checks if this indexed pixel is transparent.
     *
     * @details
     * In indexed color mode, palette index 0 is conventionally the transparent color.
     *
     * @return True if the palette index is 0, false otherwise
     */
    [[nodiscard]] bool is_transparent() const
    {
        return index_ == 0;
    }

    [[nodiscard]] std::size_t index() const
    {
        return index_;
    }

  private:
    std::size_t index_;
};

} // namespace porytiles2