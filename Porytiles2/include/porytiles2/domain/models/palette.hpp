#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
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
 * @brief A generic palette container for colors that support transparency checking.
 *
 * @details
 * Palette is a container for storing colors that satisfy the SupportsTransparency concept. It supports both dynamic
 * (vector-backed) and fixed-size (array-backed) storage based on the template parameter N.
 *
 * When N == 0 (the default), the palette uses std::vector for dynamic storage, allowing colors to be added
 * incrementally. When N > 0, the palette uses std::array<T, N> for fixed-size storage with compile-time size
 * guarantees.
 *
 * Both variants support "wildcard" slots - palette positions that have no assigned color. Wildcards are represented
 * internally as std::nullopt. Client code can use wildcards to represent partially-filled palettes or to allow
 * flexible color assignment.
 *
 * The ColorType template parameter must satisfy the SupportsTransparency concept, meaning it must provide methods for
 * checking whether a color is transparent.
 *
 * @tparam ColorType The color type of this palette must satisfy the SupportsTransparency concept
 * @tparam N The fixed size of the palette (0 for dynamic sizing, default)
 */
template <SupportsTransparency ColorType, std::size_t N = 0>
class Palette {
  public:
    /**
     * @brief The storage type used internally, selected based on N.
     *
     * @details
     * When N == 0, uses std::vector for dynamic sizing. When N > 0, uses std::array for fixed-size storage. Both use
     * std::optional to support wildcard slots.
     */
    using StorageType =
        std::conditional_t<N == 0, std::vector<std::optional<ColorType>>, std::array<std::optional<ColorType>, N>>;

    /**
     * @brief Default constructs a Palette.
     *
     * @details
     * For dynamic palettes (N == 0), creates an empty palette. For fixed-size palettes (N > 0), creates a palette with
     * N wildcard slots (all std::nullopt).
     */
    Palette() = default;

    /**
     * @brief Construct this palette with a given color vector.
     *
     * @details
     * Only available for dynamic palettes (N == 0). Each color in the input vector becomes a non-wildcard slot.
     *
     * @param colors The color vector
     */
    explicit Palette(std::vector<ColorType> colors)
        requires(N == 0)
    {
        for (auto &color : colors) {
            colors_.push_back(std::move(color));
        }
    }

    /**
     * @brief Construct this palette from an array of colors.
     *
     * @details
     * Only available for fixed-size palettes (N > 0). Each color in the input array becomes a non-wildcard slot.
     *
     * @param colors The color array
     */
    explicit Palette(std::array<ColorType, N> colors)
        requires(N > 0)
    {
        for (std::size_t i = 0; i < N; i++) {
            colors_[i] = std::move(colors[i]);
        }
    }

    /**
     * @brief Constructs a Palette filled with a single color.
     *
     * @details
     * For dynamic palettes (N == 0), creates a palette containing 16 copies of the provided color. For fixed-size
     * palettes (N > 0), fills all N slots with the provided color. No slots are wildcards after this construction.
     *
     * @param color The color to fill the palette with
     */
    explicit Palette(ColorType color)
    {
        if constexpr (N == 0) {
            for (std::size_t i = 0; i < pal::max_size; i++) {
                colors_.push_back(color);
            }
        }
        else {
            for (std::size_t i = 0; i < N; i++) {
                colors_[i] = color;
            }
        }
    }

    /**
     * @brief Adds a color to the end of the palette.
     *
     * @details
     * Appends the provided color as a non-wildcard slot at the end of the palette. This method is only available for
     * dynamic palettes (N == 0).
     *
     * @param color The color to add
     */
    void add(ColorType color)
    {
        static_assert(N == 0, "add() is not available for fixed-size Palette");
        colors_.push_back(color);
    }

    /**
     * @brief Adds a wildcard slot to the end of the palette.
     *
     * @details
     * Appends a wildcard slot (std::nullopt) at the end of the palette. This method is only available for dynamic
     * palettes (N == 0).
     */
    void add_wildcard()
    {
        static_assert(N == 0, "add_wildcard() is not available for fixed-size Palette");
        colors_.push_back(std::nullopt);
    }

    /**
     * @brief Sets the color at a specific index in the palette.
     *
     * @details
     * Replaces the slot at the given index with the provided color, making it a non-wildcard slot. Panics if the index
     * is out of bounds.
     *
     * @param index The index at which to set the color
     * @param color The color to set
     * @pre index must be less than size()
     */
    void set(std::size_t index, ColorType color)
    {
        if (index >= size()) {
            panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
        }
        colors_.at(index) = color;
    }

    /**
     * @brief Sets a slot as a wildcard at a specific index.
     *
     * @details
     * Replaces the slot at the given index with a wildcard (std::nullopt). Panics if the index is out of bounds.
     *
     * @param index The index at which to set the wildcard
     * @pre index must be less than size()
     */
    void set_wildcard(std::size_t index)
    {
        if (index >= size()) {
            panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
        }
        colors_.at(index) = std::nullopt;
    }

