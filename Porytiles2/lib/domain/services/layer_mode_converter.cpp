#include "porytiles2/domain/services/layer_mode_converter.hpp"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::triple_layerize(const PorymapTilesetComponent &component)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        layer_mode, component.detect_layer_mode(), "layer mode detection failed", std::vector<TilemapEntry>);
    if (layer_mode == tileset::LayerMode::triple) {
        // No-op case
        return component.metatiles_bin();
    }

    const auto &metatiles_bin = component.metatiles_bin();
    const auto &metatile_attributes = component.metatile_attributes_bin();

    const std::size_t num_metatiles = metatiles_bin.size() / metatile::entries_per_metatile_dual;

    std::vector<TilemapEntry> result;
    result.reserve(num_metatiles * metatile::entries_per_metatile_triple);

    // Create a transparent entry (tile_index = 0, pal_index = 0, no flips)
    const TilemapEntry transparent{0, 0, false, false};

    for (std::size_t i = 0; i < num_metatiles; ++i) {
        // Number of transparent entries to insert
        constexpr std::size_t transparent_entries = 4;
        const auto &attribute = metatile_attributes[i];
        const std::size_t input_offset = i * metatile::entries_per_metatile_dual;

        switch (attribute.layer_type()) {
        case attr::LayerType::normal:
            // Insert 4 transparent entries at the start
            for (std::size_t j = 0; j < transparent_entries; ++j) {
                result.push_back(transparent);
            }
            // Copy the 8 original entries
            for (std::size_t j = 0; j < metatile::entries_per_metatile_dual; ++j) {
                result.push_back(metatiles_bin[input_offset + j]);
            }
            break;

        case attr::LayerType::covered:
            // Copy the 8 original entries
            for (std::size_t j = 0; j < metatile::entries_per_metatile_dual; ++j) {
                result.push_back(metatiles_bin[input_offset + j]);
            }
            // Insert 4 transparent entries at the end
            for (std::size_t j = 0; j < transparent_entries; ++j) {
                result.push_back(transparent);
            }
            break;

        case attr::LayerType::split:
            // Copy the first 4 entries
            for (std::size_t j = 0; j < transparent_entries; ++j) {
                result.push_back(metatiles_bin[input_offset + j]);
            }
            // Insert 4 transparent entries in the middle
            for (std::size_t j = 0; j < transparent_entries; ++j) {
                result.push_back(transparent);
            }
            // Copy the last 4 entries
            for (std::size_t j = transparent_entries; j < metatile::entries_per_metatile_dual; ++j) {
                result.push_back(metatiles_bin[input_offset + j]);
            }
            break;
        }
    }

    return result;
}

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::dual_layerize(const PorymapTilesetComponent &component)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        layer_mode, component.detect_layer_mode(), "layer mode detection failed", std::vector<TilemapEntry>);
    if (layer_mode == tileset::LayerMode::dual) {
        // No-op case
        return component.metatiles_bin();
    }
    // Iterate over component tilemap entries and attributes.
    panic("TODO: implement");
}

} // namespace porytiles2
