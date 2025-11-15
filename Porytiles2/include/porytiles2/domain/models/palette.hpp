#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles2/domain/models/supports_transparency.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

namespace pal {

inline constexpr std::size_t max_size = 16;

inline constexpr std::size_t num_pals = 16;

} // namespace pal

/**
 * @brief A palette container for colors that support transparency checking.
 *
 * @details
 * Palette is a simple vector-backed container for storing colors that satisfy the SupportsTransparency concept. It
 * provides basic operations for adding, setting, and accessing colors in the palette. The palette has no hard size
 * limit, but is typically used with palettes of size 16 (the GBA standard palette size).
 *
 * The ColorType template parameter must satisfy the SupportsTransparency concept, meaning it must provide methods for
 * checking whether a color is transparent.
 *
 * @tparam ColorType The color type of this palette must satisfy the SupportsTransparency concept
 */
template <SupportsTransparency ColorType>
class Palette {
  public:
    /**
     * @brief Default constructs an empty Palette.
     */
    Palette() = default;

    /**
     * @brief Construct this palette with a given color vector.
     * @param colors The color vector
     */
    explicit Palette(std::vector<ColorType> colors) : colors_(std::move(colors)) {}

    /**
     * @brief Constructs a Palette filled with a single color.
     *
     * @details
     * Creates a palette containing 16 copies of the provided color. This is useful for creating uniform palettes or
     * placeholder palettes.
     *
     * @param color The color to fill the palette with
     */
    explicit Palette(ColorType color)
    {
        for (std::size_t i = 0; i < pal::max_size; i++) {
            add(color);
        }
    }

    /**
     * @brief Adds a color to the end of the palette.
     *
     * @details
     * Appends the provided color to the palette's internal color vector.
     *
     * @param color The color to add
     */
    void add(ColorType color)
    {
        colors_.push_back(color);
    }

    /**
     * @brief Sets the color at a specific index in the palette.
     *
     * @details
     * Replaces the color at the given index with the provided color. Panics if the index is out of bounds.
     *
     * @param color The color to set
     * @param index The index at which to set the color
     */
    void set(ColorType color, std::size_t index)
    {
        if (index >= size()) {
            panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
        }
        colors_.at(index) = color;
    }

    /**
     * @brief Returns the number of colors in the palette.
     *
     * @return The size of the palette
     */
    [[nodiscard]] std::size_t size() const
    {
        return colors_.size();
    }

    /**
     * @brief Returns a const reference to the internal color vector.
     *
     * @return Const reference to the vector of colors
     */
    [[nodiscard]] const std::vector<ColorType> &colors() const
    {
        return colors_;
    }

  private:
    std::vector<ColorType> colors_;
};

} // namespace porytiles2
