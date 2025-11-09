#include "porytiles2/domain/models/tiles_png_workspace.hpp"

#include "fmt/format.h"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Helper function to export workspace tiles with optional flip transformations.
 *
 * @param workspace The TilesPngWorkspace to export from
 * @param apply_flips If true, applies h_flip/v_flip transformations to restore original forms;
 *                    if false, exports canonical forms as-is
 * @return An Image<IndexPixel> in the standard tiles.png format
 */
Image<IndexPixel> export_image_helper(const TilesPngWorkspace &workspace, bool apply_flips)
{
    // Standard tiles.png format: 16 tiles per row (128 pixels wide)
    const std::size_t tiles_per_row = 16;

    // Calculate number of tile rows needed (ceiling division)
    const std::size_t tiles_per_col = (workspace.capacity() + tiles_per_row - 1) / tiles_per_row;

    // Calculate image dimensions
    const std::size_t image_width = tiles_per_row * tile::side_length_pix;
    const std::size_t image_height = tiles_per_col * tile::side_length_pix;

    // Create output image
    Image<IndexPixel> img{image_width, image_height};

    // Copy each tile's pixels into the image
    for (std::size_t tile_idx = 0; tile_idx < workspace.capacity(); ++tile_idx) {
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
        if (apply_flips) {
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

bool TilesPngWorkspace::insert_tile(const CanonicalPixelTile<IndexPixel> &tile)
{
    // Check if we're at capacity
    if (cursor_ >= capacity_) {
        return false;
    }

    // No point inserting a transparent tile
    if (tile.is_transparent()) {
        return false;
    }

    // Insert tile at cursor position
    tiles_[cursor_] = tile;

    // Add to canonical_forms_ map
    const PixelTile<IndexPixel> &base_tile = tile;
    canonical_forms_[base_tile].push_back(cursor_);

    // Fast-forward cursor to next transparent tile
    cursor_++;
    while (cursor_ < capacity_ && !tiles_[cursor_].is_transparent()) {
        cursor_++;
    }

    return true;
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

Image<IndexPixel> TilesPngWorkspace::export_canonical_image() const
{
    return export_image_helper(*this, false);
}

Image<IndexPixel> TilesPngWorkspace::export_original_image() const
{
    return export_image_helper(*this, true);
}

bool TilesPngWorkspace::at_capacity() const
{
    return cursor_ == capacity_;
}

} // namespace porytiles2
