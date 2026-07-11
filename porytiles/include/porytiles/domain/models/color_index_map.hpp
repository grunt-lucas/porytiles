#pragma once

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/color_index.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/supports_transparency.hpp"

namespace porytiles {

/// @brief A bidirectional mapping between pixel color values and sequential integer indices.
///
/// @details
/// ColorIndexMap provides a true bidirectional association between pixel color values and sequential integer indices,
/// starting from 0. The class maintains two internal maps:
/// - Color-to-index mapping: look up an index given a color
/// - Index-to-color mapping: look up a color given an index
///
/// This class is used for palette operations where each unique non-transparent color in a collection of tiles needs to
/// be assigned a unique integer identifier, with the ability to perform lookups in both directions.
///
/// The mapping is constructed by examining all unique non-transparent colors across a collection of tiles, filtering
/// out both intrinsically transparent pixels (alpha=0) and extrinsically transparent pixels (e.g., magenta for Rgba32).
/// Each unique color that passes the transparency filter is assigned the next available sequential index.
///
/// @tparam PixelType The pixel color type, must satisfy SupportsTransparency concept
///
/// Example usage:
/// @code{.cpp}
/// std::vector<PixelTile<Rgba32>> tiles = {...};
/// ColorIndexMap<Rgba32> color_map{};
/// for (const auto &tile : tiles) {
///     color_map.add_tile(tile, rgba_magenta);
/// }
///
/// // Forward lookup: color -> index
/// auto index_opt = color_map.index_at_color(Rgba32{255, 0, 0});
/// if (index_opt) {
///     // index_opt contains the ColorIndex for red
/// }
///
/// // Reverse lookup: index -> color
/// auto color_opt = color_map.color_at_index(ColorIndex{0});
/// if (color_opt) {
///     // color_opt contains the color at ColorIndex 0
/// }
/// @endcode
template <SupportsTransparency PixelType>
class ColorIndexMap {
  public:
    ColorIndexMap() = default;

    /// @brief Returns the number of unique colors in the mapping.
    ///
    /// @details
    /// Returns the count of unique non-transparent colors that were identified during construction. This is equivalent
    /// to the size of both the color-to-index and index-to-color maps.
    ///
    /// @return The number of unique colors mapped
    [[nodiscard]] std::size_t size() const
    {
        return index_map_.size();
    }

    /// @brief Determines if the mapping is empty.
    ///
    /// @return Whether the mapping is empty
    [[nodiscard]] bool empty() const
    {
        return size() == 0;
    }

    /// @brief Adds colors from a single tile to the mapping using intrinsic transparency.
    ///
    /// @details
    /// Extracts unique non-transparent colors from the provided tile and adds any new colors to the mapping. Colors
    /// that already exist in the mapping are skipped. New colors are assigned sequential indices starting from the
    /// current size of the mapping.
    ///
    /// This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
    ///
    /// @param tile The tile to extract colors from
    void add_tile(const PixelTile<PixelType> &tile)
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        add_colors_impl(tile.unique_nontransparent_colors());
    }

