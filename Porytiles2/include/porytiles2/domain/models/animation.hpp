#pragma once

#include <optional>
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
 * In addition to the standard 0.png, 1.png, etc. frames, there is a special the "key frame" that appears in tiles.png.
 * The GBA game engine uses the other frames (stored as separate .4bpp files) to animate by swapping tile data in VRAM
 * at runtime.
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

    [[nodiscard]] const AnimationParams &params() const
    {
        return params_;
    }

    void params(AnimationParams params)
    {
        params_ = std::move(params);
    }

    /**
     * @brief Checks if this animation has a key frame set.
     *
     * @return True if the key frame has been set, false otherwise
     */
    [[nodiscard]] bool has_key_frame() const
    {
        return key_frame_.has_value();
    }

    /**
     * @brief Returns the key frame of this animation.
     *
     * @details
     * The key frame is a special frame of the animation and is the frame whose tiles are stored in tiles.png. All other
     * frames are stored as separate .4bpp files and loaded dynamically by the game engine.
     *
     * @pre has_key_frame() must return true
     * @return Reference to the keyframe
     */
    [[nodiscard]] const AnimationFrame<PixelType> &key_frame() const
    {
        if (!key_frame_.has_value()) {
            panic("key_frame() called on Animation with no key frame set");
        }
        return *key_frame_;
    }

    void key_frame(AnimationFrame<PixelType> key_frame)
    {
        key_frame_ = std::move(key_frame);
    }

    [[nodiscard]] const std::vector<AnimationFrame<PixelType>> &frames() const
    {
        return frames_;
    }

    [[nodiscard]] std::vector<AnimationFrame<PixelType>> &frames()
    {
        return frames_;
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
        if (index >= frame_count()) {
            panic("index " + std::to_string(index) + " is out of range");
        }
        return frames_[index];
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

  private:
    std::string name_;
    AnimationParams params_;
    std::optional<AnimationFrame<PixelType>> key_frame_;
    std::vector<AnimationFrame<PixelType>> frames_;
};

} // namespace porytiles2
