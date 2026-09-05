#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/algorithms/color_search.hpp"
#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/domain/services/tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Options for a color search.
struct FindColorOptions {
    /// @brief The color to look for, alpha is ignored.
    Rgba32 color;
    /// @brief The rule that makes a pixel match the color.
    ColorTolerance tolerance;
    /// @brief Maximum number of results to produce, or nullopt for no cap.
    std::optional<std::size_t> limit;
};

/// @brief Options for a color dump.
struct DumpColorsOptions {
    /// @brief Whether to cluster similar colors instead of printing a flat list.
    bool group;
    /// @brief The grouping rule. When unset and @c group is on, @c default_group_tolerance applies.
    std::optional<ColorTolerance> tolerance;

    /// @brief The grouping rule used when the user gives none, defaults to gba grouping
    static constexpr ColorTolerance default_group_tolerance = ColorTolerance::gba();
};

/// @brief Inspects the colors in a tileset's Porytiles @c rgba32 assets.
///
/// @details
/// Implementation for the find-tileset-color and dump-tileset-colors commands. Both load the tileset through the repo,
/// metatileize its layer images, and inspect the resulting metatiles plus its animation frames.
///
/// The methods return the lines to print rather than printing them, so the driver can decide what to do with them.
class InspectTilesetColors {
  public:
    InspectTilesetColors(
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const TilesetMetadataProvider *> metadata_provider,
        gsl::not_null<const PorytilesTilesetManager *> tileset_manager,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const TilePrinter *> tile_printer,
        gsl::not_null<const PalettePrinter *> palette_printer,
        gsl::not_null<const UserDiagnostics *> diag)
        : tileset_repo_{tileset_repo}, metadata_provider_{metadata_provider}, tileset_manager_{tileset_manager},
          domain_config_{domain_config}, format_{format}, tile_printer_{tile_printer},
          palette_printer_{palette_printer}, diag_{diag}
    {
    }

    /// @brief Finds every metatile layer and animation tile containing the requested color.
    ///
    /// @details
    /// Produces a summary line, then one header plus ASCII rendering per match with the matching pixels marked, up to
    /// the requested limit. Matching skips alpha=0 pixels, see @c ColorMatcher.
    ///
    /// @param tileset_name The canonical tileset name
    /// @param options The color, tolerance, and limit
    /// @pre @p tileset_name names an existing tileset
    /// @return The lines to print, or an error when the tileset is unmanaged or fails to load
    [[nodiscard]] ChainableResult<std::vector<std::string>>
    find_color(const std::string &tileset_name, const FindColorOptions &options) const;

    /// @brief Lists every color in the tileset with pixel counts for each, then compares the total against the color
    /// budget.
    ///
    /// @details
    /// The budget is the computed limits from the user's palette count configuration. Colors print in descending count
    /// order, flat or grouped by similarity. Under the GBA grouping rule each group's header also names the actual
    /// color the game displays for all of its members.
    ///
    /// @param tileset_name The canonical tileset name
    /// @param options Grouping and tolerance
    /// @pre @p tileset_name names an existing tileset
    /// @return The lines to print, or an error when the tileset is unmanaged or fails to load
    [[nodiscard]] ChainableResult<std::vector<std::string>>
    dump_colors(const std::string &tileset_name, const DumpColorsOptions &options) const;

  private:
    /// @brief A loaded tileset with its layer images already split into metatiles.
    struct LoadedTileset {
        std::unique_ptr<Tileset> tileset;
        std::vector<Metatile<Rgba32>> metatiles;
        Rgba32 extrinsic_transparency;
    };

    [[nodiscard]] ChainableResult<LoadedTileset> load(const std::string &tileset_name) const;

    const TilesetRepo *tileset_repo_;
    const TilesetMetadataProvider *metadata_provider_;
    const PorytilesTilesetManager *tileset_manager_;
    const DomainConfig *domain_config_;
    const TextFormatter *format_;
    const TilePrinter *tile_printer_;
    const PalettePrinter *palette_printer_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
