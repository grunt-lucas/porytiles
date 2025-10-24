#pragma once

#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

class LayerModeConverter {
  public:
    explicit LayerModeConverter(
        gsl::not_null<TextFormatter *> format,
        gsl::not_null<UserDiagnostics *> diag,
        gsl::not_null<TilePrinter *> tile_printer)
        : format_{format}, diag_{diag}, tile_printer_{tile_printer}
    {
    }

    [[nodiscard]] ChainableResult<std::vector<TilemapEntry>>
    triple_layerize(const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes);

    [[nodiscard]] ChainableResult<std::vector<TilemapEntry>>
    dual_layerize(const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes);

  private:
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
};

} // namespace porytiles2
