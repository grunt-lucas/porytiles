#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/domain/models/supports_transparency.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief A complete tileset animation with name, configuration, and frame data.
 *
 * @details
 * Animation represents a full animation definition for a tileset, combining:
 * - A unique name identifying the animation (e.g., "flower", "water", "waterfall")
 * - Configuration parameters (AnimationParams) controlling timing and VRAM placement
 * - Frame data (vector of AnimationFrame) containing the actual tile pixels
 *
 * Frame 0 is special - it's the "keyframe" that appears in tiles.png. The GBA game engine uses the other frames
 * (stored as separate .4bpp files) to animate by swapping tile data in VRAM at runtime.
 *
 * The template parameter determines the pixel format:
 * - Animation<Rgba32>: Used in PorytilesTilesetComponent (source format, RGBA pixels)
 * - Animation<IndexPixel>: Used in PorymapTilesetComponent (compiled format, palette indices)
 *
 * @tparam PixelType The pixel type for animation frame tiles; must satisfy SupportsTransparency concept
 */
template <SupportsTransparency PixelType>
class Animation {
  public:
    Animation() = default;

    explicit Animation(std::string name) : name_{std::move(name)} {}

    Animation(std::string name, AnimationParams params) : name_{std::move(name)}, params_{std::move(params)} {}

    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    void name(std::string n)
    {
        name_ = std::move(n);
    }

    [[nodiscard]] const AnimationParams &params() const
    {
        return params_;
    }

    [[nodiscard]] AnimationParams &params()
    {
        return params_;
    }

    void params(AnimationParams p)
    {
        params_ = std::move(p);
    }

    [[nodiscard]] const std::vector<AnimationFrame<PixelType>> &frames() const
    {
        return frames_;
    }

    [[nodiscard]] std::vector<AnimationFrame<PixelType>> &frames()
    {
        return frames_;
    }

    void frames(std::vector<AnimationFrame<PixelType>> f)
    {
        frames_ = std::move(f);
    }

    /**
     * @brief Adds a frame to the end of this animation's frame list.
     *
     * @param frame The frame to add
     */
    void add_frame(AnimationFrame<PixelType> frame)
    {
        frames_.push_back(std::move(frame));
    }

    /**
     * @brief Returns the number of frames in this animation.
     *
     * @return The frame count
     */
    [[nodiscard]] std::size_t frame_count() const
    {
        return frames_.size();
    }

    /**
     * @brief Returns the frame at the specified index.
     *
     * @param index The frame index
     * @pre index must be less than frame_count()
     * @return Reference to the frame at the specified index
     */
    [[nodiscard]] const AnimationFrame<PixelType> &frame_at(std::size_t index) const
    {
        return frames_.at(index);
    }

    /**
     * @brief Returns the keyframe (frame 0) of this animation.
     *
     * @details
     * The keyframe is the first frame of the animation and is the frame whose tiles are stored in tiles.png. All other
     * frames are stored as separate .4bpp files and loaded dynamically by the game engine.
     *
     * @pre Animation must have at least one frame
     * @return Reference to the keyframe
     */
    [[nodiscard]] const AnimationFrame<PixelType> &keyframe() const
    {
        if (frames_.empty()) {
            panic("animation '" + name_ + "' has no frames");
        }
        return frames_.at(keyframe_index());
    }

    /**
     * @brief Returns the index of the keyframe (always 0).
     *
     * @details
     * By convention, frame 0 is always the keyframe. This static method makes this convention explicit and allows code
     * to reference the keyframe index without magic numbers.
     *
     * @return 0 (the keyframe index)
     */
    [[nodiscard]] static constexpr std::size_t keyframe_index()
    {
        return 0;
    }

    /**
     * @brief Checks if this animation has any frames.
     *
     * @return True if the animation has at least one frame, false otherwise
     */
    [[nodiscard]] bool has_frames() const
    {
        return !frames_.empty();
    }

  private:
    std::string name_;
    AnimationParams params_;
    std::vector<AnimationFrame<PixelType>> frames_;
};

} // namespace porytiles2