    /**
     * @brief Checks if a slot is a wildcard.
     *
     * @details
     * Returns true if the slot at the given index is a wildcard (has no assigned color). Panics if the index is out of
     * bounds.
     *
     * @param index The index to check
     * @pre index must be less than size()
     * @return true if the slot is a wildcard, false otherwise
     */
    [[nodiscard]] bool is_wildcard(std::size_t index) const
    {
        if (index >= size()) {
            panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
        }
        return !colors_.at(index).has_value();
    }

    /**
     * @brief Checks if the palette contains any wildcard slots.
     *
     * @return true if any slot is a wildcard, false if all slots have colors
     */
    [[nodiscard]] bool has_any_wildcards() const
    {
        for (std::size_t i = 0; i < size(); i++) {
            if (!colors_.at(i).has_value()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Returns the number of slots in the palette.
     *
     * @details
     * For dynamic palettes (N == 0), returns the current vector size. For fixed-size palettes (N > 0), returns N.
     *
     * @return The size of the palette
     */
    [[nodiscard]] std::size_t size() const
    {
        if constexpr (N == 0) {
            return colors_.size();
        }
        else {
            return N;
        }
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
     * @pre Slot 0 must not be a wildcard
     * @return The ColorType in pal slot 0
     */
    [[nodiscard]] ColorType slot_zero_color() const
    {
        if (size() == 0) {
            panic("palette had zero size");
        }
        if (!colors_.at(0).has_value()) {
            panic("slot 0 is a wildcard");
        }
        return colors_.at(0).value();
    }

    /**
     * @brief Gets the color at a specific index.
     *
     * @details
     * Returns the color at the given index. Panics if the index is out of bounds or if the slot is a wildcard.
     *
     * @param index The index to get
     * @pre index must be less than size()
     * @pre The slot at index must not be a wildcard
     * @return The color at the given index
     */
    [[nodiscard]] ColorType at(std::size_t index) const
    {
        if (index >= size()) {
            panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
        }
        if (!colors_.at(index).has_value()) {
            panic("slot " + std::to_string(index) + " is a wildcard");
        }
        return colors_.at(index).value();
    }

    /**
     * @brief Gets the optional color at a specific index.
     *
     * @details
     * Returns the optional color at the given index, which may be std::nullopt if the slot is a wildcard. Panics if
     * the index is out of bounds.
     *
     * @param index The index to get
     * @pre index must be less than size()
     * @return The optional color at the given index
     */
    [[nodiscard]] std::optional<ColorType> at_optional(std::size_t index) const
    {
        if (index >= size()) {
            panic("index " + std::to_string(index) + " >= size " + std::to_string(size()));
        }
        return colors_.at(index);
    }

    /**
     * @brief Creates a map from colors to their palette indices.
     *
     * @details
     * Builds a lookup table that maps each non-wildcard color in the palette to its corresponding index position.
     * Slot 0 is explicitly excluded because it is reserved for the transparent color and handled separately. Wildcard
     * slots are also excluded.
     *
     * @warning Duplicate Color Handling: Since std::map stores only one value per key, if the palette contains
     * duplicate colors at multiple indices, only the first occurrence (lowest index) will be stored in the map. For
     * example, if slots 7 and 14 both contain the same RGBA color, the map will only contain an entry for slot 7.
     *
     * This can cause issues when comparing tiles that were indexed using different palette slots for the same color.
     * If you need to match tiles where the palette may have duplicate colors, use color-equivalence comparison (compare
     * the actual palette colors at each index) rather than direct index comparison. See
     * TilesPngWorkspace::find_existing_contiguous_tiles_by_color() for an example of this pattern.
     *
     * @return A map from ColorType to PaletteIndex for non-wildcard indices 1 through size()-1
     */
    [[nodiscard]] std::map<ColorType, PaletteIndex> color_to_index_map() const
    {
        std::map<ColorType, PaletteIndex> result{};
        for (std::size_t i = 1; i < size(); i++) {
            if (colors_.at(i).has_value()) {
                result.emplace(colors_.at(i).value(), PaletteIndex{i});
            }
        }
        return result;
    }

    /**
     * @brief Creates a map from palette indices to their colors.
     *
     * @details
     * Builds a lookup table that maps each non-wildcard palette index to its corresponding color. Slot 0 is explicitly
     * excluded because it is reserved for the transparent color and handled separately. Wildcard slots are also
     * excluded.
     *
     * @return A map from PaletteIndex to ColorType for non-wildcard indices 1 through size()-1
     */
    [[nodiscard]] std::map<PaletteIndex, ColorType> index_to_color_map() const
    {
        std::map<PaletteIndex, ColorType> result{};
        for (std::size_t i = 1; i < size(); i++) {
            if (colors_.at(i).has_value()) {
                result.emplace(PaletteIndex{i}, colors_.at(i).value());
            }
        }
        return result;
    }

  private:
    StorageType colors_{};
};

} // namespace porytiles2
