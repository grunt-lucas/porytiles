#pragma once

#include <bitset>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "porytiles/domain/packing/models/packable_tile.hpp"
#include "porytiles/domain/packing/models/packed_palette.hpp"
#include "porytiles/domain/packing/models/palette_pool.hpp"
#include "porytiles/domain/packing/models/prefilled_palette.hpp"
#include "porytiles/domain/packing/models/shape_group_metadata.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Input data aggregate for the low-level palette packing algorithm.
struct PackingInput {
    /// @brief Regular tiles to pack into palettes.
    std::vector<PackableTile> tiles_{};

    /// @brief Priority "hint" tiles that can be assigned before regular tiles.
    ///
    /// @details
    /// Hints can optionally be processed first by the packing algorithm. This allows users to ensure certain colors
    /// group together during packing.
    std::vector<PackableTile> hints_{};

    /// @brief Pre-assigned palettes with fixed colors.
    ///
    /// @details
    /// These palettes represent hardware palettes that already have some colors assigned. The packer must work around
    /// these constraints when assigning tiles.
    std::set<PrefilledPalette> prefilled_pals_{};

    /// @brief A bitset marking which palettes are available for editing.
    ///
    /// @details
    /// For primary tilesets, typically the first six bits (bits 0 through 5) should be set. For secondary tilesets,
    /// this is typically bits 0 through 12, with the first six palettes fully locked as \link PrefilledPalette
    /// PrefilledPalettes\endlink.
    ///
    /// In some configurations, out-of-band palettes may be enabled for packing. This corresponds to the "Primary
    /// Palette Fixing" case (see topic_staging_area.md), a technique used in the community to expand available primary
    /// tileset colors without modifying fieldmap parameters.
    PalettePool pal_pool_;

    /// @brief Maximum number of colors per palette.
    ///
    /// @details
    /// GBA hardware palettes have 16 slots, but slot 0 is transparency, leaving 15 usable color slots. Default is 15.
    std::size_t pal_capacity_ = pal::max_size - 1;

    /// @brief Optional shape group metadata for sharing-aware packing.
    ///
    /// @details
    /// When present, strategies should prefer placing shape group siblings in different palettes to maximize tile
    /// sharing opportunities. Each strategy applies sharing awareness differently:
    /// - BestFusion/O&R: adds a cost penalty when a candidate palette contains a sibling.
    /// - Backtracking: sorts sibling-containing palettes lower in the candidate list.
    ///
    /// When @c std::nullopt, strategy behavior is identical to non-sharing-aware packing.
    std::optional<ShapeGroupMetadata> shape_group_metadata_{};
};

/// @brief The final palette assignments after a successful packing operation.
struct PackingOutput {
    /// @brief The packed palettes with their colors and assigned tiles.
    ///
    /// @details
    /// Each PackedPalette contains the accumulated colors and the map of tile IDs assigned to that palette.
    std::vector<PackedPalette> pals_;

    /// @brief Maps tile IDs to their assigned hardware palette indices.
    ///
    /// @details
    /// This provides a quick lookup for finding which palette a specific tile was assigned to. The mapped value is the
    /// hardware palette index (i.e., `PackedPalette::hardware_index()`), NOT the index into the `palettes_` vector.
    std::map<PackableTile::Id, std::size_t> tile_to_pal_;
};

/// @brief Abstract interface for palette packing algorithms.
///
/// @details
/// PackingStrategy defines the interface for algorithms that solve the "Pagination Problem" (Bin Packing with
/// Overlapping Items, aka "VM packing"). Different strategies may use different approaches such as greedy algorithms
/// (Best Fusion) or non-greedy algorithms with backtracking (Overload-and-Remove, Classic DFS, Classic BFS).
///
/// Implementations should be stateless - all state needed for packing is provided via the PackingInput parameter.
class PackingStrategy {
  public:
    virtual ~PackingStrategy() = default;

    /// @brief Packs tile colors into palettes according to the strategy's algorithm.
    ///
    /// @details
    /// Attempts to assign all tiles from the input to palettes such that each tile's colors fit within at least one
    /// palette's capacity.
    ///
    /// @param input The packing input containing tiles, hints, fixed slots, and constraints
    /// @return A PackingOutput on success, or an error if packing is not possible
    [[nodiscard]] virtual ChainableResult<PackingOutput> pack(const PackingInput &input) const = 0;

    /// @brief Returns the human-readable name of this strategy.
    ///
    /// @return The strategy name (e.g., "Best Fusion", "Overload-and-Remove")
    [[nodiscard]] virtual std::string name() const = 0;
};

} // namespace porytiles
