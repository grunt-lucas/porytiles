#include "porytiles2/domain/models/tiles_png_workspace.hpp"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Helper function to export workspace tiles with optional flip transformations and trimming.
 *
 * @param workspace The TilesPngWorkspace to export from
 * @param flip_mode Controls whether tiles are exported in canonical or original (flipped) form
 * @param trim_mode Controls whether trailing transparent tiles are included or trimmed
 * @return An Image<IndexPixel> in the standard tiles.png format
 */
Image<IndexPixel>
export_image_helper(const TilesPngWorkspace &workspace, ExportFlipMode flip_mode, ExportTrimMode trim_mode)
{
    // Standard tiles.png format: 16 tiles per row (128 pixels wide)
    constexpr std::size_t tiles_per_row = 16;

    // Determine the effective tile count based on export mode
    std::size_t effective_tile_count = workspace.capacity();
    if (trim_mode == ExportTrimMode::trim_trailing_transparent) {
        // Find the last non-transparent tile
        // Start from capacity-1 and work backwards
        std::size_t last_non_transparent = 0; // Default to tile 0 (which is always present)
        for (std::size_t i = workspace.capacity(); i > 0; --i) {
            const std::size_t idx = i - 1;
            if (!workspace.tile_at(idx).is_transparent()) {
                last_non_transparent = idx;
                break;
            }
        }
        // Include tiles from 0 to last_non_transparent (inclusive)
        effective_tile_count = last_non_transparent + 1;
    }

    // Calculate number of tile rows needed (ceiling division)
    const std::size_t tiles_per_col = (effective_tile_count + tiles_per_row - 1) / tiles_per_row;

    // Calculate image dimensions
    const std::size_t image_width = tiles_per_row * tile::side_length_pix;
    const std::size_t image_height = tiles_per_col * tile::side_length_pix;

    /*
     * TODO: this will currently cause a diff in most vanilla tiles.png even if no changes, since by default the
     * Image<IndexPixel> here passed to PngIndexedImageSaver::save_to_file gets saved using an 8-bit palette. Vanilla
     * tiles.png typically use a 4-bit palette. The contents will be identical, but a 'pngcheck -v' will reveal the
     * diff. Once we support IndexPixel4 and IndexPixel8, we should modify this.
     */
    // Create output image
    Image<IndexPixel> img{image_width, image_height};

    // Copy each tile's pixels into the image
    for (std::size_t tile_idx = 0; tile_idx < effective_tile_count; ++tile_idx) {
        // Calculate tile position in the grid
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;

        // Calculate pixel offsets for this tile
        const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
        const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

        // Get the tile at this index
        const auto &canonical_tile = workspace.tile_at(tile_idx);

        // Determine which tile form to use
        // Get base class reference to avoid implicit slicing
        const PixelTile<IndexPixel> &canonical_base = canonical_tile;

        PixelTile<IndexPixel> tile_to_export;
        if (flip_mode == ExportFlipMode::original) {
            // Apply flip transformations to restore original form
            tile_to_export = canonical_base.flip(canonical_tile.h_flip(), canonical_tile.v_flip());
        }
        else {
            // Use canonical form as-is (explicit base class assignment)
            tile_to_export = canonical_base;
        }

        // Copy all pixels from the tile to the image
        for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
            for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                const std::size_t dest_row = pixel_row_offset + pixel_row;
                const std::size_t dest_col = pixel_col_offset + pixel_col;
                img.set(dest_row, dest_col, tile_to_export.at(pixel_row, pixel_col));
            }
        }
    }

    return img;
}

} // namespace

namespace porytiles2 {

TilesPngWorkspace::TilesPngWorkspace(std::size_t capacity) : cursor_{1}, capacity_{capacity}
{
    // Initialize tiles_ vector with capacity number of transparent tiles
    // Default-constructed IndexPixel is IndexPixel(0), which is transparent
    PixelTile<IndexPixel> transparent_pixel_tile;
    CanonicalPixelTile transparent_tile{transparent_pixel_tile};

    tiles_.resize(capacity, transparent_tile);
}

TilesPngWorkspace::TilesPngWorkspace(const Image<IndexPixel> &img, std::size_t capacity)
    : cursor_{1}, capacity_{capacity}
{
    /*
     * TODO: what should we do if tile 0 is not transparent? This is the pokeemerald convention but it's possible we
     * could run into a tileset in the wild where this is the case. E.g. unit test SecondConstructorShouldLoadValidImage
     * showcases this. We'd want to warn the user somehow? Maybe we can throw this warning further up the stack in the
     * application?
     */

    const PlainTextFormatter formatter{};

    // Precondition: image dimensions are multiples of 8
    if (img.width() % tile::side_length_pix != 0 || img.height() % tile::side_length_pix != 0) {
        const auto msg = formatter.format(
            "Image dimensions must be a multiple of {}, got {}x{}", tile::side_length_pix, img.width(), img.height());
        panic(msg);
    }

    // Calculate total tiles in image
    const std::size_t tiles_per_row = img.width() / tile::side_length_pix;
    const std::size_t tiles_per_col = img.height() / tile::side_length_pix;
    const std::size_t total_tiles = tiles_per_row * tiles_per_col;

    // Panic if image contains more tiles than capacity
    if (total_tiles > capacity) {
        const auto msg = formatter.format("Image contains {} tiles but capacity is only {}", total_tiles, capacity);
        panic(msg);
    }

    // Reserve space for tiles
    tiles_.reserve(capacity);

    // Extract each 8x8 tile from the image
    for (std::size_t tile_row = 0; tile_row < tiles_per_col; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < tiles_per_row; ++tile_col) {
            // Create a PixelTile to hold the extracted tile data
            PixelTile<IndexPixel> pixel_tile;

            // Calculate pixel offsets for this tile
            const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
            const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

            // Copy pixels from source image to tile
            for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
                for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                    const std::size_t src_row = pixel_row_offset + pixel_row;
                    const std::size_t src_col = pixel_col_offset + pixel_col;
                    pixel_tile.set(pixel_row, pixel_col, img.at(src_row, src_col));
                }
            }

