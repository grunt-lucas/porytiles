#pragma once

#include <cstddef>

#include "porytiles/domain/models/metatile.hpp"

namespace porytiles {

/**
 * @brief A manual override that maps a specific metatile entry to an animation subtile.
 *
 * @details
 * When using manual frame linking, users explicitly declare which metatile entries reference which animation subtiles.
 * Each override entry specifies the metatile position (id, layer, subtile), flip flags, palette index, and which
 * subtile within the animation frame to use.
 *
 * All fields are required when specifying overrides in anim.json.
 */
struct AnimOverrideEntry {
    /// The metatile ID this override applies to (corresponds to JSON "id" field).
    std::size_t metatile_id;
    /// The layer within the metatile (bottom, middle, or top).
    metatile::Layer layer;
    /// The subtile position within the layer (northwest, northeast, southwest, southeast).
    metatile::Subtile subtile;
    /// Zero-based index into the animation's tile range (tile_offset + frame_subtile = actual tile index).
    std::size_t frame_subtile;
    /// The palette index to use for this tile.
    std::size_t pal_index;
    /// Whether the tile is horizontally flipped.
    bool h_flip;
    /// Whether the tile is vertically flipped.
    bool v_flip;
};

} // namespace porytiles
