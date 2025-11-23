#pragma once

#include <memory>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

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
        const std::array<Palette<Rgba32>, pal::num_pals> &pals);

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    Rgba32 extrinsic_transparency_;
};

} // namespace porytiles2
