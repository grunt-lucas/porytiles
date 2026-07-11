#pragma once

#include <map>
#include <optional>
#include <vector>

#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

namespace porytiles {

/// @brief Defines whether flip transformations should be applied during image export.
///
/// @details
/// This enum controls whether tiles are exported in their canonical (lexicographically minimal) form
/// or with flip transformations applied to restore their original orientations.
enum class ExportFlipMode {
    /// @brief Export tiles in canonical form without applying flip transformations.
    ///
    /// @details
    /// Tiles are exported exactly as stored in the workspace - in their canonical (lexicographically minimal)
    /// orientation. This is appropriate for fresh compilations where there is no "original" tile orientation to
    /// preserve.
    canonical,

    /// @brief Export tiles in original form by applying stored flip transformations.
    ///
    /// @details
    /// Tiles are exported with h_flip and v_flip transformations applied to restore their original pixel arrangements
    /// as
    /// they were before canonicalization. This enables round-trip preservation of tile orientations, which is
    /// particularly useful in patch builds.
    original
};

/// @brief Defines how trailing transparent tiles should be handled during image export.
///
/// @details
/// This enum controls whether the exported image should include all tiles up to the workspace capacity or trim trailing
/// transparent tiles to produce a more compact output.
enum class ExportTrimMode {
    /// @brief Include all tiles up to workspace capacity, even if trailing tiles are transparent.
    ///
    /// @details
    /// This mode exports a full image with dimensions calculated to accommodate all tiles in the workspace, including
    /// any transparent tiles at the end. This is the default behavior and maintains compatibility with existing code.
    include_trailing_transparent,

    /// @brief Trim trailing transparent tiles from the exported image.
    ///
    /// @details
    /// This mode finds the last non-transparent tile and exports only tiles up to and including that tile, producing a
    /// more compact image. If all tiles are transparent, the image will contain only tile 0.
    trim_trailing_transparent
};

/// @brief A workspace for managing canonical IndexPixel tiles destined for tiles.png output.
///
/// @details
/// TilesPngWorkspace provides a fixed-capacity container for \link PixelTile PixelTiles \endlink that will be written
/// to the tiles.png output file in the PorymapTilesetComponent of a Tileset. The workspace manages tile deduplication,
/// transparent tile slots, and efficient insertion through cursor-based tracking.
///
/// Storage Model:
/// The workspace uses a pre-allocated vector where all slots are initialized with transparent tiles. Non-transparent
/// tiles are inserted by replacing transparent tiles at the cursor position. The cursor always points to the next
/// available transparent slot, skipping over already-inserted non-transparent tiles.
///
/// Canonical Forms:
/// Only non-transparent tiles are tracked in the canonical forms map for deduplication purposes. This map stores the
/// canonical (lexicographically minimal) representation of each unique tile and all indices where that tile appears in
/// the workspace.
class TilesPngWorkspace {
  public:
    /// @brief Constructs a TilesPngWorkspace with a specified capacity, initializing all slots with transparent tiles.
    ///
    /// @details
    /// Creates a workspace that can hold up to `capacity` tiles. All tile slots are pre-initialized with transparent
    /// tiles (tiles where every pixel is IndexPixel(0)). The cursor is initialized to position 1, as position 0 is
    /// reserved for the mandatory transparent tile per GBA tileset requirements.
    ///
    /// After construction:
    /// - tiles_ vector contains `capacity` transparent tiles
    /// - cursor_ points to index 1 (first available slot after the reserved tile 0)
    /// - canonical_forms_ map is empty (transparent tiles are not tracked)
    /// - The workspace is ready to accept non-transparent tile insertions
    ///
    /// @param capacity The maximum number of tiles this workspace can hold
    explicit TilesPngWorkspace(std::size_t capacity);

