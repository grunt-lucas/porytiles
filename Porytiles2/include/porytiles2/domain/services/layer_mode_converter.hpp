#pragma once

#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

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
     * @brief Converts a tileset component to triple-layer format.
     *
     * @details
     * Converts dual-layer metatiles (8 entries per metatile) to triple-layer metatiles (12 entries per metatile) by
     * inserting transparent tilemap entries based on each metatile's LayerType attribute. If the component is already
     * in triple-layer format, it is returned unchanged.
     *
     * The conversion strategy depends on the metatile's LayerType:
     * - normal: Inserts 4 transparent entries at the beginning, followed by the 8 original entries
     * - covered: Copies the 8 original entries first, then appends 4 transparent entries at the end
     * - split: Copies the first 4 entries, inserts 4 transparent entries in the middle, then copies the last 4 entries
     *
     * @param component The tileset component to convert
     * @return A triple-layerized TilemapEntry vector
     */
    [[nodiscard]] ChainableResult<std::vector<TilemapEntry>> triple_layerize(const PorymapTilesetComponent &component);

    /**
     * @brief Converts a tileset from triple-layer format to dual-layer format.
     *
     * @details
     * Converts triple-layer metatiles (12 entries per metatile) to dual-layer metatiles (8 entries per metatile) by
     * removing transparent tilemap entries based on each metatile's inferred LayerType. This is the inverse operation
     * of triple_layerize().
     *
     * The conversion strategy depends on the metatile's inferred LayerType:
     * - normal: Removes the first 4 transparent entries, keeps the last 8 entries
     * - covered: Keeps the first 8 entries, removes the last 4 transparent entries
     * - split: Keeps the first 4 entries, removes the middle 4 transparent entries, keeps the last 4 entries
     *
     * @param entries The triple-layer tilemap entries to convert
     * @param source_metatiles The source metatiles used to infer layer types
     * @pre The entries vector contains triple-layer entries (size must equal source_metatiles.size() * 12)
     * @pre No metatile in source_metatiles has implied LayerMode::triple
     * @return A dual-layerized TilemapEntry vector
     */
    [[nodiscard]] std::vector<TilemapEntry>
    dual_layerize(const std::vector<TilemapEntry> &entries, const std::vector<Metatile<Rgba32>> &source_metatiles);

  private:
    TextFormatter *format_;
    UserDiagnostics *diag_;
    TilePrinter *tile_printer_;
};

} // namespace porytiles2