            // Create canonical version of the tile
            CanonicalPixelTile canonical_tile{pixel_tile};
            tiles_.push_back(canonical_tile);

            // Add non-transparent tiles to canonical_forms_ map
            if (!canonical_tile.is_transparent()) {
                const std::size_t tile_index = tiles_.size() - 1;
                const PixelTile<IndexPixel> &base_tile = canonical_tile;
                canonical_forms_[base_tile].push_back(tile_index);
            }
        }
    }

    // Pad tiles_ vector to capacity with transparent tiles
    PixelTile<IndexPixel> transparent_pixel_tile;
    CanonicalPixelTile transparent_tile{transparent_pixel_tile};
    tiles_.resize(capacity, transparent_tile);

    // Set cursor to first transparent tile after tile 0
    cursor_ = 1;
    while (cursor_ < capacity && !tiles_[cursor_].is_transparent()) {
        cursor_++;
    }
}

std::size_t TilesPngWorkspace::insert_tile(const CanonicalPixelTile<IndexPixel> &tile)
{
    // Check if we're at capacity
    if (cursor_ >= capacity_) {
        panic("TilesPngWorkspace is at capacity");
    }

    // No point inserting a transparent tile
    if (tile.is_transparent()) {
        return 0;
    }

    // Insert tile at cursor position
    tiles_[cursor_] = tile;
    const std::size_t old_cursor = cursor_;

    // Add to canonical_forms_ map
    const PixelTile<IndexPixel> &base_tile = tile;
    canonical_forms_[base_tile].push_back(cursor_);

    // Fast-forward cursor to next transparent tile
    cursor_++;
    while (cursor_ < capacity_ && !tiles_[cursor_].is_transparent()) {
        cursor_++;
    }

    return old_cursor;
}

std::optional<std::size_t> TilesPngWorkspace::first_occurrence_of(const CanonicalPixelTile<IndexPixel> &tile) const
{
    const PixelTile<IndexPixel> &base_tile = tile;
    auto it = canonical_forms_.find(base_tile);
    if (it != canonical_forms_.end() && !it->second.empty()) {
        return it->second.front();
    }
    return std::nullopt;
}

CanonicalPixelTile<IndexPixel> TilesPngWorkspace::tile_at(std::size_t index) const
{
    if (index >= tiles_.size()) {
        panic("index " + std::to_string(index) + " >= size " + std::to_string(tiles_.size()));
    }
    return tiles_.at(index);
}

Image<IndexPixel> TilesPngWorkspace::export_image(ExportFlipMode flip_mode, ExportTrimMode trim_mode) const
{
    return export_image_helper(*this, flip_mode, trim_mode);
}

bool TilesPngWorkspace::at_capacity() const
{
    return cursor_ == capacity_;
}

void TilesPngWorkspace::reserve_anim_slots(std::size_t anim_tile_count)
{
    const PlainTextFormatter formatter{};

    // Precondition: must be called before any regular tile insertions
    // Cursor should be at 1 (initial position after reserved tile 0)
    if (cursor_ != 1) {
        panic("reserve_animation_slots must be called before any regular tile insertions");
    }

    // Precondition: anim_tile_count must fit in the workspace
    // Need room for: tile 0 (transparent) + anim_tile_count + at least one regular tile
    if (anim_tile_count >= capacity_ - 1) {
        const auto msg = formatter.format(
            "anim_tile_count ({}) must be less than capacity - 1 ({})", anim_tile_count, capacity_ - 1);
        panic(msg);
    }

    // Animation tiles occupy indices 1 through anim_tile_count (inclusive)
    // So animation_end_offset_ is anim_tile_count + 1 (first index after animation region)
    anim_end_offset_ = anim_tile_count + 1;

    // Move cursor past the reserved animation region
    cursor_ = anim_end_offset_;
}

void TilesPngWorkspace::place_anim_tile(std::size_t reserved_index, const CanonicalPixelTile<IndexPixel> &tile)
{
    const PlainTextFormatter formatter{};

    // Precondition: animation slots must have been reserved
    if (anim_end_offset_ <= 1) {
        panic("place_animation_tile called but no animation slots were reserved");
    }

    // reserved_index is 0-based within the reserved region
    // So absolute index is reserved_index + animation_start_offset() = reserved_index + 1
    const std::size_t absolute_index = reserved_index + anim_start_offset();

    // Precondition: reserved_index must be within the reserved region
    if (absolute_index >= anim_end_offset_) {
        const auto msg = formatter.format(
            "reserved_index ({}) is out of bounds for animation region (max: {})",
            reserved_index,
            anim_end_offset_ - anim_start_offset() - 1);
        panic(msg);
    }

    // Place the tile at the absolute index
    tiles_[absolute_index] = tile;

    // Add to canonical_forms_ map for deduplication support
    if (tile.is_transparent()) {
        panic("illegal transparent key frame tile");
    }
    const PixelTile<IndexPixel> &base_tile = tile;
    canonical_forms_[base_tile].push_back(absolute_index);
}

} // namespace porytiles2