    /// @brief Constructs a TilesPngWorkspace from an existing IndexPixel image and capacity.
    ///
    /// @details
    /// Extracts 8×8 pixel tiles from the input image in row-major order and populates the workspace. The image
    /// dimensions must be multiples of 8 (the tile dimension). If the image contains more tiles than the specified
    /// capacity, the constructor panics.
    ///
    /// Extraction Process:
    /// 1. Validates image dimensions are multiples of 8
    /// 2. Calculates the number of tiles in the image (width/8 × height/8)
    /// 3. Verifies the tile count does not exceed capacity
    /// 4. Extracts each 8×8 tile region from the image in row-major order
    /// 5. Creates a CanonicalPixelTile for each extracted tile
    /// 6. Adds non-transparent tiles to the canonical_forms_ map for deduplication
    /// 7. Pads the tiles_ vector to capacity with transparent tiles
    /// 8. Sets the cursor to the first transparent tile after tile 0
    ///
    /// Non-transparent tile tracking:
    /// Only tiles that contain at least one non-zero IndexPixel are added to canonical_forms_. This enables efficient
    /// deduplication via first_occurrence_of() lookups.
    ///
    /// Cursor initialization:
    /// After loading the image, the cursor is positioned at the first transparent tile index greater than 0.
    /// If all tiles from the image are non-transparent, the cursor points to the first padding tile.
    ///
    /// @pre Image dimensions must be multiples of 8
    /// @pre Image may not contain more tiles than the specified capacity
    ///
    /// @param img The IndexPixel image to extract tiles from; dimensions must be multiples of 8
    /// @param capacity The maximum number of tiles this workspace can hold
    explicit TilesPngWorkspace(const Image<IndexPixel> &img, std::size_t capacity);

    /// @brief Creates a workspace pre-loaded with primary tiles for secondary tileset compilation.
    ///
    /// @details
    /// Constructs a workspace with @p total_capacity tiles. Positions 0 through @p primary_tile_count - 1 are populated
    /// from @p primary_tiles_png. Position @p primary_tile_count is reserved as a transparent tile per vanilla
    /// secondary tileset convention. The cursor is set to @p primary_tile_count + 1, ready for secondary tile
    /// insertion.
    ///
    /// Primary tiles are registered in the canonical forms map for deduplication, so secondary tiles that match an
    /// existing primary tile will reuse its global index rather than inserting a duplicate.
    ///
    /// @param primary_tiles_png The compiled primary tileset's tiles.png image
    /// @param primary_tile_count The number of tile slots reserved for primary tiles (e.g. 512)
    /// @param total_capacity The total tile capacity for primary + secondary (e.g. 1024)
    /// @pre @p primary_tiles_png dimensions must be multiples of 8
    /// @pre @p primary_tile_count must be less than @p total_capacity
    /// @return A workspace with primary tiles pre-loaded and cursor positioned for secondary insertion
    [[nodiscard]] static TilesPngWorkspace for_secondary(
        const Image<IndexPixel> &primary_tiles_png, std::size_t primary_tile_count, std::size_t total_capacity);

    /// @brief Creates a workspace for standalone secondary compilation with no paired primary.
    ///
    /// @details
    /// Creates a workspace with @p total_capacity tiles, all transparent. Positions 0 through
    /// @p primary_tile_count are reserved (transparent placeholders for primary tile slots).
    /// Position @p primary_tile_count is the secondary transparent tile (vanilla convention).
    /// The cursor is set to @p primary_tile_count + 1, ready for secondary tile insertion.
    /// No tiles are registered in canonical_forms_ since there are no primary tiles to deduplicate against.
    ///
    /// @param primary_tile_count Number of primary tile slots to reserve.
    /// @param total_capacity Total workspace capacity (primary + secondary).
    /// @pre @p primary_tile_count must be less than @p total_capacity.
    /// @return A workspace with transparent primary region and cursor positioned for secondary insertion.
    [[nodiscard]] static TilesPngWorkspace
    for_standalone_secondary(std::size_t primary_tile_count, std::size_t total_capacity);

    /// @brief Attempts to insert a non-transparent tile into the workspace at the current cursor position.
    ///
    /// @details
    /// Inserts the given canonical tile at the cursor position, replacing the transparent tile that was there. After
    /// insertion, the cursor is advanced to the next available transparent tile slot. The inserted tile is added to the
    /// canonical_forms_ map for deduplication support. The function then returns the index of the inserted tile.
    ///
    /// Insertion Criteria:
    /// - Panics if the workspace is at capacity (cursor >= capacity)
    /// - Returns 0 if the tile is transparent (index 0 is standard location for transparent tile)
    /// - Returns index if the tile was successfully inserted
    ///
    /// Post-insertion State:
    /// - The tile replaces the transparent tile at the cursor position
    /// - The tile's canonical form is added to canonical_forms_ with its index
    /// - The cursor advances to the next transparent tile (or reaches capacity)
    /// - Fast-forward: The cursor skips over any non-transparent tiles to find the next available slot
    ///
    /// Cursor Fast-Forward:
    /// After inserting a tile, the cursor is incremented and then fast-forwarded through any non-transparent tiles
    /// until it finds the next transparent slot or reaches capacity. This ensures O(1) insertion for the next
    /// insert_tile call in the common case.
    ///
    /// @param tile The canonical pixel tile to insert; must be non-transparent for successful insertion
    /// @pre Workspace cursor is less than capacity, i.e., there is room in the workspace for new tiles
    /// @return The index of the inserted tile
    [[nodiscard]] std::size_t insert_tile(const CanonicalPixelTile<IndexPixel> &tile);

    /// @brief Finds the first occurrence index of a given canonical tile in the workspace.
    ///
    /// @details
    /// Searches the canonical_forms_ map for the given tile and returns the index of its first occurrence in the
    /// workspace. This method only finds non-transparent tiles that have been explicitly inserted or loaded from an
    /// image during construction.
    ///
    /// Deduplication Use Case:
    /// This method is used to check if a tile already exists in the workspace before inserting it. If the tile is
    /// found, the caller can reuse the existing tile's index rather than inserting a duplicate.
    ///
    /// Transparent Tiles:
    /// Transparent tiles are never tracked in canonical_forms_, so this method will always return std::nullopt for any
    /// transparent tile, even though the workspace may contain many transparent tiles. Since tile 0 is guaranteed to be
    /// transparent, callers who need to index into a transparent tile can always just use tile 0 instead of searching
    /// for one.
    ///
    /// @param tile The canonical pixel tile to search for
    /// @return The index of the tile's first occurrence if found, std::nullopt otherwise
    [[nodiscard]] std::optional<std::size_t> first_occurrence_of(const CanonicalPixelTile<IndexPixel> &tile) const;

    /// @brief Finds the first occurrence of a tile using color-equivalence comparison.
    ///
    /// @details
    /// Similar to first_occurrence_of(), but uses color-equivalence comparison instead of exact index matching. This
    /// handles the case where palettes contain duplicate colors at different indices. Two pixels are considered
    /// equivalent if they reference the same color value in the palette, even if their indices differ.
    ///
    /// This is necessary for patch/locked builds where vanilla workspace tiles may use a different palette index than
    /// the one computed by index_tile_from_color_tile() (which always picks the first matching index). For example, if
    /// palette slot 7 and slot 14 both contain the same color, vanilla tiles might use slot 14 while our computed tiles
    /// use slot 7 - they should still be considered matching.
    ///
    /// Note: This method performs a linear scan of the workspace (O(capacity × 64)) instead of the O(1) map lookup used
    /// by first_occurrence_of(). This is acceptable because workspace capacity is bounded and this method is only used
    /// in patch/locked modes.
    ///
    /// @param tile The canonical pixel tile to search for
    /// @param palette The palette to use for color lookup
    /// @return The index of the tile's first occurrence if found, std::nullopt otherwise
    [[nodiscard]] std::optional<std::size_t> first_occurrence_of_by_color(
        const CanonicalPixelTile<IndexPixel> &tile, const Palette<Rgba32, pal::max_size> &palette) const;

