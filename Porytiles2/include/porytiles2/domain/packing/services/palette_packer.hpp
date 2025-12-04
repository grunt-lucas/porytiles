#pragma once

#include <array>
#include <map>
#include <optional>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief The input parameters for a packing operation.
 */
struct PackingParams {
    // TODO: could the fields be refs or ptrs?

    /**
     * @brief Raw pixel tiles to pack into palettes.
     */
    std::vector<PixelTile<Rgba32>> tiles_;

    /**
     * @brief Bidirectional mapping between Rgba32 colors and ColorIndex
     */
    ColorIndexMap<Rgba32> color_map_;

    /**
     * @brief The extrinsic transparency color (e.g., rgba_magenta)
     */
    Rgba32 extrinsic_transparency_;

    /**
     * @brief Existing palettes with locked and wildcarded colors (from PorytilesTilesetComponent)
     */
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals_;

    /**
     * @brief Priority tiles that guide packing (e.g., ensure certain colors group together)
     */
    std::vector<PaletteHint> hints_;

    /**
     * @brief Bitset specifying which hardware palettes are available for packing
     */
    std::bitset<pal::num_pals> available_pals_;
};

/**
 * @brief Result from the high-level tile packing operation.
 *
 * @details
 * PalettePacking contains the final hardware palettes and the mapping from tile IDs to their assigned palette indices
 * after a successful packing operation.
 */
struct PalettePacking {
    /**
     * @brief The final hardware palettes with colors assigned.
     *
     * @details
     * Each palette is indexed by its hardware palette index (0, 1, 2, ...). A `std::nullopt` entry signifies that the
     * palette packer performed no assignments for that palette index; the corresponding palette in the Porymap
     * component will be left untouched. For populated palettes, slot 0 contains the transparency color (preserved from
     * input or defaulted), remaining slots contain packed colors, and unfilled slots contain Rgba32{0, 0, 0}.
     */
    // TODO: make unfilled slot value configurable
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> pals_;

    /**
     * @brief Maps regular tile ids to their assigned hardware palette indices.
     *
     * @details
     * The key is the tile id (index). The value is the hardware palette index to which the tile was assigned.
     */
    std::map<std::size_t, std::size_t> tile_to_pal_;

    // TODO: need an anim_to_palette version once we implement
};

/**
 * @brief Domain service that packs \link ColorSet ColorSets\endlink into hardware palettes.
 *
 * @details
 * PalettePacker solves the "Pagination Problem" (Bin Packing with Overlapping Items) - assigning tiles to palettes such
 * that each tile's colors fit within a single palette's capacity, while minimizing the number of palettes used.
 *
 * The packer delegates the actual algorithm to a PackingStrategy, allowing different algorithms (Best Fusion,
 * Overload-and-Remove) to be used interchangeably.
 *
 * Key features:
 * - **Hints**: Priority tiles assigned before regular tiles
 * - **Prefilled palettes**: Pre-assigned palette slots (e.g., from primary tileset when compiling secondary, fixed
 * slots for DNS window colors, etc.)
 * - **Strategy pattern**: Pluggable algorithms for different trade-offs
 */
class PalettePacker {
  public:
    /**
     * @brief Constructs a PalettePacker with the specified dependencies.
     *
     * @param strategy The packing algorithm to use
     * @param format TextFormatter for building diagnostic output
     * @param diag UserDiagnostics for warnings and errors
     */
    explicit PalettePacker(
        gsl::not_null<const PackingStrategy *> strategy,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : strategy_{strategy}, format_{format}, diag_{diag}
    {
    }

    void strategy(gsl::not_null<const PackingStrategy *> strategy)
    {
        strategy_ = strategy;
    }

    /**
     * @brief Packs pixel tiles into hardware palettes using a high-level API.
     *
     * @details
     * This method provides a caller-friendly interface that accepts Rgba32 pixel tiles and palettes, handling all
     * internal conversions to/from ColorSet and ColorIndex.
     *
     * The method:
     * 1. Converts input pixel tiles to PackableTile with ColorSet
     * 2. Converts palette hints to PackableTile hints
     * 3. Converts input palettes from PorytilesTilesetComponent to PrefilledPalette constraints
     * 4. Delegates to packing strategy
     * 5. Converts PackedPalette results back to Palette<Rgba32, pal::max_size>
     *
     @ @param params The packing input parameters
     *
     * @pre All colors in tiles, input_palettes, and hints must exist in color_map
     * @return PalettePacking on success, or an error describing the failure
     */
    [[nodiscard]] ChainableResult<PalettePacking> pack_tiles(const PackingParams &params) const;

  private:
    const PackingStrategy *strategy_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
