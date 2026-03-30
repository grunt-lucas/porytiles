#pragma once

#include <compare>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief A single pixel modification within a tile.
 *
 * @details
 * Records which pixel was changed and the before/after values. Multiple PixelMangleChange entries can exist per tile
 * when multi-pixel mangling is required (e.g., for solid-color tiles where single-pixel swaps are insufficient).
 */
struct PixelMangleChange {
    std::size_t pixel_index{}; ///< Which pixel in the tile (0-63, linear index)
    IndexPixel original_pixel; ///< The pixel value before mangling
    IndexPixel mangled_pixel;  ///< The pixel value after mangling
};

/**
 * @brief Record of all pixel modifications made to a single tile during mangling.
 *
 * @details
 * When a tile is mangled to make it unique, this struct records the exact changes made: which tile was modified and
 * which pixels were changed. This information is needed to backport the changes to the tiles.png image.
 *
 * Each TileMangleRecord targets a distinct tile_index. No two records share the same tile_index.
 * This invariant is guaranteed by mangle_duplicates, which visits each tile at most once.
 * A record contains one or more pixel changes (single-pixel mangles are most common, but multi-pixel
 * mangles are used as a fallback when single-pixel swaps are insufficient).
 */
struct TileMangleRecord {
    std::size_t tile_index{}; ///< Which tile in the key frame (0-based index, unique across records)
    std::vector<PixelMangleChange> pixel_changes; ///< One or more pixel modifications applied to this tile

    /// @brief Orders records by tile_index, enabling storage in std::set to enforce the non-overlapping invariant.
    [[nodiscard]] friend auto operator<=>(const TileMangleRecord &lhs, const TileMangleRecord &rhs)
    {
        return lhs.tile_index <=> rhs.tile_index;
    }

    [[nodiscard]] friend bool operator==(const TileMangleRecord &lhs, const TileMangleRecord &rhs)
    {
        return lhs.tile_index == rhs.tile_index;
    }
};

/**
 * @brief Result of the mangling operation.
 *
 * @details
 * Contains the potentially modified tiles and a record of all modifications made. If no duplicates were found, the
 * mangle_records vector will be empty and tiles will be unchanged.
 */
struct MangleResult {
    std::vector<PixelTile<IndexPixel>> tiles; ///< The tiles after mangling (unique)
    // Keyed by tile_index: the set structurally enforces that each record targets a distinct tile.
    std::set<TileMangleRecord> mangle_records; ///< Record of all modifications made (ordered by tile_index)
};

/**
 * @brief Service that mangles duplicate key frame tiles to make them unique.
 *
 * @details
 * During animation decompilation, multiple key frame tiles may be identical. This is problematic because recompilation
 * cannot distinguish between identical tiles. This service modifies duplicate tiles by swapping individual pixels to
 * visually similar colors that already exist in the tile, making each tile unique.
 *
 * The mangling algorithm:
 * 1. Finds a pixel to modify (prefers corners → edges → interior for minimal visual impact)
 * 2. Swaps to a similar color that already exists in the tile (no new colors introduced)
 * 3. Verifies the modified tile is unique against all existing tiles
 * 4. Preserves the palette_index (upper 4 bits) for true-color mode compatibility
 *
 * The service uses palette-aware RGB distance to find visually similar color alternatives, ensuring the mangled tile
 * looks as close as possible to the original while still being technically unique.
 */
class AnimKeyFrameMangler {
  public:
    explicit AnimKeyFrameMangler(
        gsl::not_null<const UserDiagnostics *> diag, gsl::not_null<const TilePrinter *> tile_printer);

    virtual ~AnimKeyFrameMangler() = default;

    /**
     * @brief Mangles duplicate tiles to make them unique.
     *
     * @details
     * Processes the input tiles and ensures all duplicates are modified to be unique. The algorithm finds duplicates,
     * selects pixels to modify based on visual impact priority (corners first, then edges, then interior), and swaps
     * colors to visually similar alternatives already present in the tile.
     *
     * @param anim_name Animation name (for diagnostic messages)
     * @param tiles Key frame tiles to process (will be copied and potentially modified)
     * @param palettes Per-tile palettes for color similarity calculations (one pointer per tile)
     * @param extrinsic_transparency The extrinsic transparency color (for tile color conversion in diagnostics)
     * @param existing_tiles Set of all existing tiles to check uniqueness against
     * @pre @p palettes size must equal @p tiles size.
     * @return MangleResult containing unique tiles and a record of all modifications, or an error if mangling failed
     */
    [[nodiscard]] ChainableResult<MangleResult> mangle_duplicates(
        const std::string &anim_name,
        std::vector<PixelTile<IndexPixel>> tiles,
        const std::vector<const Palette<Rgba32, pal::max_size> *> &palettes,
        const Rgba32 &extrinsic_transparency,
        const std::set<PixelTile<IndexPixel>> &existing_tiles) const;

  private:
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
};

} // namespace porytiles2
