#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
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
 * AnimationDecompiler converts Animation<IndexPixel> (from Porymap format) to Animation<Rgba32> (for Porytiles format).
 * This is used during tileset import to extract animation keyframe tiles from tiles.png and convert them to RGBA
 * format suitable for storage in the Porytiles component.
 *
 * The decompiler uses the tileset's palettes to look up the actual RGBA colors for each palette index. Since animation
 * keyframe tiles may use any palette in the tileset, the decompiler requires access to all palettes.
 *
 * The correct palette for each animation tile is recovered by scanning metatile entries (metatiles_bin) to find which
 * palette index is used when referencing the animation tile. If a tile is referenced with multiple different palette
 * indices (ambiguous), the most common one is used.
 */
class AnimationDecompiler {
  public:
    explicit AnimationDecompiler(
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
     * Converts each tile in each frame of the animation from IndexPixel to Rgba32 using the provided palettes. The
     * correct palette for each animation tile is recovered by scanning metatile entries to find which palette index
     * references the animation tiles.
     *
     * Key frame tiles are extracted from tiles_png at the animation's tile_offset position, decompiled to RGBA using
     * the same palette as the animation frames, and set on the result animation.
     *
     * For transparent pixels (index 0), the extrinsic transparency color is used.
     *
     * If an animation tile is not found in any metatile entry, the behavior depends on the pal_strategy parameter:
     * - default_pal: Falls back to palette 0
     * - internal_png_palette: Attempts to match frame PNG internal palettes against tileset palettes
     * - full_tileset_scan: Not yet implemented (panics)
     *
     * If multiple palettes reference the same tile (ambiguous), the most common palette index is used.
     *
     * If duplicate key frame tiles are detected, the behavior depends on the key_frame_strategy parameter:
     * - error: Returns an error with details about which tiles are duplicates
     * - mangle: Modifies duplicate tiles to make them unique and backports changes to tiles.png via porymap_component
     *
     * @param anim The indexed animation to decompile.
     * @param pals Array of palettes to use for color lookup.
     * @param metatiles_bin The metatile entries containing tile and palette references.
     * @param tiles_png The indexed tiles.png image containing key frame tiles.
     * @param extrinsic_transparency The RGBA color representing transparency.
     * @param pal_strategy The strategy for resolving palette when no metatile reference is found.
     * @param key_frame_strategy The strategy for handling duplicate key frame tiles.
     * @param porymap_component The Porymap component to backport tile changes to (may be nullptr to skip backporting).
     * @return The decompiled RGBA animation with key frame and frames populated, or error.
     */
    [[nodiscard]] ChainableResult<Animation<Rgba32>> decompile_animation(
        const Animation<IndexPixel> &anim,
        const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
        std::span<const TilemapEntry> metatiles_bin,
        const Image<IndexPixel> &tiles_png,
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
