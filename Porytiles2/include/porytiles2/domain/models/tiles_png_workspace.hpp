#pragma once

#include <map>
#include <optional>
#include <vector>

#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"

namespace porytiles2 {

/**
 * @brief A workspace for managing canonical IndexPixel tiles destined for tiles.png output.
 *
 * @details
 * TilesPngWorkspace provides a fixed-capacity container for \link PixelTile PixelTiles \endlink that will be written to
 * the tiles.png output file in the PorymapTilesetComponent of a Tileset. The workspace manages tile deduplication,
 * transparent tile slots, and efficient insertion through cursor-based tracking.
 *
 * Key Features:
 * - Pre-allocated storage: The workspace pre-allocates all tile slots up to capacity with transparent tiles
 * - Tile 0 reservation: Index 0 is always reserved for a transparent tile per pokeemerald tileset conventions
 * - Cursor-based insertion: Tracks the next available transparent slot for O(1) insertion in the common case
 * - Deduplication support: Maintains a mapping of canonical tile forms to their first occurrence indices
 * - Transparent tile rejection: Refuses to insert transparent tiles (except the reserved tile 0)
 *
 * Storage Model:
 * The workspace uses a pre-allocated vector where all slots are initialized with transparent tiles. Non-transparent
 * tiles are inserted by replacing transparent tiles at the cursor position. The cursor always points to the next
 * available transparent slot, skipping over already-inserted non-transparent tiles.
 *
 * Canonical Forms:
 * Only non-transparent tiles are tracked in the canonical forms map for deduplication purposes. This map stores the
 * canonical (lexicographically minimal) representation of each unique tile and all indices where that tile appears in
 * the workspace.
 */
class TilesPngWorkspace {
  public:
    /**
     * @brief Constructs a TilesPngWorkspace with a specified capacity, initializing all slots with transparent tiles.
     *
     * @details
     * Creates a workspace that can hold up to `capacity` tiles. All tile slots are pre-initialized with transparent
     * tiles (tiles where every pixel is IndexPixel(0)). The cursor is initialized to position 1, as position 0 is
     * reserved for the mandatory transparent tile per GBA tileset requirements.
     *
     * After construction:
     * - tiles_ vector contains `capacity` transparent tiles
     * - cursor_ points to index 1 (first available slot after the reserved tile 0)
     * - canonical_forms_ map is empty (transparent tiles are not tracked)
     * - The workspace is ready to accept non-transparent tile insertions
     *
     * @param capacity The maximum number of tiles this workspace can hold
     */
    explicit TilesPngWorkspace(std::size_t capacity);

    /**
     * @brief Constructs a TilesPngWorkspace from an existing IndexPixel image and capacity.
     *
     * @details
     * Extracts 8×8 pixel tiles from the input image in row-major order and populates the workspace. The image
     * dimensions must be multiples of 8 (the tile dimension). If the image contains more tiles than the specified
     * capacity, the constructor panics.
     *
     * Extraction Process:
     * 1. Validates image dimensions are multiples of 8
     * 2. Calculates the number of tiles in the image (width/8 × height/8)
     * 3. Verifies the tile count does not exceed capacity
     * 4. Extracts each 8×8 tile region from the image in row-major order
     * 5. Creates a CanonicalPixelTile for each extracted tile
     * 6. Adds non-transparent tiles to the canonical_forms_ map for deduplication
     * 7. Pads the tiles_ vector to capacity with transparent tiles
     * 8. Sets the cursor to the first transparent tile after tile 0
     *
     * Non-transparent tile tracking:
     * Only tiles that contain at least one non-zero IndexPixel are added to canonical_forms_. This enables efficient
     * deduplication via first_occurrence_of() lookups.
     *
     * Cursor initialization:
     * After loading the image, the cursor is positioned at the first transparent tile index greater than 0.
     * If all tiles from the image are non-transparent, the cursor points to the first padding tile.
     *
     * @pre Image dimensions must be multiples of 8
     * @pre Image may not contain more tiles than the specified capacity
     *
     * @param img The IndexPixel image to extract tiles from; dimensions must be multiples of 8
     * @param capacity The maximum number of tiles this workspace can hold
     */
    explicit TilesPngWorkspace(const Image<IndexPixel> &img, std::size_t capacity);

    /**
     * @brief Attempts to insert a non-transparent tile into the workspace at the current cursor position.
     *
     * @details
     * Inserts the given canonical tile at the cursor position, replacing the transparent tile that was there. After
     * insertion, the cursor is advanced to the next available transparent tile slot. The inserted tile is added to the
     * canonical_forms_ map for deduplication support.
     *
     * Insertion Criteria:
     * - Returns false if the workspace is at capacity (cursor >= capacity)
     * - Returns false if the tile is transparent (no point inserting transparent tiles)
     * - Returns true if the tile was successfully inserted
     *
     * Post-insertion State:
     * - The tile replaces the transparent tile at the cursor position
     * - The tile's canonical form is added to canonical_forms_ with its index
     * - The cursor advances to the next transparent tile (or reaches capacity)
     * - Fast-forward: The cursor skips over any non-transparent tiles to find the next available slot
     *
     * Cursor Fast-Forward:
     * After inserting a tile, the cursor is incremented and then fast-forwarded through any non-transparent tiles until
     * it finds the next transparent slot or reaches capacity. This ensures O(1) insertion for the next insert_tile call
     * in the common case.
     *
     * @param tile The canonical pixel tile to insert; must be non-transparent for successful insertion
     * @return true if the tile was successfully inserted, false if the workspace is at capacity or the tile is
     * transparent
     */
    [[nodiscard]] bool insert_tile(const CanonicalPixelTile<IndexPixel> &tile);

