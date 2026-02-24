#pragma once

#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

// Forward declaration for backporting mangled tiles
class PorymapTilesetComponent;

/**
 * @brief Decompiles indexed animation tiles to RGBA format.
 *
 * @details
 * AnimDecompiler converts Animation<IndexPixel> (from Porymap format) to Animation<Rgba32> (for Porytiles format).
 * This is used during tileset import to extract animation keyframe tiles from tiles.png and convert them to RGBA
 * format suitable for storage in the Porytiles component.
 *
 * The decompiler uses the tileset's palettes to look up the actual RGBA colors for each palette index. Since animation
 * keyframe tiles may use any palette in the tileset, the decompiler requires access to all palettes.
 *
 * The correct palette for each animation is determined by the configured @c AnimPalResolutionStrategy, which drives
 * the behavior from the start (strategy-first). Available strategies include direct palette selection (@c palette_00
 * through @c palette_15), local metatile scanning (@c scan_local_metatiles), and PNG internal palette matching
 * (@c internal_png_pal).
 */
class AnimDecompiler {
  public:
    explicit AnimDecompiler(
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    /**
     * @brief Decompiles an IndexPixel animation to Rgba32 format.
     *
     * @details
     * Converts each tile in each frame of the animation from IndexPixel to Rgba32 using the provided palettes.
     *
     * Key frame tiles are extracted from tiles_png at the animation's tile_offset position, decompiled to RGBA using
     * the resolved palette, and set on the result animation.
     *
     * For transparent pixels (index 0), the extrinsic transparency color is used.
     *
     * The palette is determined by the @p pal_strategy parameter (strategy-first):
     * - @c palette_00 through @c palette_15: Use the specified palette index directly.
     * - @c scan_local_metatiles: Scan metatile entries to find which palette references the animation tiles. Errors if
     *   the tiles are not referenced in any metatile.
     * - @c internal_png_pal: Match frame PNG internal palettes against tileset palettes.
     * - @c scan_all_tilesets: Not yet implemented.
     *
     * If duplicate key frame tiles are detected, the behavior depends on the @p key_frame_strategy parameter:
     * - @c error: Returns an error with details about which tiles are duplicates.
     * - @c mangle: Modifies duplicate tiles to make them unique and backports changes to tiles.png via
     *   @p porymap_component.
     *
     * @param anim The indexed animation to decompile.
     * @param pals Array of palettes to use for color lookup.
     * @param metatiles_bin The metatile entries containing tile and palette references.
     * @param tiles_png The indexed tiles.png image containing key frame tiles.
     * @param inter_anim_canonical_tiles Canonical forms of previously-processed animations' key frame tiles, used to
     *   detect inter-animation duplicates. Pass an empty set when processing the first animation.
     * @param extrinsic_transparency The RGBA color representing transparency.
     * @param pal_strategy The strategy for determining which palette to use for the animation tiles.
     * @param key_frame_strategy The strategy for handling duplicate key frame tiles.
     * @param porymap_component The Porymap component to backport tile changes to (may be nullptr to skip backporting).
     * @return The decompiled RGBA animation with key frame and frames populated, or error.
     */
    [[nodiscard]] ChainableResult<Animation<Rgba32>> decompile_animation(
        const Animation<IndexPixel> &anim,
        const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
        std::span<const TilemapEntry> metatiles_bin,
        const Image<IndexPixel> &tiles_png,
        const std::set<PixelTile<IndexPixel>> &inter_anim_canonical_tiles,
        const ConfigValue<Rgba32> &extrinsic_transparency,
        const ConfigValue<AnimPalResolutionStrategy> &pal_strategy,
        const ConfigValue<AnimKeyFrameResolutionStrategy> &key_frame_strategy,
        PorymapTilesetComponent *porymap_component) const;

  private:
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles2
