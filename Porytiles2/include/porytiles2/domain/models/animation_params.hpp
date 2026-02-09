#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles2/utilities/dynamic_cased_name.hpp"

namespace porytiles2 {

namespace anim {

inline constexpr std::size_t default_frame_factor = 16;
inline constexpr std::size_t default_frame_offset = 0;
inline constexpr std::size_t default_counter_max = 256;

} // namespace anim

/**
 * @brief Configuration parameters for a single tileset animation.
 *
 * @details
 * AnimationParams stores the configuration data needed to generate and interpret animation code for a tileset. These
 * parameters control how animation frames are selected and where animation tiles are located in VRAM.
 *
 * The parameters map directly to the generated C code patterns:
 * - frame_factor: The modulus divisor in `timer % frame_factor == frame_offset` conditions
 * - frame_offset: The remainder value in `timer % frame_factor == frame_offset` conditions
 * - frame_names: Unique frame definitions; position in vector determines FrameN index (0-indexed)
 * - frame_order: The playback sequence; each entry must reference a name from frame_names
 * - tile_offset: The VRAM offset where this animation's tiles begin (in TILE_OFFSET_4BPP units)
 * - tile_count: The number of tiles per animation frame
 * - counter_max: The maximum timer value before wrapping (typically 256)
 */
class AnimationParams {
  public:
    AnimationParams() = default;

    /**
     * @brief Returns the structured name for this animation, preserving case format information.
     *
     * @details
     * When parsing animation code (vanilla or Porytiles-managed), the animation name is extracted from C identifiers
     * like `QueueAnimTiles_General_Water_Current_LandWatersEdge`. The segment after the tileset prefix (e.g.,
     * `"Water_Current_LandWatersEdge"`) is parsed into a @c DynamicCasedName that preserves the two-level segment/word
     * structure. This enables lossless conversion to any output format: @c to_snake_case() for map keys,
     * @c to_c_identifier() for frame variable name reconstruction, @c to_pascal_case() for flattened identifiers.
     *
     * Replaces the former @c vanilla_identifier_ field, which stored the original C identifier as a plain string.
     *
     * @return The structured name, or an empty DynamicCasedName if not set
     */
    [[nodiscard]] const DynamicCasedName &cased_name() const
    {
        return cased_name_;
    }

    void cased_name(DynamicCasedName value)
    {
        cased_name_ = std::move(value);
    }

    /**
     * @brief Returns the frame factor (modulus divisor for timer).
     *
     * @details
     * In the generated C code, this appears as `timer % frame_factor == frame_offset`. Common values are 8 and 16,
     * where 16 means the animation updates every 16 game frames.
     *
     * @return The frame factor value
     */
    [[nodiscard]] std::size_t frame_factor() const
    {
        return frame_factor_;
    }

    void frame_factor(std::size_t value)
    {
        frame_factor_ = value;
    }

    /**
     * @brief Returns the frame offset (remainder value for timer modulo check).
     *
     * @details
     * In the generated C code, this appears as `timer % frame_factor == frame_offset`. Different animations in the
     * same tileset use different offsets (0, 1, 2, ...) to stagger their updates and avoid visual sync artifacts.
     *
     * @return The frame offset value
     */
    [[nodiscard]] std::size_t frame_offset() const
    {
        return frame_offset_;
    }

    void frame_offset(std::size_t value)
    {
        frame_offset_ = value;
    }

    /**
     * @brief Returns the unique frame definitions.
     *
     * @details
     * Lists all unique animation frame files. The position in this vector determines the FrameN index used in generated
     * C code. For example, ["center", "left", "right"] means center.png is Frame0, left.png is Frame1, right.png is
     * Frame2. Frame names can be arbitrary strings (not just numbers).
     *
     * Each entry is a @c DynamicCasedName so that downstream code can convert losslessly to any case format
     * (snake_case for file paths, PascalCase for C identifiers) without re-parsing.
     *
     * @return Reference to the unique frame names vector
     */
    [[nodiscard]] const std::vector<DynamicCasedName> &frame_names() const
    {
        return frame_names_;
    }

