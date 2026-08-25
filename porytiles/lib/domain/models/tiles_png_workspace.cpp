#include "porytiles/domain/models/tiles_png_workspace.hpp"

#include <algorithm>

#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace {

using namespace porytiles;

/// @brief Checks if two tiles are color-equivalent using the provided palette for color lookup.
///
/// @details
/// Two tiles are color-equivalent if, for every pixel position, the colors they reference in the palette are equal.
/// This handles the case where palettes contain duplicate colors at different indices - e.g., if palette slots 7 and 14
/// both contain the same color, pixels using index 7 and index 14 are considered equivalent.
///
/// Note: Index 0 always represents transparency in GBA tilesets, so we treat index 0 specially - two pixels are only
/// equivalent if both have index 0 (both transparent) or both have non-zero indices with matching colors.
///
/// @param expected The expected tile (from computed keyframe)
/// @param actual The actual tile in the workspace
/// @param palette The palette to use for color lookup
/// @return true if tiles are color-equivalent, false otherwise
bool tiles_color_equivalent(
    const PixelTile<IndexPixel> &expected,
    const PixelTile<IndexPixel> &actual,
    const Palette<Rgba32, palette::max_size> &palette)
{
    constexpr Rgba32 extrinsic{};
    return canonical_color_tile_from_index_tile(expected, palette, extrinsic) ==
           canonical_color_tile_from_index_tile(actual, palette, extrinsic);
}

