#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/rgba_metatile.hpp"
#include "porytiles2/domain/models/rgba_pal.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

class MetatileDecompiler {
  public:
    explicit MetatileDecompiler(
        std::string tileset_name,
        gsl::not_null<DomainConfig *> config,
        gsl::not_null<TextFormatter *> format,
        gsl::not_null<UserDiagnostics *> diag,
        gsl::not_null<TilePrinter *> tile_printer)
        : tileset_name_{std::move(tileset_name)}, config_{config}, format_{format}, diag_{diag},
          tile_printer_{tile_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::vector<RgbaMetatile>> decompile_metatiles(
        const std::vector<TilemapEntry> &entries,
        const std::vector<MetatileAttribute> &attributes,
        const Image<IndexPixel> &tiles,
        const std::array<RgbaPal, pal::num_pals> &pals);

  private:
    std::string tileset_name_;
    DomainConfig *config_;
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
};

} // namespace porytiles2
