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
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

class MetatileDecompiler {
  public:
    explicit MetatileDecompiler(
        gsl::not_null<TextFormatter *> format,
        gsl::not_null<UserDiagnostics *> diag,
        gsl::not_null<TilePrinter *> tile_printer)
        : format_{format}, diag_{diag}, tile_printer_{tile_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::vector<Metatile<Rgba32>>> decompile_metatiles(
        const std::vector<TilemapEntry> &entries,
        const Image<IndexPixel> &tiles_png,
        const std::array<Palette<Rgba32>, pal::num_pals> &pals);

  private:
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
};

} // namespace porytiles2
