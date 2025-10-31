#pragma once

#include <map>
#include <optional>
#include <vector>

#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/supports_transparency.hpp"

namespace porytiles2 {

/**
 * @brief A bidirectional mapping between pixel color values and sequential integer indices.
 *
 * @details
 * ColorIndexMap provides a true bidirectional association between pixel color values and sequential integer indices,
 * starting from 0. The class maintains two internal maps:
 * - Color-to-index mapping: look up an index given a color
 * - Index-to-color mapping: look up a color given an index
 *
 * This class is used for palette operations where each unique non-transparent color in a collection of tiles needs to
 * be assigned a unique integer identifier, with the ability to perform lookups in both directions.
 *
 * The mapping is constructed by examining all unique non-transparent colors across a collection of tiles, filtering
 * out both intrinsically transparent pixels (alpha=0) and extrinsically transparent pixels (e.g., magenta for Rgba32).
 * Each unique color that passes the transparency filter is assigned the next available sequential index.
 *
 * Key features:
 * - True bidirectional lookup (color -> index and index -> color)
 * - Sequential index assignment starting from 0
 * - Automatic filtering of transparent pixels during construction
 * - Deduplication of colors across multiple tiles
 * - Efficient lookup via std::map in both directions
 * - Generic support for any pixel type satisfying SupportsTransparency concept
 *
 * @tparam PixelType The pixel color type, must satisfy SupportsTransparency concept
 *
 * Example usage:
 * @code{.cpp}
 * std::vector<PixelTile<Rgba32>> tiles = {...};
 * ColorIndexMap<Rgba32> color_map{tiles, rgba_magenta};
 *
 * // Forward lookup: color -> index
 * auto index_opt = color_map.index_at_color(Rgba32{255, 0, 0});
 * if (index_opt) {
 *     // index_opt contains the index for red
 * }
 *
 * // Reverse lookup: index -> color
 * auto color_opt = color_map.color_at_index(0);
 * if (color_opt) {
 *     // color_opt contains the color at index 0
 * }
 * @endcode
 */
template <SupportsTransparency PixelType>
class ColorIndexMap {
  public:
    ColorIndexMap() = default;

    /**
     * @brief Constructs a ColorIndexMap from a collection of tiles and an extrinsic transparency value.
     *
     * @details
     * This constructor analyzes all provided tiles to identify unique non-transparent colors and assigns each a
     * sequential integer index starting from 0. The process:
     *
     * 1. Iterates through each tile in the collection
     * 2. Extracts unique non-transparent colors from each tile (using PixelTile::unique_nontransparent_colors)
     * 3. Filters colors based on both intrinsic transparency (alpha=0) and extrinsic transparency (matching the
     *    provided extrinsic parameter)
     * 4. Assigns each unique color that passes filtering a sequential index (0, 1, 2, ...)
     * 5. Ensures deduplication: colors appearing in multiple tiles receive the same index
     *
     * The order of index assignment depends on the order in which colors are encountered during tile iteration.
     *
     * @param tiles A vector of tiles with the specified pixel type to analyze for unique colors
     * @param extrinsic The extrinsic transparency value used for transparency filtering
     */
    ColorIndexMap(const std::vector<PixelTile<PixelType>> &tiles, const PixelType &extrinsic)
    {
        std::map<PixelType, unsigned int> pixel_indexes{};
        std::map<unsigned int, PixelType> index_to_color{};

        unsigned int color_index = 0;
        for (const auto &tile : tiles) {
            for (const auto &pixel : tile.unique_nontransparent_colors(extrinsic)) {
                if (pixel_indexes.insert({pixel, color_index}).second) {
                    index_to_color.insert({color_index, pixel});
                    color_index++;
                }
            }
        }

        index_map_ = pixel_indexes;
        color_map_ = index_to_color;
    }

    /**
     * @brief Returns the number of unique colors in the mapping.
     *
     * @details
     * Returns the count of unique non-transparent colors that were identified during construction. This is equivalent
     * to the size of both the color-to-index and index-to-color maps.
     *
     * @return The number of unique colors mapped
     */
    [[nodiscard]] std::size_t size() const
    {
        return index_map_.size();
    }

    /**
     * @brief Determines if the mapping is empty.
     *
     * @return Whether the mapping is empty
     */
    [[nodiscard]] bool empty() const
    {
        return size() == 0;
    }

    /**
     * @brief Retrieves the color associated with a given index.
     *
     * @details
     * Performs a lookup in the index-to-color mapping to find the pixel color assigned to the specified index. If
     * the index exists in the map, returns the associated color. If the index does not exist (was never assigned
     * during construction), returns std::nullopt.
     *
     * This method enables efficient reverse lookup: given an index, retrieve its color.
     *
     * @param index The integer index to lookup
     * @return std::optional<PixelType> containing the color if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<PixelType> color_at_index(unsigned int index) const
    {
        auto it = color_map_.find(index);
        if (it != color_map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Retrieves the index associated with a given color.
     *
     * @details
     * Performs a lookup in the color-to-index mapping to find the integer index assigned to the specified pixel color.
     * If the color exists in the map, returns the associated index. If the color does not exist (was never assigned
     * during construction or was filtered out as transparent), returns std::nullopt.
     *
     * This method enables efficient forward lookup: given a color, retrieve its index.
     *
     * @param color The pixel color to lookup
     * @return std::optional<unsigned int> containing the index if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<unsigned int> index_at_color(const PixelType &color) const
    {
        auto it = index_map_.find(color);
        if (it != index_map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

  private:
    std::map<PixelType, unsigned int> index_map_;
    std::map<unsigned int, PixelType> color_map_;
};

} // namespace porytiles2
