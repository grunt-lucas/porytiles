#pragma once

#include <optional>
#include <string>
#include <vector>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/supports_transparency.hpp"

namespace porytiles {

/// @brief Represents a single frame of an animation, containing tiles and a frame name.
///
/// @details
/// AnimFrame holds the pixel data for one frame of an animation. Each frame consists of one or more 8x8 tiles
/// arranged in a grid. The frame name typically corresponds to the PNG filename (e.g., "0", "1", "2" for 0.png, 1.png,
/// 2.png).
///
/// The template parameter PixelType determines whether the frame uses RGBA pixels (for Porytiles component, source
/// format) or indexed pixels (for Porymap component, compiled format).
///
/// Key concepts:
/// - A special "key frame" that appears in tiles.png, but isn't part of the animation loop
///   - Users tell Porytiles which metatile subtiles should be animated by using key frame tiles on the layer PNGs
/// - Other frames (0, 1, 2, ...) are stored as separate .4bpp files and swapped at runtime
/// - All frames of an animation must have the same tile count
///
/// @tparam PixelType The pixel type for tiles; must satisfy SupportsTransparency concept
template <SupportsTransparency PixelType>
class AnimFrame {
  public:
    AnimFrame() = default;

    explicit AnimFrame(std::string frame_name) : frame_name_{std::move(frame_name)} {}

    AnimFrame(std::string frame_name, std::vector<PixelTile<PixelType>> tiles)
        : frame_name_{std::move(frame_name)}, tiles_{std::move(tiles)}
    {
    }

    [[nodiscard]] const std::string &frame_name() const
    {
        return frame_name_;
    }

    void frame_name(std::string name)
    {
        frame_name_ = std::move(name);
    }

    [[nodiscard]] const std::vector<PixelTile<PixelType>> &tiles() const
    {
        return tiles_;
    }

    void tiles(std::vector<PixelTile<PixelType>> t)
    {
        tiles_ = std::move(t);
    }

    void add_tile(PixelTile<PixelType> tile)
    {
        tiles_.push_back(std::move(tile));
    }

    [[nodiscard]] std::size_t tile_count() const
    {
        return tiles_.size();
    }

    // @pre: index must be less than tile_count()
    [[nodiscard]] const PixelTile<PixelType> &tile_at(std::size_t index) const
    {
        return tiles_.at(index);
    }

    [[nodiscard]] bool has_palette() const
    {
        return palette_.has_value();
    }

    [[nodiscard]] const Palette<Rgba32> &palette() const
    {
        return palette_.value();
    }

    void palette(Palette<Rgba32> pal)
    {
        palette_ = std::move(pal);
    }

  private:
    // Note: Frame dimensions (width/height in tiles) are stored in AnimParams since all frames in an animation
    // must share the same dimensions.
    std::string frame_name_;
    std::vector<PixelTile<PixelType>> tiles_;
    std::optional<Palette<Rgba32>> palette_;
};

} // namespace porytiles
