#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/config/import_transparency_mode.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

class MetatileDecompiler {
  public:
    explicit MetatileDecompiler(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        const Rgba32 &extrinsic_transparency)
        : format_{format}, diag_{diag}, tile_printer_{tile_printer}, extrinsic_transparency_{extrinsic_transparency}
    {
    }

    /// @brief Decompiles triple-layerized tilemap entries into RGBA metatiles.
    ///
    /// @details
    /// Each group of 12 entries becomes one Metatile<Rgba32>. Entries 0-3 are the bottom layer, 4-7 the middle layer,
    /// and 8-11 the top layer. Every entry's tile is looked up in @p tiles_png, flipped per the entry's flip flags, and
    /// colored with the entry's palette. Palette index 0 pixels are transparent and are written as either alpha 0 or
    /// the extrinsic transparency color, selected per layer by the provided @c ImportTransparencyMode.
    ///
    /// @param entries The triple-layerized tilemap entries to decompile
    /// @param tiles_png The indexed tile sheet the entries reference
    /// @param palettes The palettes the entries reference
    /// @param mode Selects the pixel value used for transparent pixels
    /// @param synthesized_layers Per-metatile layer that triple-layerization synthesized because it was absent from
    /// the dual-layer Porymap data, as reported by LayerModeConverter::synthesized_layers(). Only consulted when
    /// @p mode is mixed. An empty vector means no metatile has a synthesized layer.
    /// @pre @p entries has a size divisible by 12
    /// @pre @p synthesized_layers is empty or has one element per metatile in @p entries
    /// @return The decompiled metatiles, one per 12 entries
    [[nodiscard]] ChainableResult<std::vector<Metatile<Rgba32>>> decompile_metatiles(
        const std::vector<TilemapEntry> &entries,
        const Image<IndexPixel> &tiles_png,
        const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes,
        ImportTransparencyMode mode = ImportTransparencyMode::extrinsic,
        const std::vector<std::optional<metatile::Layer>> &synthesized_layers = {});

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const Rgba32 extrinsic_transparency_;
};

} // namespace porytiles
