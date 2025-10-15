#include "porytiles2/domain/models/tiles_png_workspace.hpp"

#include "fmt/format.h"

#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

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

bool TilesPngWorkspace::at_capacity() const
{
    return cursor_ == capacity_;
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

} // namespace porytiles2
