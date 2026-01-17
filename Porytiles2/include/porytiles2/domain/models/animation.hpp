#pragma once

#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/domain/models/supports_transparency.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

namespace anim {

/*
 * TODO: define these somewhere else, they are infra concerns
 */
constexpr std::string g_tileset_anims_prefix = "gTilesetAnims_";
constexpr std::string porytiles_managed_prefix = "PorytilesManaged_";

} // namespace anim

/**
 * @brief A complete tileset animation with name, configuration, and frame data.
 *
 * @details
 * Animation represents a full animation definition for a tileset, combining:
 * - A unique name identifying the animation (e.g., "flower", "water", "waterfall")
 * - Configuration parameters (AnimationParams) controlling timing and VRAM placement
 * - An optional key frame (for Porytiles-format animations)
 * - Frame data (map of AnimationFrame) containing the actual tile pixels
 *
 * In addition to the standard frames, there is a special "key frame" that appears in tiles.png. The GBA game engine
 * uses the other frames (stored as separate .4bpp files) to animate by swapping tile data in VRAM at runtime.
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

    [[nodiscard]] const std::map<std::string, AnimationFrame<PixelType>> &frames() const
    {
        return frames_;
    }

    [[nodiscard]] std::map<std::string, AnimationFrame<PixelType>> &frames()
    {
        return frames_;
    }

    [[nodiscard]] std::vector<AnimationFrame<PixelType>> frames_values() const
    {
        return frames_ | std::views::values | std::ranges::to<std::vector>();
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

    bool has_frame(const std::string &frame_name)
    {
        return frames_.contains(frame_name);
    }

    /**
     * @brief Returns the frame with the specified name.
     *
     * @param frame_name The frame name
     * @pre index must be less than frame_count()
     * @return Reference to the frame at the specified index
     */
    [[nodiscard]] const AnimationFrame<PixelType> &frame_for_name(const std::string &frame_name) const
    {
        if (!frames_.contains(frame_name)) {
            panic("anim '" + name_ + "' has no such frame '" + frame_name + "'");
        }
        return frames_.at(frame_name);
    }

    /**
     * @brief Adds a frame to this Animation frame map.
     *
     * @param name The frame name
     * @param frame The frame to add
     */
    void put_frame(const std::string &name, AnimationFrame<PixelType> frame)
    {
        frames_.emplace(name, std::move(frame));
    }

    /**
     * @brief Returns the "composite" frame for this animation.
     *
     * @details
     * The "composite" frame for an Animation is a special, artificially constructed frame where each constituent
     * subtile contains all colors across all AnimationFrames (including the key frame) for the given subtile index.
     * This is essential: since animation tiles are dynamic but the palette index is fixed, the palette packer needs to
     * know all possible colors that could appear in a given 8x8 animation tile region at any point in the animation.
     * The composite frame's pixel arrangement is arbitrary and shouldn't be relied upon for meaningful information.
     *
     * @param extrinsic_transparency The extrinsic transparency color to use when filtering pixels
     * @pre has_key_frame() must return true
     * @return A composite frame for this Animation
     *
     * @todo This method currently only supports extrinsic transparency (Rgba32). Consider extending to support
     * intrinsic transparency (IndexPixel) if needed in the future.
     */
    [[nodiscard]] AnimationFrame<PixelType> composite_frame(const PixelType &extrinsic_transparency) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        if (!has_key_frame()) {
            panic("composite_frame() called on Animation with no key frame");
        }

        AnimationFrame<PixelType> composite{"composite"};
        const std::size_t tile_count = key_frame().tile_count();

        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            std::set<PixelType> all_colors;

            // Collect from key frame
            auto key_colors = key_frame().tile_at(tile_idx).unique_nontransparent_colors(extrinsic_transparency);
            all_colors.merge(key_colors);

            // Collect from regular frames
            for (const auto &[name, frame] : frames_) {
                auto colors = frame.tile_at(tile_idx).unique_nontransparent_colors(extrinsic_transparency);
                all_colors.merge(colors);
            }

            // Create composite tile with all colors packed arbitrarily, first pixel is guaranteed extrinsic_transparent
            PixelTile<PixelType> composite_tile{extrinsic_transparency};
            std::size_t pix_idx = 1;
            for (const auto &color : all_colors) {
                if (pix_idx >= tile::size_pix) {
                    break;
                }
                composite_tile.set(pix_idx++, color);
            }

            composite.add_tile(std::move(composite_tile));
        }

        return composite;
    }

  private:
    std::string name_;
    AnimationParams params_;
    std::optional<AnimationFrame<PixelType>> key_frame_;
    std::map<std::string, AnimationFrame<PixelType>> frames_;
};

} // namespace porytiles2