    /// @brief Adds colors from a single tile to the mapping using extrinsic transparency.
    ///
    /// @details
    /// Extracts unique non-transparent colors from the provided tile and adds any new colors to the mapping. Colors
    /// that already exist in the mapping are skipped. New colors are assigned sequential indices starting from the
    /// current size of the mapping.
    ///
    /// This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32).
    ///
    /// @param tile The tile to extract colors from
    /// @param extrinsic The extrinsic transparency value used for transparency filtering
    void add_tile(const PixelTile<PixelType> &tile, const PixelType &extrinsic)
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        add_colors_impl(tile.unique_nontransparent_colors(extrinsic));
    }

    /// @brief Adds colors from a fixed-size palette to the mapping using intrinsic transparency.
    ///
    /// @details
    /// Extracts non-transparent colors from the provided palette and adds any new colors to the mapping. Wildcards and
    /// transparent colors are skipped. Colors that already exist in the mapping are also skipped. New colors are
    /// assigned sequential indices starting from the current size of the mapping.
    ///
    /// This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
    ///
    /// @param pal The palette to extract colors from
    void add_pal(const Palette<PixelType, pal::max_size> &pal)
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        add_colors_from_pal_impl(pal, [](const PixelType &p) { return p.is_transparent(); });
    }

    /// @brief Adds colors from a fixed-size palette to the mapping using extrinsic transparency.
    ///
    /// @details
    /// Extracts non-transparent colors from the provided palette and adds any new colors to the mapping. Wildcards and
    /// transparent colors are skipped. Colors that already exist in the mapping are also skipped. New colors are
    /// assigned sequential indices starting from the current size of the mapping.
    ///
    /// This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32).
    ///
    /// @param pal The palette to extract colors from
    /// @param extrinsic The extrinsic transparency value used for transparency filtering
    void add_pal(const Palette<PixelType, pal::max_size> &pal, const PixelType &extrinsic)
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        add_colors_from_pal_impl(pal, [&extrinsic](const PixelType &p) { return p.is_transparent(extrinsic); });
    }

    /// @brief Adds colors from a dynamic palette to the mapping using intrinsic transparency.
    ///
    /// @details
    /// Extracts non-transparent colors from the provided palette and adds any new colors to the mapping. Wildcards and
    /// transparent colors are skipped. Colors that already exist in the mapping are also skipped. New colors are
    /// assigned sequential indices starting from the current size of the mapping.
    ///
    /// This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
    ///
    /// @param pal The palette to extract colors from
    void add_pal(const Palette<PixelType> &pal)
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        add_colors_from_pal_impl(pal, [](const PixelType &p) { return p.is_transparent(); });
    }

    /// @brief Adds colors from a dynamic palette to the mapping using extrinsic transparency.
    ///
    /// @details
    /// Extracts non-transparent colors from the provided palette and adds any new colors to the mapping. Wildcards and
    /// transparent colors are skipped. Colors that already exist in the mapping are also skipped. New colors are
    /// assigned sequential indices starting from the current size of the mapping.
    ///
    /// This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32).
    ///
    /// @param pal The palette to extract colors from
    /// @param extrinsic The extrinsic transparency value used for transparency filtering
    void add_pal(const Palette<PixelType> &pal, const PixelType &extrinsic)
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        add_colors_from_pal_impl(pal, [&extrinsic](const PixelType &p) { return p.is_transparent(extrinsic); });
    }

    /// @brief Adds colors from an animation to the mapping using intrinsic transparency.
    ///
    /// @details
    /// Iterates through all tiles in the animation's key frame (if present) and all regular frames, adding any new
    /// colors to the mapping. Colors that already exist in the mapping are skipped. New colors are assigned sequential
    /// indices starting from the current size of the mapping.
    ///
    /// This overload is only available for pixel types that support intrinsic transparency (e.g., IndexPixel).
    ///
    /// @param anim The animation to extract colors from
    void add_anim(const Animation<PixelType> &anim)
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        if (anim.has_key_frame()) {
            for (const auto &tile : anim.key_frame().tiles()) {
                add_tile(tile);
            }
        }
        for (const auto &[name, frame] : anim.frames()) {
            for (const auto &tile : frame.tiles()) {
                add_tile(tile);
            }
        }
    }

    /// @brief Adds colors from an animation to the mapping using extrinsic transparency.
    ///
    /// @details
    /// Iterates through all tiles in the animation's key frame (if present) and all regular frames, adding any new
    /// colors to the mapping. Colors that already exist in the mapping are skipped. New colors are assigned sequential
    /// indices starting from the current size of the mapping.
    ///
    /// This overload is only available for pixel types that support extrinsic transparency (e.g., Rgba32).
    ///
    /// @param anim The animation to extract colors from
    /// @param extrinsic The extrinsic transparency value used for transparency filtering
    void add_anim(const Animation<PixelType> &anim, const PixelType &extrinsic)
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        if (anim.has_key_frame()) {
            for (const auto &tile : anim.key_frame().tiles()) {
                add_tile(tile, extrinsic);
            }
        }
        for (const auto &[name, frame] : anim.frames()) {
            for (const auto &tile : frame.tiles()) {
                add_tile(tile, extrinsic);
            }
        }
    }

    /// @brief Retrieves the color associated with a given index.
    ///
    /// @details
    /// Performs a lookup in the index-to-color mapping to find the pixel color assigned to the specified index. If the
    /// index exists in the map, returns the associated color. If the index does not exist (was never assigned during
    /// construction), returns std::nullopt.
    ///
    /// This method enables efficient reverse lookup: given an index, retrieve its color.
    ///
    /// @param index The color index to lookup
    /// @return std::optional<PixelType> containing the color if found, std::nullopt otherwise
    [[nodiscard]] std::optional<PixelType> color_at_index(ColorIndex index) const
    {
        auto it = color_map_.find(index);
        if (it != color_map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /// @brief Retrieves the index associated with a given color.
    ///
    /// @details
    /// Performs a lookup in the color-to-index mapping to find the color index assigned to the specified pixel color.
    /// If the color exists in the map, returns the associated index. If the color does not exist (was never assigned
    /// during construction or was filtered out as transparent), returns std::nullopt.
    ///
    /// This method enables efficient forward lookup: given a color, retrieve its index.
    ///
    /// @param color The pixel color to lookup
    /// @return std::optional<ColorIndex> containing the index if found, std::nullopt otherwise
    [[nodiscard]] std::optional<ColorIndex> index_at_color(const PixelType &color) const
    {
        auto it = index_map_.find(color);
        if (it != index_map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

  private:
    /// @brief Helper method to add colors from a set to the mapping.
    ///
    /// @details
    /// Iterates through the provided set of colors and adds any colors not already in the mapping. New colors are
    /// assigned sequential indices starting from the current size of the mapping.
    ///
    /// @param colors The set of colors to add
    void add_colors_impl(const std::set<PixelType> &colors)
    {
        auto color_index = size();
        for (const auto &pixel : colors) {
            if (index_map_.insert({pixel, ColorIndex{color_index}}).second) {
                color_map_.insert({ColorIndex{color_index}, pixel});
                ++color_index;
            }
        }
    }

    /// @brief Helper method to add colors from a palette to the mapping.
    ///
    /// @details
    /// Iterates through all slots in the palette, skipping wildcards and transparent colors. Any new non-transparent
    /// colors are added to the mapping with sequential indices.
    ///
    /// @tparam N The size template parameter of the palette (0 for dynamic, > 0 for fixed)
    /// @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
    /// @param pal The palette to extract colors from
    /// @param is_transparent_pred A predicate function that returns true if a color is transparent
    template <std::size_t N, typename TransparencyPredicate>
    void add_colors_from_pal_impl(const Palette<PixelType, N> &pal, TransparencyPredicate is_transparent_pred)
    {
        auto color_index = size();
        for (std::size_t i = 0; i < pal.size(); i++) {
            auto color_opt = pal.at_optional(i);
            if (!color_opt.has_value()) {
                continue; // Skip wildcards
            }
            const auto &color = color_opt.value();
            if (is_transparent_pred(color)) {
                continue; // Skip transparent colors
            }
            if (index_map_.insert({color, ColorIndex{color_index}}).second) {
                color_map_.insert({ColorIndex{color_index}, color});
                ++color_index;
            }
        }
    }

    std::map<PixelType, ColorIndex> index_map_;
    std::map<ColorIndex, PixelType> color_map_;
};

} // namespace porytiles