    /// @brief Retrieves the canonical tile at the specified index in the workspace.
    ///
    /// @details
    /// Returns a copy of the CanonicalPixelTile at the given index. The index must be within the bounds of the tiles_
    /// vector (0 to capacity - 1). This method provides read-only access to any tile in the workspace, including
    /// transparent tiles and non-transparent tiles.
    ///
    /// Bounds Checking:
    /// If the index is out of bounds, the method panics with an error message indicating the invalid index and the
    /// workspace size.
    ///
    /// @param index The zero-based index of the tile to retrieve; must be < capacity
    /// @pre index must be less than tiles_.size()
    /// @return A copy of the CanonicalPixelTile at the specified index
    [[nodiscard]] CanonicalPixelTile<IndexPixel> tile_at(std::size_t index) const;

    /// @brief Exports the workspace tiles to an Image<IndexPixel> in tiles.png format.
    ///
    /// @details
    /// Creates an Image<IndexPixel> representation of tiles in the workspace, arranged in row-major order with 16
    /// tiles per row (128 pixels wide). This method provides flexible control over both flip transformation
    /// application and trailing transparent tile trimming via enum parameters.
    ///
    /// Image Layout:
    /// - Width: Always 128 pixels (16 tiles × 8 pixels per tile)
    /// - Height: Calculated based on the trim_mode parameter
    /// - Tile arrangement: Row-major order, matching the extraction order from the constructor
    ///
    /// Flip Mode:
    /// - ExportFlipMode::canonical: Exports tiles in their canonical (lexicographically minimal) form as stored,
    ///   without applying flip transformations. Appropriate for fresh compilations.
    /// - ExportFlipMode::original: Applies h_flip/v_flip transformations to restore original pixel arrangements,
    ///   enabling round-trip preservation useful in patch builds.
    ///
    /// Trim Mode:
    /// - ExportTrimMode::include_trailing_transparent: Exports all tiles up to workspace capacity.
    /// - ExportTrimMode::trim_trailing_transparent: Exports only tiles up to and including the last non-transparent
    ///   tile, producing a more compact image.
    ///
    /// @param flip_mode Controls whether tiles are exported in canonical or original (flipped) form
    /// @param trim_mode Controls whether trailing transparent tiles are included or trimmed
    /// @return An Image<IndexPixel> containing workspace tiles in the specified format (tiles.png format)
    [[nodiscard]] Image<IndexPixel> export_image(
        ExportFlipMode flip_mode = ExportFlipMode::canonical,
        ExportTrimMode trim_mode = ExportTrimMode::trim_trailing_transparent) const;

    /// @brief Exports only the secondary portion of the workspace (tiles from @p primary_tile_count onward).
    ///
    /// @details
    /// Creates an Image<IndexPixel> containing only the secondary tiles. The tile at position @p primary_tile_count
    /// becomes the first tile (row 0, col 0) in the output image.
    ///
    /// When trimming, if all secondary tiles are transparent, the output will contain at least the secondary
    /// transparent tile at position @p primary_tile_count.
    ///
    /// @param primary_tile_count The number of primary tiles to skip (e.g. 512)
    /// @param flip_mode Controls whether tiles are exported in canonical or original (flipped) form
    /// @param trim_mode Controls whether trailing transparent tiles are included or trimmed
    /// @return An Image<IndexPixel> containing only the secondary tiles in tiles.png format
    [[nodiscard]] Image<IndexPixel> export_secondary_image(
        std::size_t primary_tile_count,
        ExportFlipMode flip_mode = ExportFlipMode::canonical,
        ExportTrimMode trim_mode = ExportTrimMode::trim_trailing_transparent) const;

