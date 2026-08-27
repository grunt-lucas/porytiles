#pragma once

#include <optional>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/models/layer.hpp"

#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

class LayerModeConverter {
  public:
    explicit LayerModeConverter(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<const TilePrinter *> tile_printer,
        const Rgba32 &extrinsic_transparency)
        : format_{format}, diag_{diag}, tile_printer_{tile_printer}, extrinsic_transparency_{extrinsic_transparency}
    {
    }

    /// @brief Converts a tileset component to triple-layer format.
    ///
    /// @details
    /// Converts dual-layer metatiles (8 entries per metatile) to triple-layer metatiles (12 entries per metatile) by
    /// inserting transparent tilemap entries based on each metatile's LayerType attribute. If the component is already
    /// in triple-layer format, it is returned unchanged.
    ///
    /// The conversion strategy depends on the metatile's LayerType:
    /// - normal: Inserts 4 transparent entries at the beginning, followed by the 8 original entries
    /// - covered: Copies the 8 original entries first, then appends 4 transparent entries at the end
    /// - split: Copies the first 4 entries, inserts 4 transparent entries in the middle, then copies the last 4 entries
    ///
    /// @param component The tileset component to convert
    /// @return A triple-layerized TilemapEntry vector
    [[nodiscard]] ChainableResult<std::vector<TilemapEntry>> triple_layerize(const PorymapTilesetComponent &component);

    /// @brief Converts a tileset from triple-layer format to dual-layer format.
    ///
    /// @details
    /// Converts triple-layer metatiles (12 entries per metatile) to dual-layer metatiles (8 entries per metatile) by
    /// dropping one 4-entry layer group per metatile, selected by the metatile's effective LayerType. When every
    /// dropped group is transparent (the case for entries produced by triple_layerize() from dual-compatible content),
    /// this is the exact inverse of triple_layerize().
    ///
    /// The conversion strategy depends on the metatile's effective LayerType:
    /// - normal: Removes the first 4 entries, keeps the last 8 entries
    /// - covered: Keeps the first 8 entries, removes the last 4 entries
    /// - split: Keeps the first 4 entries, removes the middle 4 entries, keeps the last 4 entries
    ///
    /// "Effective LayerType" is resolved per metatile via the following cascade:
    /// 1. An explicit `explicit_layer_types` value (a fieldmap.role_pins layer_type pin) wins if present.
    /// 2. Otherwise infer_layer_type() classifies dual-representable content as normal, covered, or split by which two
    ///    layers hold content.
    /// 3. For genuinely triple content (all three layers visible) there is nothing meaningful to infer, so
    ///    infer_layer_type() falls back to normal. This is a fallback, not a classification, and it drops the bottom
    ///    group.
    ///
    /// If the dropped group contains an entry that references a visible (non-transparent) tile, a warning is emitted
    /// (tag `dual-layer-drop`). The check inspects the actual tilemap entries at reduction time (after manual
    /// animation overrides), not the source RGBA transparency, so it catches entries made visible by post-inference
    /// overrides.
    ///
    /// A metatile with implied triple-layer content (all three layers visible at some subtile) is legal input:
    /// validate_layer_mode gates such content behind the ignore_triple_layer_content option. When it reaches this
    /// function it has no inferable layer type (tier 3), so without a pin the fallback to normal drops its bottom
    /// group; the `dual-layer-drop` warning carries a role-pins hint so the user can pin a different group to keep
    /// instead.
    ///
    /// @param entries The triple-layer tilemap entries to convert
    /// @param source_metatiles The source metatiles used to infer layer types
    /// @param explicit_layer_types Per-metatile explicit layer-type overrides; an empty vector (or a nullopt element)
    /// means "infer this metatile's layer type".
    /// @pre The entries vector contains triple-layer entries (size must equal source_metatiles.size() * 12)
    /// @return A dual-layerized TilemapEntry vector
    [[nodiscard]] std::vector<TilemapEntry> dual_layerize(
        const std::vector<TilemapEntry> &entries,
        const std::vector<Metatile<Rgba32>> &source_metatiles,
        const std::vector<std::optional<LayerType>> &explicit_layer_types = {});

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    const TilePrinter *tile_printer_;
    const Rgba32 extrinsic_transparency_;
};

} // namespace porytiles
