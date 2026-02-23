#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

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
    std::size_t pal_index;         ///< Composite-aware palette index for this subtile
    bool h_flip;                   ///< Horizontal flip required to match
    bool v_flip;                   ///< Vertical flip required to match
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
     * @param animation The compiled animation with IndexPixel tiles.
     * @param tile_offset The starting tile index in tiles.png for this animation's keyframe.
     * @param extrinsic_transparency The extrinsic transparent color.
     * @param subtile_pal_indices Composite-aware palette index for each keyframe subtile.
     * @pre @p animation must have at least one frame (the keyframe).
     * @pre @p subtile_pal_indices.size() must equal the keyframe tile count.
     */
    void register_animation(
        const std::string &anim_name,
        const Animation<Rgba32> &animation,
        std::size_t tile_offset,
        const Rgba32 &extrinsic_transparency,
        const std::vector<std::size_t> &subtile_pal_indices);

    /**
     * @brief Attempts to match a tile against registered animation keyframes.
     *
     * @details
     * Searches the lookup map for a tile that matches the given canonical tile (considering all flip orientations).
     * If found, returns match information including the tile index and required flip bits.
     *
     * @param tile The canonical tile to match
     * @return Match information if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<AnimTileMatch> find_match(const CanonicalPixelTile<Rgba32> &tile) const;

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
        std::size_t pal_index;         // Composite-aware palette index
        bool h_flip;                   // Flip applied to reach canonical form
        bool v_flip;
    };

    /**
     * @brief Internal storage for animation registration metadata.
     */
    struct AnimRegistration {
        std::size_t tile_offset;
        std::size_t tile_count;
    };

    // Map from canonical tile form to keyframe info
    std::map<PixelTile<Rgba32>, KeyframeTileInfo> lookup_map_;
    std::size_t total_tiles_{0};
    std::map<std::string, AnimRegistration> animation_registrations_;
};

} // namespace porytiles2
