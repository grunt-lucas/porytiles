#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

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
 *
 * Usage during import:
 * @code
 * AnimationDecompiler decompiler;
 *
 * // Decompile to RGBA, recovering palette from metatile data
 * auto rgba_anim = decompiler.decompile_animation(
 *     index_anim, palettes, metatiles_bin, extrinsic_transparency);
 * @endcode
 */
class AnimationDecompiler {
  public:
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
     * If an animation tile is not found in any metatile entry, falls back to palette 0. If multiple palettes reference
     * the same tile (ambiguous), the most common palette index is used.
     *
     * @param anim The indexed animation to decompile
     * @param pals Array of palettes to use for color lookup
     * @param metatiles_bin The metatile entries containing tile and palette references
     * @param tiles_png The indexed tiles.png image containing key frame tiles
     * @param extrinsic_transparency The RGBA color representing transparency
     * @pre All key frame tiles must be unique (no duplicates)
     * @return The decompiled RGBA animation with key frame and frames populated
     */
    [[nodiscard]] Animation<Rgba32> decompile_animation(
        const Animation<IndexPixel> &anim,
        const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
        std::span<const TilemapEntry> metatiles_bin,
        const Image<IndexPixel> &tiles_png,
        const Rgba32 &extrinsic_transparency) const;
};

} // namespace porytiles2
