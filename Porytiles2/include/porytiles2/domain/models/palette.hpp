#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "porytiles2/domain/models/palette_index.hpp"
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
     * @brief Get the slot 0 palette color.
     *
     * @details
     * The slot 0 color is effectively unused by the GBA. Any tile pixel with value 0 will be treated as transparent by
     * the hardware, thus the value in any palette's 0 slot is basically ignored. Some community tools exploit this by
     * utilizing slot 0 for configuration, metadata, etc. (see .pla blend colors). Porytiles should respect user slot 0
     * preferences and do its best to ignore the value set here. It should also avoid clobbering it at all costs. Users
     * communicate transparency information in Porytiles assets by utilizing the intrinsic/extrinsic transparency
     * concept.
     *
     * @pre Palette must have size >= 1
     * @return The ColorType in pal slot 0
     */
    [[nodiscard]] ColorType slot_zero_color() const
    {
        if (size() == 0) {
            panic("palette had zero size");
        }
        return colors_.at(0);
    }

    /**
     * @brief Creates a map from colors to their palette indices.
     *
     * @details
     * Builds a lookup table that maps each color in the palette to its corresponding index position. Slot 0 is
     * explicitly excluded because it is reserved for the transparent color and handled separately.
     *
     * @return A map from ColorType to PaletteIndex for indices 1 through size()-1
     */
    [[nodiscard]] std::map<ColorType, PaletteIndex> color_to_index_map() const
    {
        std::map<ColorType, PaletteIndex> result{};
        for (std::size_t i = 1; i < size(); i++) {
            result.emplace(colors_.at(i), PaletteIndex{static_cast<unsigned int>(i)});
        }
        return result;
    }

    /**
     * @brief Creates a map from palette indices to their colors.
     *
     * @details
     * Builds a lookup table that maps each palette index to its corresponding color. Slot 0 is explicitly excluded
     * because it is reserved for the transparent color and handled separately.
     *
     * @return A map from PaletteIndex to ColorType for indices 1 through size()-1
     */
    [[nodiscard]] std::map<PaletteIndex, ColorType> index_to_color_map() const
    {
        std::map<PaletteIndex, ColorType> result{};
        for (std::size_t i = 1; i < size(); i++) {
            result.emplace(PaletteIndex{static_cast<unsigned int>(i)}, colors_.at(i));
        }
        return result;
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