    void frame_names(std::vector<DynamicCasedName> value)
    {
        frame_names_ = std::move(value);
    }

    /**
     * @brief Returns the playback sequence.
     *
     * @details
     * Defines the order in which animation frames are played. Each entry must reference a name from frame_names().
     * For example, ["center", "right", "center", "left"] means play center, then right, then center, then left.
     * Frames can repeat in the sequence to create complex animation patterns.
     *
     * Each entry is a @c DynamicCasedName so that downstream code can convert losslessly to any case format
     * without re-parsing.
     *
     * @return Reference to the frame order vector
     */
    [[nodiscard]] const std::vector<DynamicCasedName> &frame_order() const
    {
        return frame_order_;
    }

    void frame_order(std::vector<DynamicCasedName> value)
    {
        frame_order_ = std::move(value);
    }

    /**
     * @brief Returns the VRAM tile offset for this animation.
     *
     * @details
     * In the generated C code, this appears as `TILE_OFFSET_4BPP(tile_offset)` in the AppendTilesetAnimToBuffer call.
     * This value is computed by Porytiles based on where the animation's keyframe tiles are placed in tiles.png.
     *
     * @return The VRAM tile offset
     */
    [[nodiscard]] std::size_t tile_offset() const
    {
        return tile_offset_;
    }

    void tile_offset(std::size_t value)
    {
        tile_offset_ = value;
    }

    /**
     * @brief Returns the number of tiles per animation frame.
     *
     * @details
     * In the generated C code, this appears as `tile_count * TILE_SIZE_4BPP` in the AppendTilesetAnimToBuffer call.
     * This is determined by the dimensions of the animation frame PNGs.
     *
     * @return The number of tiles per frame
     */
    [[nodiscard]] std::size_t tile_count() const
    {
        return tile_count_;
    }

    void tile_count(std::size_t value)
    {
        tile_count_ = value;
    }

    /**
     * @brief Returns the width of animation frames in tiles.
     *
     * @details
     * When non-zero, this specifies the number of 8x8 tiles per row in animation frame PNGs. This value is used when
     * writing animation frames back to disk to preserve the original grid layout. A value of 0 means dimensions were
     * not specified and the default single-row layout is used.
     *
     * @return The frame width in tiles, or 0 if unspecified
     */
    [[nodiscard]] std::size_t width_tiles() const
    {
        return width_tiles_;
    }

    void width_tiles(std::size_t value)
    {
        width_tiles_ = value;
    }

    /**
     * @brief Returns the height of animation frames in tiles.
     *
     * @details
     * When non-zero, this specifies the number of 8x8 tile rows in animation frame PNGs. This value is used when
     * writing animation frames back to disk to preserve the original grid layout. A value of 0 means dimensions were
     * not specified and the default single-row layout is used.
     *
     * @return The frame height in tiles, or 0 if unspecified
     */
    [[nodiscard]] std::size_t height_tiles() const
    {
        return height_tiles_;
    }

    void height_tiles(std::size_t value)
    {
        height_tiles_ = value;
    }

    /**
     * @brief Returns the animation counter maximum value.
     *
     * @details
     * In the generated C code, this appears as `sPrimaryTilesetAnimCounterMax = counter_max` in the InitTilesetAnim
     * function. Common values are 128 and 256, controlling the timer wrap-around behavior.
     *
     * @return The counter maximum value
     */
    [[nodiscard]] std::size_t counter_max() const
    {
        return counter_max_;
    }

    void counter_max(std::size_t value)
    {
        counter_max_ = value;
    }

  private:
    DynamicCasedName cased_name_;
    std::size_t frame_factor_{anim::default_frame_factor};
    std::size_t frame_offset_{anim::default_frame_offset};
    std::vector<DynamicCasedName> frame_names_{DynamicCasedName{"0"}};
    std::vector<DynamicCasedName> frame_order_{DynamicCasedName{"0"}};
    std::size_t tile_offset_{0};
    std::size_t tile_count_{0};
    std::size_t width_tiles_{0};
    std::size_t height_tiles_{0};
    std::size_t counter_max_{anim::default_counter_max};
};

} // namespace porytiles2