    /**
     * @brief Finds the first occurrence index of a given canonical tile in the workspace.
     *
     * @details
     * Searches the canonical_forms_ map for the given tile and returns the index of its first occurrence in the
     * workspace. This method only finds non-transparent tiles that have been explicitly inserted or loaded from an
     * image during construction.
     *
     * Deduplication Use Case:
     * This method is used to check if a tile already exists in the workspace before inserting it. If the tile is found,
     * the caller can reuse the existing tile's index rather than inserting a duplicate.
     *
     * Transparent Tiles:
     * Transparent tiles are never tracked in canonical_forms_, so this method will always return std::nullopt for any
     * transparent tile, even though the workspace may contain many transparent tiles. Since tile 0 is guaranteed to be
     * transparent, callers who need to index into a transparent tile can always just use tile 0 instead of searching
     * for one.
     *
     * @param tile The canonical pixel tile to search for
     * @return The index of the tile's first occurrence if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::size_t> first_occurrence_of(const CanonicalPixelTile<IndexPixel> &tile) const;

    /**
     * @brief Retrieves the canonical tile at the specified index in the workspace.
     *
     * @details
     * Returns a copy of the CanonicalPixelTile at the given index. The index must be within the bounds of the tiles_
     * vector (0 to capacity - 1). This method provides read-only access to any tile in the workspace, including
     * transparent tiles and non-transparent tiles.
     *
     * Bounds Checking:
     * If the index is out of bounds, the method panics with an error message indicating the invalid index and the
     * workspace size.
     *
     * @param index The zero-based index of the tile to retrieve; must be < capacity
     * @return A copy of the CanonicalPixelTile at the specified index
     *
     * @throws Panics if index >= tiles_.size()
     */
    [[nodiscard]] CanonicalPixelTile<IndexPixel> tile_at(std::size_t index) const;

    /**
     * @brief Exports the workspace tiles to an Image<IndexPixel> in canonical form (tiles.png format).
     *
     * @details
     * Creates an Image<IndexPixel> representation of all tiles in the workspace, arranged in row-major order with 16
     * tiles per row (128 pixels wide). This method exports tiles in their canonical (lexicographically minimal) form
     * as stored in the workspace, without applying any flip transformations.
     *
     * Image Layout:
     * - Width: Always 128 pixels (16 tiles × 8 pixels per tile)
     * - Height: Calculated to accommodate all tiles in the workspace (must be a multiple of 8)
     * - Tile arrangement: Row-major order, matching the extraction order from the constructor
     *
     * Canonical Form:
     * Each tile is exported exactly as stored in the workspace - in its canonical (lexicographically minimal)
     * orientation among all flip variants. Flip flags are not applied during export. This is the appropriate format
     * for tiles.png output in fresh compilations, where there is no "original" tile orientation to preserve.
     *
     * @return An Image<IndexPixel> containing all workspace tiles in canonical form (tiles.png format)
     */
    [[nodiscard]] Image<IndexPixel> export_canonical_image() const;

    /**
     * @brief Exports the workspace tiles to an Image<IndexPixel> in original (pre-canonicalization) form.
     *
     * @details
     * Creates an Image<IndexPixel> representation of all tiles in the workspace, arranged in row-major order with 16
     * tiles per row (128 pixels wide). This method applies flip transformations to restore tiles to their original
     * orientations as they were before canonicalization.
     *
     * Image Layout:
     * - Width: Always 128 pixels (16 tiles × 8 pixels per tile)
     * - Height: Calculated to accommodate all tiles in the workspace (must be a multiple of 8)
     * - Tile arrangement: Row-major order, matching the extraction order from the constructor
     *
     * Original Form Restoration:
     * Each tile is exported with flip transformations applied based on its stored h_flip and v_flip flags. This
     * reverses the canonicalization process, restoring tiles to their original pixel arrangements. This enables
     * round-trip preservation: original image → workspace → exported original image, which is particularly useful in
     * incremental builds.
     *
     * @return An Image<IndexPixel> containing all workspace tiles in original (flipped) form
     */
    [[nodiscard]] Image<IndexPixel> export_original_image() const;

    /**
     * @brief Checks if the workspace has reached capacity and can no longer accept new tile insertions.
     *
     * @details
     * Returns true when the cursor has reached or exceeded the capacity, indicating that all available transparent tile
     * slots have been filled with non-transparent tiles. At this point, insert_tile() will return false for any further
     * insertion attempts.
     *
     * Note: This method returns true even if there are still transparent tiles in the workspace beyond the cursor
     * position, as the cursor always advances forward and never backtracks.
     *
     * @return true if cursor == capacity, false otherwise
     */
    [[nodiscard]] bool at_capacity() const;

    /**
     * @brief Returns the maximum number of tiles this workspace can hold.
     *
     * @details
     * Returns the capacity value specified during construction. This is the fixed size of the tiles_ vector and
     * represents the maximum number of tiles (both transparent and non-transparent) that can be stored.
     *
     * @return The workspace capacity
     */
    [[nodiscard]] std::size_t capacity() const
    {
        return capacity_;
    }

  private:
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_;
    std::map<PixelTile<IndexPixel>, std::vector<std::size_t>> canonical_forms_;
    std::size_t cursor_;
    std::size_t capacity_;
};

} // namespace porytiles2