/// @brief Helper function to export a range of workspace tiles with optional flip transformations.
///
/// @details
/// Exports tiles from @p start_tile (inclusive) to @p end_tile (exclusive). The output image positions are relative,
/// so tile at @p start_tile becomes row 0, col 0 in the output image.
///
/// @param workspace The TilesPngWorkspace to export from
/// @param start_tile The first tile index to export (inclusive)
/// @param end_tile The last tile index to export (exclusive)
/// @param flip_mode Controls whether tiles are exported in canonical or original (flipped) form
/// @return An Image<IndexPixel> in the standard tiles.png format
Image<IndexPixel> export_image_range(
    const TilesPngWorkspace &workspace, std::size_t start_tile, std::size_t end_tile, ExportFlipMode flip_mode)
{
    // Standard tiles.png format: 16 tiles per row (128 pixels wide)
    constexpr std::size_t tiles_per_row = metatile::metatiles_per_row * metatile::tiles_per_side;

    const std::size_t tile_count = end_tile - start_tile;

    // Calculate number of tile rows needed (ceiling division)
    const std::size_t tiles_per_col = (tile_count + tiles_per_row - 1) / tiles_per_row;

    // Calculate image dimensions
    const std::size_t image_width = tiles_per_row * tile::side_length_pix;
    const std::size_t image_height = tiles_per_col * tile::side_length_pix;

    // Create output image
    Image<IndexPixel> img{image_width, image_height};

    // Copy each tile's pixels into the image
    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_idx = start_tile + i;

        // Calculate tile position in the output grid (relative to start_tile)
        const std::size_t tile_row = i / tiles_per_row;
        const std::size_t tile_col = i % tiles_per_row;

        // Calculate pixel offsets for this tile
        const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
        const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

        // Get the tile at this index
        const auto &canonical_tile = workspace.tile_at(tile_idx);

        // Determine which tile form to use
        const PixelTile<IndexPixel> &canonical_base = canonical_tile;

        PixelTile<IndexPixel> tile_to_export;
        if (flip_mode == ExportFlipMode::original) {
            tile_to_export = canonical_base.flip(canonical_tile.h_flip(), canonical_tile.v_flip());
        }
        else {
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

/// @brief Finds the last non-transparent tile index in the workspace, scanning backward from the given end position.
///
/// @param workspace The workspace to scan
/// @param scan_start The index to start scanning backward from (exclusive)
/// @param minimum The minimum index to return if all tiles are transparent
/// @return The index of the last non-transparent tile, or @p minimum if none found
std::size_t find_last_non_transparent(const TilesPngWorkspace &workspace, std::size_t scan_start, std::size_t minimum)
{
    for (std::size_t i = scan_start; i > minimum; --i) {
        const std::size_t idx = i - 1;
        if (!workspace.tile_at(idx).is_transparent()) {
            return idx;
        }
    }
    return minimum;
}

} // namespace

namespace porytiles {

void TilesPngWorkspace::advance_cursor_to_next_transparent()
{
    cursor_++;
    while (cursor_ < capacity_ && !tiles_[cursor_].is_transparent()) {
        cursor_++;
    }
}

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
    // Start at 0 and let advance_cursor_to_next_transparent increment to 1 and scan forward
    cursor_ = 0;
    advance_cursor_to_next_transparent();
}

TilesPngWorkspace TilesPngWorkspace::for_secondary(
    const Image<IndexPixel> &primary_tiles_png, std::size_t primary_tile_count, std::size_t total_capacity)
{
    const PlainTextFormatter formatter{};

    // Precondition: primary_tile_count must be less than total_capacity
    if (primary_tile_count >= total_capacity) {
        const auto msg = formatter.format(
            "primary_tile_count ({}) must be less than total_capacity ({})", primary_tile_count, total_capacity);
        panic(msg);
    }

    // Precondition: image dimensions are multiples of 8
    if (primary_tiles_png.width() % tile::side_length_pix != 0 ||
        primary_tiles_png.height() % tile::side_length_pix != 0) {
        const auto msg = formatter.format(
            "Primary tiles.png dimensions must be a multiple of {}, got {}x{}",
            tile::side_length_pix,
            primary_tiles_png.width(),
            primary_tiles_png.height());
        panic(msg);
    }

    // Create empty workspace with total_capacity (all transparent, cursor=1)
    TilesPngWorkspace workspace{total_capacity};

    // Extract tiles from primary image
    const std::size_t tiles_per_row = primary_tiles_png.width() / tile::side_length_pix;
    const std::size_t tiles_per_col = primary_tiles_png.height() / tile::side_length_pix;
    const std::size_t tiles_in_image = tiles_per_row * tiles_per_col;
    const std::size_t tiles_to_load = std::min(tiles_in_image, primary_tile_count);

    // Load primary tiles into positions 0..tiles_to_load-1
    for (std::size_t tile_idx = 0; tile_idx < tiles_to_load; ++tile_idx) {
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;

        PixelTile<IndexPixel> pixel_tile;
        const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
        const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

        for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
            for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                const std::size_t src_row = pixel_row_offset + pixel_row;
                const std::size_t src_col = pixel_col_offset + pixel_col;
                // Strip true-color encoding (upper nibble = palette index) to get raw color indices. Primary tiles.png
                // stores pixels as (palette << 4 | color), but index_tile_from_color_tile() produces raw color indices
                // (0-15). Using color_index() here ensures workspace tiles match what index_tile_from_color_tile()
                // produces, enabling first_occurrence_of() deduplication.
                const IndexPixel true_color_pixel = primary_tiles_png.at(src_row, src_col);
                pixel_tile.set(pixel_row, pixel_col, IndexPixel{true_color_pixel.color_index()});
            }
        }

        CanonicalPixelTile canonical_tile{pixel_tile};
        workspace.tiles_.at(tile_idx) = canonical_tile;

        // Register non-transparent primary tiles for deduplication
        if (!canonical_tile.is_transparent()) {
            const PixelTile<IndexPixel> &base_tile = canonical_tile;
            workspace.canonical_forms_[base_tile].push_back(tile_idx);
        }
    }

    // Position primary_tile_count stays transparent (vanilla secondary tile 0 convention)
    // Set cursor and anim_start_offset_ to reflect the secondary workspace's logical state
    workspace.cursor_ = primary_tile_count + 1;
    workspace.anim_start_offset_ = primary_tile_count + 1;
    workspace.anim_end_offset_ = primary_tile_count + 1;

    return workspace;
}

