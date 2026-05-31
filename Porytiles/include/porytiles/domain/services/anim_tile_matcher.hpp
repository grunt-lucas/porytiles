#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

namespace porytiles {

/**
 * @brief Result of matching a tile against animation keyframes.
 *
 * @details
 * Contains all the information needed to reference an animation keyframe tile in a metatile entry: the tile index in
 * tiles.png, the flip bits to apply, and metadata about which animation the match came from.
 */
struct AnimTileMatch {
    std::string anim_name;         ///< Name of the animation this keyframe belongs to
    std::size_t tile_index;        ///< Absolute tile index in tiles.png
    std::size_t keyframe_tile_idx; ///< Index within the keyframe (0, 1, 2, ...)
    bool h_flip;                   ///< Horizontal flip required to match
    bool v_flip;                   ///< Vertical flip required to match
    bool is_cross_tileset{false};  ///< True if this match is against a primary animation (cross-tileset linking)
};

/**
 * @brief Matches tiles against animation keyframe tiles for compilation.
 *
 * @details
 * AnimationTileMatcher is used during tileset compilation to detect when a metatile tile corresponds to an animation
 * keyframe. Instead of inserting the tile normally into TilesPngWorkspace, the compiler should reference the
 * pre-placed animation tile at its reserved offset.
 *
 * The matcher builds an internal lookup map from canonical tile forms to animation tile info, enabling O(1) matching
 * during the compilation assignment step.
 *
 * Usage during compilation:
 * 1. Create matcher and register all animations with their tile offsets
 * 2. For each tile encountered during metatile processing, call `find_match()`
 * 3. If a match is found, use the returned tile index and flip bits instead of normal workspace insertion
 *
 * @code
 * AnimationTileMatcher matcher;
 * matcher.register_animation("flower", flower_animation, 1); // tile_offset = 1
 *
 * for (each metatile tile) {
 *     auto match = matcher.find_match(tile);
 *     if (match) {
 *         // Use match->tile_index and match->h_flip/v_flip
 *     } else {
 *         // Normal workspace insertion
 *     }
 * }
 * @endcode
 */
class AnimTileMatcher {
  public:
    /**
     * @brief Registers an animation's keyframe tiles for matching.
     *
     * @details
     * Extracts all tiles from the animation's keyframe (frame 0) and registers them in the lookup map. Each tile is
     * stored in canonical form along with metadata about its position and the animation it belongs to.
     *
     * @param anim_name Name of the animation.
     * @param animation The animation with RGBA tiles.
     * @param tile_offset The starting tile index in tiles.png for this animation's keyframe.
     * @param extrinsic_transparency The extrinsic transparent color.
     * @param is_cross_tileset Whether this animation is cross-tileset, i.e. from the paired primary
     * @pre @p animation must have at least one frame (the keyframe).
     */
    void register_animation(
        const std::string &anim_name,
        const Animation<Rgba32> &animation,
        std::size_t tile_offset,
        const Rgba32 &extrinsic_transparency,
        bool is_cross_tileset = false);

    /**
     * @brief Attempts to match a tile against registered animation keyframes.
     *
     * @details
     * Searches the lookup map for a tile that matches the given canonical tile (considering all flip orientations).
     * The cross-ET comparator on the map compares each candidate under its own registered extrinsic transparency,
     * so registered entries and the input tile may have been compiled under different ETs and still match when
     * their opaque-pixel patterns coincide.
     *
     * @param tile The canonical tile to match.
     * @param extrinsic_transparency The extrinsic transparency value applied to @p tile during the cross-ET lookup.
     * Typically this is the ET configured for the tileset whose pixel data @p tile was derived from.
     * @return Match information if found, @c std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<AnimTileMatch>
    find_match(const CanonicalPixelTile<Rgba32> &tile, const Rgba32 &extrinsic_transparency) const;

    /**
     * @brief Returns the total number of keyframe tiles registered across all animations.
     *
     * @return Total tile count for all registered animations
     */
    [[nodiscard]] std::size_t total_keyframe_tiles() const
    {
        return total_tiles_;
    }

    /**
     * @brief Returns the tile offset for a registered animation.
     *
     * @param anim_name Name of the animation
     * @return The tile offset if the animation is registered, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::size_t> tile_offset_for(const std::string &anim_name) const;

    /**
     * @brief Returns the tile count for a registered animation.
     *
     * @param anim_name Name of the animation
     * @return The tile count if the animation is registered, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::size_t> tile_count_for(const std::string &anim_name) const;

    /**
     * @brief Checks whether a tile index falls within any registered animation range.
     *
     * @details
     * Iterates over all registered animations and checks if @p tile_index falls within
     * [tile_offset, tile_offset + tile_count) for any of them.
     *
     * @param tile_index The absolute tile index in tiles.png to check.
     * @return @c true if @p tile_index falls within a registered animation range, @c false otherwise.
     */
    [[nodiscard]] bool is_in_animation_range(std::size_t tile_index) const;

    /**
     * @brief Clears all registered animations.
     */
    void clear();

  private:
    /**
     * @brief Internal storage for keyframe tile info.
     */
    struct KeyframeTileInfo {
        std::string anim_name;
        std::size_t tile_index;        // Absolute index in tiles.png
        std::size_t keyframe_tile_idx; // Index within keyframe
        bool h_flip;                   // Flip applied to reach canonical form
        bool v_flip;
        bool is_cross_tileset;
    };

    /**
     * @brief Internal storage for animation registration metadata.
     */
    struct AnimRegistration {
        std::size_t tile_offset;
        std::size_t tile_count;
    };

    /**
     * @brief Key type for the cross-ET animation lookup map.
     *
     * @details
     * Carries the canonical pixel data alongside the extrinsic transparency value that was configured for the
     * tileset this key was registered from. The paired ET is what makes the @c KeyframeKeyCompare comparator able to
     * classify this entry's pixels independently of any other entry's ET.
     */
    struct KeyframeKey {
        PixelTile<Rgba32> tile;
        Rgba32 extrinsic_transparency;
    };

    /**
     * @brief Strict weak ordering comparator that classifies each side's pixels under its own registered ET.
     *
     * @details
     * Delegates to @c PixelTile<Rgba32>::cross_et_compare, which is the strict-weak-ordering analog of
     * @c equals_ignoring_transparency with split extrinsic transparency values. This is what lets the map hold
     * entries from tilesets compiled with different ET values and still find matches when their opaque-pixel
     * patterns coincide.
     */
    struct KeyframeKeyCompare {
        using is_transparent = void;

        bool operator()(const KeyframeKey &a, const KeyframeKey &b) const
        {
            return PixelTile<Rgba32>::cross_et_compare(
                       a.tile, a.extrinsic_transparency, b.tile, b.extrinsic_transparency) < 0;
        }
    };

    // Map from canonical tile form (with its source ET) to keyframe info
    std::map<KeyframeKey, KeyframeTileInfo, KeyframeKeyCompare> lookup_map_;
    std::size_t total_tiles_{0};
    std::map<std::string, AnimRegistration> animation_registrations_;
};

} // namespace porytiles
