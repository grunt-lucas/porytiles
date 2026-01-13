#include "porytiles2/domain/services/primary_tileset_importer.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <ranges>
#include <unordered_set>
#include <vector>

#include "porytiles2/domain/algorithms/palette_matchers.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/animation_decompiler.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/metatile_decompiler.hpp"
#include "porytiles2/domain/services/metatile_validator.hpp"
#include "porytiles2/utilities/functional/transform.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_validators.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetImporter::import(const std::string &tileset_name) const
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_name, std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_pals_in_primary, tileset_name, std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_pals_total, tileset_name, std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_metatiles_in_primary, tileset_name, std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_in_primary, tileset_name, std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_per_metatile, tileset_name, std::unique_ptr<Tileset>);

    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_, extrinsic_transparency};
    MetatileDecompiler metatile_decompiler{format_, diag_, tile_printer_, extrinsic_transparency};

    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_component,
        import_porymap_component_from_vanilla(tileset_name),
        format_->format("failed to import '{}' assets from backing store", FormatParam{tileset_name, Style::bold}),
        std::unique_ptr<Tileset>);

    // Decompile Porymap tilemap entries
    PT_TRY_ASSIGN_CHAIN_ERR(
        tilemap_entries,
        layer_mode_converter.triple_layerize(*porymap_component),
        format_->format(
            "failed to triple-layerize Porymap component for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::unique_ptr<Tileset>);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            tilemap_entries, porymap_component->tiles_png(), porymap_component->pals()),
        format_->format(
            "failed to decompile Porymap component metatiles for tileset '{}'", FormatParam{tileset_name, Style::bold}),
        std::unique_ptr<Tileset>);

    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>();

    // TODO: fill in new_porytiles_component with decompiled assets.

    auto new_tileset =
        std::make_unique<Tileset>(tileset_name, std::move(new_porytiles_component), std::move(porymap_component));

    return new_tileset;
}

} // namespace porytiles2