TilesPngWorkspace
TilesPngWorkspace::for_standalone_secondary(std::size_t primary_tile_count, std::size_t total_capacity)
{
    const PlainTextFormatter formatter{};

    // Precondition: primary_tile_count must be less than total_capacity
    if (primary_tile_count >= total_capacity) {
        const auto msg = formatter.format(
            "primary_tile_count ({}) must be less than total_capacity ({})", primary_tile_count, total_capacity);
        panic(msg);
    }

    // Create empty workspace with total_capacity (all transparent, cursor=1)
    TilesPngWorkspace workspace{total_capacity};

    // No primary tiles to load or register in canonical_forms_.
    // Position primary_tile_count stays transparent (vanilla secondary tile 0 convention).
    workspace.cursor_ = primary_tile_count + 1;
    workspace.anim_start_offset_ = primary_tile_count + 1;
    workspace.anim_end_offset_ = primary_tile_count + 1;

    return workspace;
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
    advance_cursor_to_next_transparent();

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

std::optional<std::size_t> TilesPngWorkspace::first_occurrence_of_by_color(
    const CanonicalPixelTile<IndexPixel> &tile, const Palette<Rgba32, palette::max_size> &palette) const
{
    if (tile.is_transparent()) {
        return std::nullopt;
    }

    const PixelTile<IndexPixel> &tile_base = tile;

    for (std::size_t i = 1; i < capacity_; ++i) {
        if (tiles_[i].is_transparent()) {
            continue;
        }
        const PixelTile<IndexPixel> &workspace_tile_base = tiles_[i];
        if (tiles_color_equivalent(tile_base, workspace_tile_base, palette)) {
            return i;
        }
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
    std::size_t end_tile = capacity_;
    if (trim_mode == ExportTrimMode::trim_trailing_transparent) {
        end_tile = find_last_non_transparent(*this, capacity_, 0) + 1;
    }
    return export_image_range(*this, 0, end_tile, flip_mode);
}

Image<IndexPixel> TilesPngWorkspace::export_secondary_image(
    std::size_t primary_tile_count, ExportFlipMode flip_mode, ExportTrimMode trim_mode) const
{
    std::size_t end_tile = capacity_;
    if (trim_mode == ExportTrimMode::trim_trailing_transparent) {
        const std::size_t last_non_transparent = find_last_non_transparent(*this, capacity_, primary_tile_count);
        // At minimum, export the secondary transparent tile at primary_tile_count
        end_tile = std::max(last_non_transparent, primary_tile_count) + 1;
    }
    return export_image_range(*this, primary_tile_count, end_tile, flip_mode);
}

bool TilesPngWorkspace::at_capacity() const
{
    return cursor_ == capacity_;
}

void TilesPngWorkspace::reserve_anim_slots(std::size_t anim_tile_count, std::size_t start_offset)
{
    const PlainTextFormatter formatter{};

    // Precondition: cursor must be at start_offset (no regular tiles inserted past that point)
    if (cursor_ != start_offset) {
        const auto msg = formatter.format(
            "reserve_anim_slots: cursor ({}) must be at start_offset ({}) before reserving animation slots",
            cursor_,
            start_offset);
        panic(msg);
    }

    // Precondition: animation region must fit in the workspace (with room for at least one regular tile)
    if (start_offset + anim_tile_count >= capacity_) {
        const auto msg = formatter.format(
            "start_offset ({}) + anim_tile_count ({}) must be less than capacity ({}).",
            start_offset,
            anim_tile_count,
            capacity_);
        panic(msg);
    }

    // Store the animation region boundaries
    anim_start_offset_ = start_offset;
    anim_end_offset_ = start_offset + anim_tile_count;

    // Move cursor past the reserved animation region
    cursor_ = anim_end_offset_;
}

void TilesPngWorkspace::place_anim_tile(std::size_t reserved_index, const CanonicalPixelTile<IndexPixel> &tile)
{
    const PlainTextFormatter formatter{};

    // Precondition: animation slots must have been reserved
    if (!has_anim_slots()) {
        panic("place_animation_tile called but no animation slots were reserved");
    }

    // reserved_index is 0-based within the reserved region
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

    // Add to canonical_forms_ map for deduplication support (skip transparent tiles)
    if (!tile.is_transparent()) {
        const PixelTile<IndexPixel> &base_tile = tile;
        canonical_forms_[base_tile].push_back(absolute_index);
    }
}

std::optional<std::size_t> TilesPngWorkspace::find_contiguous_transparent_slots(std::size_t count) const
{
    // Edge case: count of 0 is trivially satisfied at any position
    if (count == 0) {
        return 1; // Return first valid position after tile 0
    }

    // Scan from index 1 (skip reserved tile 0) looking for contiguous transparent runs
    std::size_t run_start = 0;
    std::size_t run_length = 0;

    for (std::size_t i = 1; i < capacity_; ++i) {
        if (tiles_[i].is_transparent()) {
            if (run_length == 0) {
                run_start = i;
            }
            run_length++;

            if (run_length >= count) {
                return run_start;
            }
        }
        else {
            // Non-transparent tile breaks the run
            run_length = 0;
        }
    }

    // No suitable run found
    return std::nullopt;
}

std::optional<std::size_t> TilesPngWorkspace::find_existing_contiguous_tiles_by_color(
    const std::vector<CanonicalPixelTile<IndexPixel>> &tiles,
    const std::vector<const Palette<Rgba32, palette::max_size> *> &palettes) const
{
    // Edge case: empty sequence is trivially found
    if (tiles.empty()) {
        return 1; // Return first valid position after tile 0
    }

    // Precondition: parallel vectors must have same size
    if (tiles.size() != palettes.size()) {
        panic("tiles and palettes vectors must have the same size");
    }

    // Linear scan through workspace looking for contiguous match
    // Start at index 1 (skip reserved tile 0)
    for (std::size_t candidate_start = 1; candidate_start < capacity_; ++candidate_start) {
        // Check if there's enough room for all tiles from this position
        if (candidate_start + tiles.size() > capacity_) {
            break; // No more valid starting positions
        }

        // Verify all tiles in sequence match using color-equivalence
        bool all_match = true;
        for (std::size_t offset = 0; offset < tiles.size(); ++offset) {
            const std::size_t check_index = candidate_start + offset;
            const PixelTile<IndexPixel> &expected_base = tiles[offset];
            const PixelTile<IndexPixel> &actual_base = tiles_[check_index];
            const auto &palette = *palettes[offset];

            if (!tiles_color_equivalent(expected_base, actual_base, palette)) {
                all_match = false;
                break;
            }
        }

        if (all_match) {
            return candidate_start;
        }
    }

    // No contiguous sequence found
    return std::nullopt;
}

void TilesPngWorkspace::place_tiles_at(
    std::size_t start_index, const std::vector<CanonicalPixelTile<IndexPixel>> &tiles)
{
    const PlainTextFormatter formatter{};

    // Precondition: tiles must fit in workspace
    if (start_index + tiles.size() > capacity_) {
        const auto msg = formatter.format(
            "place_tiles_at: start_index ({}) + tiles.size() ({}) exceeds capacity ({})",
            start_index,
            tiles.size(),
            capacity_);
        panic(msg);
    }

    // Precondition: all target positions must be transparent
    for (std::size_t offset = 0; offset < tiles.size(); ++offset) {
        const std::size_t target_index = start_index + offset;
        if (!tiles_[target_index].is_transparent()) {
            const auto msg =
                formatter.format("place_tiles_at: position {} is not transparent, cannot place tile", target_index);
            panic(msg);
        }
    }

    // Place tiles and update canonical_forms_ map
    for (std::size_t offset = 0; offset < tiles.size(); ++offset) {
        const std::size_t target_index = start_index + offset;
        const auto &tile = tiles[offset];

        tiles_[target_index] = tile;

        // Add to canonical_forms_ map (skip transparent tiles, though they shouldn't be in the input)
        if (!tile.is_transparent()) {
            const PixelTile<IndexPixel> &base_tile = tile;
            canonical_forms_[base_tile].push_back(target_index);
        }
    }

    // Advance cursor past any newly-filled positions if needed
    // The cursor should skip over non-transparent tiles
    while (cursor_ < capacity_ && !tiles_[cursor_].is_transparent()) {
        cursor_++;
    }
}

} // namespace porytiles
