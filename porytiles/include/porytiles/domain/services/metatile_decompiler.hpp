#pragma once

#include <memory>
#include <vector>

#include "gsl/pointers"

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

    [[nodiscard]] ChainableResult<std::vector<Metatile<Rgba32>>> decompile_metatiles(
        const std::vector<TilemapEntry> &entries,
        const Image<IndexPixel> &tiles_png,
        const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes);

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const Rgba32 extrinsic_transparency_;
};

} // namespace porytiles
