#pragma once

#include <array>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/config/tile_sharing_alignment.hpp"
#include "porytiles2/domain/config/tile_sharing_packing.hpp"
#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/color_position.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/packing/services/packing_strategy.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief The input parameters for a packing operation.
 */
struct PackingParams {
    // TODO: create a required ctor so default is deleted, prevent callers from screwing up

    // TODO: could the fields be refs or ptrs?

    /**
     * @brief Raw pixel tiles to pack into palettes.
     */
    std::vector<PixelTile<Rgba32>> tiles_;

    /**
     * @brief RGBA animations to pack into palettes.
     */
    std::map<std::string, Animation<Rgba32>> anims_;

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

    /**
     * @brief Reconstructed RGBA tiles from a compiled primary tileset for cross-tileset shape group analysis.
     *
     * @details
     * Each entry is a (pixel tile, hw palette index) pair. These tiles participate in shape group analysis to detect
     * cross-tileset sharing opportunities, but are never packed by the packer (their palette assignments are fixed).
     * Empty for primary compilation or standalone secondary compilation without a paired primary.
     */
    std::vector<std::pair<PixelTile<Rgba32>, std::size_t>> primary_tiles_;

    /**
     * @brief Controls whether the packer considers shape group membership during packing.
     *
     * @details
     * When set to biased, a soft cost penalty steers shape group siblings toward different palettes.
     * When off (default), the packer ignores shape groups entirely. Wrapped in ConfigValue to carry source
     * information for diagnostic caveat messages.
     */
    ConfigValue<TileSharingPacking> tile_sharing_packing_;

    /**
     * @brief Controls palette slot alignment strategy for tile sharing deduplication.
     *
     * @details
     * When set to greedy, indirect links align palette slot indices for color-isomorphic tiles.
     * When off (default), palettes are filled sequentially with no sharing alignment. Wrapped in ConfigValue
     * to carry source information for diagnostic caveat messages.
     */
    ConfigValue<TileSharingAlignment> tile_sharing_alignment_;
};

/**
 * @brief Result from the high-level tile packing operation.
 *
 * @details
 * PalettePacking contains the final hardware palettes and the mapping from tile IDs to their assigned palette indices
 * after a successful packing operation.
 */
struct PalettePacking {
    PalettePacking() = default;

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

    // Multi-palette anim palette mapping will be a separate future feature
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
 */
class PalettePacker {
  public:
    /**
     * @brief Constructs a PalettePacker with the specified dependencies.
     *
     * @param strategy The packing algorithm to use
     * @param format TextFormatter for building diagnostic output
     * @param diag UserDiagnostics for warnings and errors
     * @param tile_printer TilePrinter for rendering tile ASCII art in diagnostics
     * @param pal_printer PalettePrinter for rendering palette diagnostics
     */
    explicit PalettePacker(
        gsl::not_null<const PackingStrategy *> strategy,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> pal_printer)
        : strategy_{strategy}, format_{format}, diag_{diag}, tile_printer_{tile_printer}, pal_printer_{pal_printer}
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
     * @param params The packing input parameters
     *
     * @pre All colors in tiles, input_palettes, and hints must exist in color_map
     * @return PalettePacking on success, or an error describing the failure
     */
    [[nodiscard]] ChainableResult<PalettePacking> pack_tiles(const PackingParams &params) const;

  private:
    const PackingStrategy *strategy_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *pal_printer_;
};

} // namespace porytiles2
