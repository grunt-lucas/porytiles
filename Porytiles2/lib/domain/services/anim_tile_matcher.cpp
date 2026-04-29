#include "porytiles2/domain/services/anim_tile_matcher.hpp"

#include <algorithm>
#include <ranges>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

void AnimTileMatcher::register_animation(
    const std::string &anim_name,
    const Animation<Rgba32> &animation,
    std::size_t tile_offset,
    const Rgba32 &extrinsic_transparency,
    bool is_cross_tileset)
{
    if (animation.frames().empty()) {
        panic("animation must have at least one frame");
    }

    // Use key frame if available, otherwise fall back to first regular frame (manual mode).
    const AnimFrame<Rgba32> &representative_frame =
        animation.has_key_frame() ? animation.key_frame() : animation.frames().begin()->second;
    const auto &tiles = representative_frame.tiles();

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        const PixelTile<Rgba32> &tile = tiles[i];

        // Skip transparent tiles. They don't need to be matched.
        // Transparent tiles are valid for animations without a key frame (manual linking).
        // For key frame animations, validate_anim_frames() catches transparent tiles earlier.
        if (tile.is_transparent(extrinsic_transparency)) {
            continue;
        }

        // Canonicalize the tile under this entry's ET and track which flips were applied. ET-aware
        // canonicalization guarantees that tiles with identical opaque-pixel patterns but different transparent
        // colors land on equivalent orientations under the map's cross-ET comparator.
        CanonicalPixelTile canonical{tile, extrinsic_transparency};

        KeyframeKey key{static_cast<const PixelTile<Rgba32> &>(canonical), extrinsic_transparency};
        if (lookup_map_.contains(key)) {
            // For key frame animations, validate_anim_frames() catches duplicates before we get here.
            // For non-key-frame animations, duplicate tiles across animations are valid. Skip registration
            // and let the first registration win.
            continue;
        }

        // Calculate absolute tile index in tiles.png
        const std::size_t absolute_index = tile_offset + i;

        lookup_map_[key] = KeyframeTileInfo{
            anim_name,
            absolute_index,
            i,
            canonical.h_flip(),
            canonical.v_flip(),
            is_cross_tileset,
        };
    }

    total_tiles_ += tiles.size();

    if (is_cross_tileset && animation_registrations_.contains(anim_name)) {
        panic("cross-tileset animation '" + anim_name + "' has the same name as an already-registered animation");
    }
    animation_registrations_.try_emplace(anim_name, AnimRegistration{tile_offset, tiles.size()});
}

std::optional<AnimTileMatch>
AnimTileMatcher::find_match(const CanonicalPixelTile<Rgba32> &tile, const Rgba32 &extrinsic_transparency) const
{
    KeyframeKey lookup_key{static_cast<const PixelTile<Rgba32> &>(tile), extrinsic_transparency};

    const auto it = lookup_map_.find(lookup_key);
    if (it == lookup_map_.end()) {
        return std::nullopt;
    }

    const KeyframeTileInfo &info = it->second;

    // Calculate the effective flip bits
    // The stored flip tells us how the keyframe tile was flipped to reach canonical form
    // The input tile's flip tells us how it was flipped to reach canonical form
    // XOR gives us the flip needed to go from keyframe tile to input tile
    bool effective_h_flip = info.h_flip != tile.h_flip(); // NOLINT
    bool effective_v_flip = info.v_flip != tile.v_flip(); // NOLINT

    return AnimTileMatch{
        info.anim_name,
        info.tile_index,
        info.keyframe_tile_idx,
        effective_h_flip,
        effective_v_flip,
        info.is_cross_tileset,
    };
}

std::optional<std::size_t> AnimTileMatcher::tile_offset_for(const std::string &anim_name) const
{
    const auto it = animation_registrations_.find(anim_name);
    if (it == animation_registrations_.end()) {
        return std::nullopt;
    }
    return it->second.tile_offset;
}

std::optional<std::size_t> AnimTileMatcher::tile_count_for(const std::string &anim_name) const
{
    const auto it = animation_registrations_.find(anim_name);
    if (it == animation_registrations_.end()) {
        return std::nullopt;
    }
    return it->second.tile_count;
}

bool AnimTileMatcher::is_in_animation_range(std::size_t tile_index) const
{
    return std::ranges::any_of(animation_registrations_ | std::views::values, [tile_index](const auto &reg) {
        return tile_index >= reg.tile_offset && tile_index < reg.tile_offset + reg.tile_count;
    });
}

void AnimTileMatcher::clear()
{
    lookup_map_.clear();
    animation_registrations_.clear();
    total_tiles_ = 0;
}

} // namespace porytiles2
