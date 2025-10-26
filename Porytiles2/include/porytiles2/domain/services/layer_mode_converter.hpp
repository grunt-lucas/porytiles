#pragma once

#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
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

    /**
     * @brief TODO
     *
     * @details
     * TODO
     *
     * @param component TODO
     * @return A triple-layerized TilemapEntry vector
     */
    [[nodiscard]] ChainableResult<std::vector<TilemapEntry>> triple_layerize(const PorymapTilesetComponent &component);

    /**
     * @brief TODO
     *
     * @details
     * TODO
     *
     * @pre asd
     *
     * @param component TODO
     * @return A dual-layerized TilemapEntry vector
     */
    [[nodiscard]] ChainableResult<std::vector<TilemapEntry>> dual_layerize(const PorymapTilesetComponent &component);

  private:
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
};

} // namespace porytiles2
