#pragma once

#include <set>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

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
 *
 * Configuration values (extrinsic transparency, palette strategy, key frame strategy) are resolved internally from the
 * provided @c DomainConfig, including per-animation palette strategy overrides.
 */
class AnimDecompiler {
  public:
    explicit AnimDecompiler(
        gsl::not_null<const DomainConfig *> config,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : config_{config}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
    {
    }

    /**
     * @brief Decompiles an IndexPixel animation to Rgba32 format.
     *
     * @details
     * Converts each tile in each frame of the animation from IndexPixel to Rgba32 using the palettes from
     * @p porymap_component. Configuration values (extrinsic transparency, palette resolution strategy, key frame
     * resolution strategy) are unwrapped internally from the @c DomainConfig provided at construction. Per-animation
     * palette strategy overrides are resolved automatically.
     *
     * Key frame tiles are extracted from tiles_png at the animation's tile_offset position, decompiled to RGBA using
     * the resolved palette, and set on the result animation.
     *
     * If duplicate key frame tiles are detected, the behavior depends on the configured key frame strategy:
     * - @c error: Returns an error with details about which tiles are duplicates.
     * - @c mangle: Modifies duplicate tiles to make them unique and backports changes to tiles.png via
     *   @p porymap_component.
     *
     * @param tileset_name The name of the tileset being decompiled (used for config scoping).
     * @param anim The indexed animation to decompile.
     * @param inter_anim_canonical_tiles Canonical forms of previously-processed animations' key frame tiles, used to
     *   detect inter-animation duplicates. Pass an empty set when processing the first animation.
     * @param porymap_component The Porymap component to read pals/metatiles/tiles from and backport tile changes to.
     * @return The decompiled RGBA animation with key frame and frames populated, or error.
     */
    [[nodiscard]] ChainableResult<Animation<Rgba32>> decompile_animation(
        const std::string &tileset_name,
        const Animation<IndexPixel> &anim,
        const std::set<PixelTile<IndexPixel>> &inter_anim_canonical_tiles,
        PorymapTilesetComponent &porymap_component) const;

  private:
    const DomainConfig *config_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles
