#pragma once

#include <optional>
#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/supports_transparency.hpp"

namespace porytiles2 {

/**
 * @brief Represents a single frame of an animation, containing tiles and a frame name.
 *
 * @details
 * AnimationFrame holds the pixel data for one frame of an animation. Each frame consists of one or more 8x8 tiles
 * arranged in a grid. The frame name typically corresponds to the PNG filename (e.g., "0", "1", "2" for 0.png, 1.png,
 * 2.png).
 *
 * The template parameter PixelType determines whether the frame uses RGBA pixels (for Porytiles component, source
 * format) or indexed pixels (for Porymap component, compiled format).
 *
 * Key concepts:
 * - A special "key frame" that appears in tiles.png, but isn't part of the animation loop
 *   - Users tell Porytiles which metatile subtiles should be animated by using key frame tiles on the layer PNGs
 * - Other frames (0, 1, 2, ...) are stored as separate .4bpp files and swapped at runtime
 * - All frames of an animation must have the same tile count
 *
 * @tparam PixelType The pixel type for tiles; must satisfy SupportsTransparency concept
 */
template <SupportsTransparency PixelType>
class AnimationFrame {
  public:
    AnimationFrame() = default;

    explicit AnimationFrame(std::string frame_name) : frame_name_{std::move(frame_name)} {}

    AnimationFrame(std::string frame_name, std::vector<PixelTile<PixelType>> tiles)
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

    /**
     * @brief Adds a tile to the end of this frame's tile list.
     *
     * @param tile The tile to add
     */
    void add_tile(PixelTile<PixelType> tile)
    {
        tiles_.push_back(std::move(tile));
    }

    /**
     * @brief Returns the number of tiles in this frame.
     *
     * @return The tile count
     */
    [[nodiscard]] std::size_t tile_count() const
    {
        return tiles_.size();
    }

    /**
     * @brief Returns the tile at the specified index.
     *
     * @param index The tile index
     * @pre index must be less than tile_count()
     * @return Reference to the tile at the specified index
     */
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
    /*
     * Note: Frame dimensions (width/height in tiles) are stored in AnimationParams since all frames in an animation
     * must share the same dimensions.
     */
    std::string frame_name_;
    std::vector<PixelTile<PixelType>> tiles_;
    std::optional<Palette<Rgba32>> palette_;
};

} // namespace porytiles2