    /// @brief Checks if the workspace has reached capacity and can no longer accept new tile insertions.
    ///
    /// @details
    /// Returns true when the cursor has reached or exceeded the capacity, indicating that all available transparent
    /// tile slots have been filled with non-transparent tiles. At this point, insert_tile() will panic for any further
    /// insertion attempts.
    ///
    /// Note: This method returns true even if there are still transparent tiles in the workspace behind the cursor
    /// position, as the cursor always advances forward and never backtracks. However, this state shouldn't be reachable
    /// under normal operation conditions.
    ///
    /// @return true if cursor == capacity, false otherwise
    [[nodiscard]] bool at_capacity() const;

    /// @brief Reserves contiguous slots for animation keyframe tiles starting at @p start_offset.
    ///
    /// @details
    /// Animation tiles are placed in a contiguous block to ensure stable offsets that can be referenced by the
    /// generated animation C code. This method reserves a contiguous block of slots for animation tiles and moves the
    /// cursor past the reserved region.
    ///
    /// For primary tilesets, @p start_offset is 1 (after the transparent tile 0). For secondary tilesets, @p
    /// start_offset is @c num_tiles_in_primary + 1 (after the secondary transparent tile at @c num_tiles_in_primary).
    ///
    /// After calling this method:
    /// - Indices @p start_offset through @p start_offset + @p anim_tile_count - 1 (inclusive) are reserved
    /// - The cursor is positioned at @p start_offset + @p anim_tile_count (first slot after reserved region)
    /// - Regular tile insertions via insert_tile() will not overwrite the reserved region
    ///
    /// @param anim_tile_count Total number of tiles to reserve for animation keyframes
    /// @param start_offset The starting index for the animation region (default: 1 for primary tilesets)
    /// @pre Cursor must be at @p start_offset (no regular tiles inserted past that point)
    /// @pre @p start_offset + @p anim_tile_count must be less than capacity
    void reserve_anim_slots(std::size_t anim_tile_count, std::size_t start_offset = 1);

    /// @brief Places an animation keyframe tile at a specific reserved index.
    ///
    /// @details
    /// Places a tile in the reserved animation region. The index is an offset within the reserved region, NOT an
    /// absolute index. For example, if animation_tile_count tiles were reserved:
    /// - place_animation_tile(0, tile) places at absolute index 1
    /// - place_animation_tile(1, tile) places at absolute index 2
    /// - etc.
    ///
    /// @param reserved_index The index within the reserved animation region (0-based)
    /// @param tile The tile to place at the specified position
    /// @pre reserve_animation_slots() must have been called first
    /// @pre reserved_index must be less than the reserved count
    void place_anim_tile(std::size_t reserved_index, const CanonicalPixelTile<IndexPixel> &tile);

    /// @brief Returns the starting absolute index for animation tiles.
    ///
    /// @details
    /// For primary tilesets, animation tiles start at index 1 (after the transparent tile 0). For secondary tilesets,
    /// animation tiles start at @c num_tiles_in_primary + 1 (after the secondary transparent tile). The value is set
    /// by reserve_anim_slots() and defaults to 1.
    ///
    /// @return The animation start index
    [[nodiscard]] std::size_t anim_start_offset() const
    {
        return anim_start_offset_;
    }

    /// @brief Returns the ending absolute index for animation tiles (exclusive).
    ///
    /// @details
    /// Returns the index just past the last reserved animation slot. If no animation slots were reserved, returns 1
    /// (same as animation_start_offset()). Regular tile insertion begins at this index.
    ///
    /// @return The first index after the animation region
    [[nodiscard]] std::size_t anim_end_offset() const
    {
        return anim_end_offset_;
    }

    /// @brief Returns whether animation slots have been reserved.
    ///
    /// @return True if reserve_animation_slots() has been called with a non-zero count
    [[nodiscard]] bool has_anim_slots() const
    {
        return anim_end_offset_ > anim_start_offset_;
    }

