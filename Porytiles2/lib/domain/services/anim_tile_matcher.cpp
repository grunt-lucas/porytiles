#include "porytiles2/domain/services/anim_tile_matcher.hpp"

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

void AnimTileMatcher::register_animation(
    const std::string &anim_name,
    const Animation<Rgba32> &animation,
    std::size_t tile_offset,
    const Rgba32 &extrinsic_transparency,
    const std::vector<std::size_t> &subtile_pal_indices)
{
    if (animation.frames().empty()) {
        panic("animation must have at least one frame");
    }

    const AnimFrame<Rgba32> &key_frame = animation.key_frame();
    const auto &tiles = key_frame.tiles();

    assert_or_panic(
        subtile_pal_indices.size() == tiles.size(), "subtile_pal_indices.size() must equal keyframe tile count");

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        const PixelTile<Rgba32> &tile = tiles[i];

        // Skip transparent tiles - they don't need to be matched
        if (tile.is_transparent(extrinsic_transparency)) {
            panic("detected transparent key frame tile");
        }

        // Canonicalize the tile and track which flips were applied
        CanonicalPixelTile canonical{tile};

        // Get the base tile (canonical form) for lookup
        const PixelTile<Rgba32> &base_tile = canonical;
        if (lookup_map_.contains(base_tile)) {
            panic("detected duplicate key frame tile");
        }

        // Calculate absolute tile index in tiles.png
        const std::size_t absolute_index = tile_offset + i;

        lookup_map_[base_tile] = KeyframeTileInfo{
            anim_name,
            absolute_index,
            i,
            subtile_pal_indices[i],
            canonical.h_flip(),
            canonical.v_flip(),
        };
    }

    total_tiles_ += tiles.size();
    animation_registrations_[anim_name] = AnimRegistration{tile_offset, tiles.size()};
}

std::optional<AnimTileMatch> AnimTileMatcher::find_match(const CanonicalPixelTile<Rgba32> &tile) const
{
    // Get the canonical base tile for lookup
    const PixelTile<Rgba32> &base_tile = tile;

    auto it = lookup_map_.find(base_tile);
    if (it == lookup_map_.end()) {
        return std::nullopt;
    }

    const KeyframeTileInfo &info = it->second;

    // Calculate the effective flip bits
    // The stored flip tells us how the keyframe tile was flipped to reach canonical form
    // The input tile's flip tells us how it was flipped to reach canonical form
    // XOR gives us the flip needed to go from keyframe tile to input tile
    bool effective_h_flip = info.h_flip != tile.h_flip();
    bool effective_v_flip = info.v_flip != tile.v_flip();

    return AnimTileMatch{
        info.anim_name,
        info.tile_index,
        info.keyframe_tile_idx,
        info.pal_index,
        effective_h_flip,
        effective_v_flip,
    };
}

std::optional<std::size_t> AnimTileMatcher::tile_offset_for(const std::string &anim_name) const
{
    auto it = animation_registrations_.find(anim_name);
    if (it == animation_registrations_.end()) {
        return std::nullopt;
    }
    return it->second.tile_offset;
}

std::optional<std::size_t> AnimTileMatcher::tile_count_for(const std::string &anim_name) const
{
    auto it = animation_registrations_.find(anim_name);
    if (it == animation_registrations_.end()) {
        return std::nullopt;
    }
    return it->second.tile_count;
}

void AnimTileMatcher::clear()
{
    lookup_map_.clear();
    animation_registrations_.clear();
    total_tiles_ = 0;
}

} // namespace porytiles2