    /// @brief Finds the first contiguous run of transparent tiles that can accommodate the requested count.
    ///
    /// @details
    /// Scans the workspace starting from index 1 (skipping reserved tile 0) to find a contiguous sequence of
    /// transparent tiles with length >= count. This is useful in patch mode to find free space for animation
    /// keyframes when they don't already exist in the workspace.
    ///
    /// @param count The number of contiguous transparent slots needed
    /// @return The starting index of the first suitable run, or std::nullopt if no run found
    [[nodiscard]] std::optional<std::size_t> find_contiguous_transparent_slots(std::size_t count) const;

    /// @brief Checks if a sequence of tiles already exists contiguously using color-equivalence comparison.
    ///
    /// @details
    /// Uses color-equivalence comparison to find contiguous tile sequences. This handles the case where palettes
    /// contain duplicate colors at different indices. Two pixels are considered equivalent if they reference the same
    /// color value in the palette, even if their indices differ.
    ///
    /// This is necessary for patch/locked builds where vanilla workspace tiles may use a different palette index than
    /// the one computed by index_tile_from_color_tile() (which always picks the first matching index). For example, if
    /// palette slot 7 and slot 14 both contain the same color, vanilla tiles might use slot 14 while our computed tiles
    /// use slot 7 - they should still be considered matching.
    ///
    /// Note: This method performs a linear scan of the workspace (O(capacity × tiles × 64)). This is acceptable because
    /// animation sequences are typically small and workspace capacity is bounded.
    ///
    /// @param tiles The sequence of canonical tiles to search for
    /// @param palettes Parallel vector of palette pointers corresponding to each tile (for color lookup)
    /// @pre tiles.size() == palettes.size()
    /// @return The starting index if found contiguously, or std::nullopt if not found
    [[nodiscard]] std::optional<std::size_t> find_existing_contiguous_tiles_by_color(
        const std::vector<CanonicalPixelTile<IndexPixel>> &tiles,
        const std::vector<const Palette<Rgba32, pal::max_size> *> &palettes) const;

    /// @brief Places tiles at specific positions for patch mode animation placement.
    ///
    /// @details
    /// Places the provided tiles starting at the specified index. All target positions must be transparent
    /// (the method panics if any position is non-transparent). After placement, the canonical_forms_ map is
    /// updated and the cursor is advanced past any newly-filled positions.
    ///
    /// @param start_index The starting index where tiles should be placed
    /// @param tiles The tiles to place at consecutive positions starting from start_index
    /// @pre All target positions must be transparent
    /// @pre start_index + tiles.size() must not exceed capacity
    void place_tiles_at(std::size_t start_index, const std::vector<CanonicalPixelTile<IndexPixel>> &tiles);

    /// @brief Returns the maximum number of tiles this workspace can hold.
    ///
    /// @details
    /// Returns the capacity value specified during construction. This is the fixed size of the tiles_ vector and
    /// represents the maximum number of tiles (both transparent and non-transparent) that can be stored.
    ///
    /// @return The workspace capacity
    [[nodiscard]] std::size_t capacity() const
    {
        return capacity_;
    }

  private:
    /// @brief Advances the cursor to the next transparent tile slot.
    ///
    /// @details
    /// Increments the cursor and then scans forward to find the next transparent tile. If no transparent tiles
    /// remain, the cursor reaches capacity_. This helper is used after tile insertion/placement to maintain
    /// the cursor invariant.
    void advance_cursor_to_next_transparent();

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_;
    std::map<PixelTile<IndexPixel>, std::vector<std::size_t>> canonical_forms_;
    std::size_t cursor_;
    std::size_t capacity_;
    std::size_t anim_start_offset_{1}; // First index in anim region (defaults to 1 for primary tilesets)
    std::size_t anim_end_offset_{1};   // First index after anim region (defaults to 1, i.e. no animations)
};

} // namespace porytiles
