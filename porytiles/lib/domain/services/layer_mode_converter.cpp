#include "porytiles/domain/services/layer_mode_converter.hpp"

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::triple_layerize(const PorymapTilesetComponent &component)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        layer_mode, component.detect_layer_mode(), std::vector<TilemapEntry>, "Layer mode detection failed.");
    if (layer_mode == LayerMode::triple) {
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
        case LayerType::normal:
            // Insert 4 transparent entries at the start
            for (std::size_t j = 0; j < transparent_entries; ++j) {
                result.push_back(transparent);
            }
            // Copy the 8 original entries
            for (std::size_t j = 0; j < metatile::entries_per_metatile_dual; ++j) {
                result.push_back(metatiles_bin[input_offset + j]);
            }
            break;

        case LayerType::covered:
            // Copy the 8 original entries
            for (std::size_t j = 0; j < metatile::entries_per_metatile_dual; ++j) {
                result.push_back(metatiles_bin[input_offset + j]);
            }
            // Insert 4 transparent entries at the end
            for (std::size_t j = 0; j < transparent_entries; ++j) {
                result.push_back(transparent);
            }
            break;

        case LayerType::split:
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

[[nodiscard]] std::vector<TilemapEntry> LayerModeConverter::dual_layerize(
    const std::vector<TilemapEntry> &entries, const std::vector<Metatile<Rgba32>> &source_metatiles)
{
    const std::size_t expected_entries_size = source_metatiles.size() * metatile::entries_per_metatile_triple;
    if (entries.size() != expected_entries_size) {
        panic(
            "entries vector size " + std::to_string(entries.size()) + " did not match expected size " +
            std::to_string(expected_entries_size));
    }

    std::vector<TilemapEntry> result;
    result.reserve(source_metatiles.size() * metatile::entries_per_metatile_dual);

    for (std::size_t i = 0; i < source_metatiles.size(); ++i) {
        const auto &metatile = source_metatiles[i];
        const std::size_t input_offset = i * metatile::entries_per_metatile_triple;

        // Check precondition: no metatile should have implied LayerMode::triple
        const LayerMode layer_mode = metatile.infer_layer_mode(extrinsic_transparency_);
        if (layer_mode == LayerMode::triple) {
            panic("metatile " + std::to_string(i) + " has implied LayerMode::triple, cannot dual_layerize");
        }

        // Infer the layer type for this metatile using extrinsic transparency
        const LayerType layer_type = metatile.infer_layer_type(extrinsic_transparency_);

        switch (layer_type) {
        case LayerType::normal:
            // Skip first 4 transparent entries, copy next 8
            for (std::size_t j = 4; j < metatile::entries_per_metatile_triple; ++j) {
                result.push_back(entries[input_offset + j]);
            }
            break;

        case LayerType::covered:
            // Copy first 8 entries, skip last 4 transparent
            for (std::size_t j = 0; j < metatile::entries_per_metatile_dual; ++j) {
                result.push_back(entries[input_offset + j]);
            }
            break;

        case LayerType::split:
            // Copy first 4, skip middle 4 transparent, copy last 4
            for (std::size_t j = 0; j < 4; ++j) {
                result.push_back(entries[input_offset + j]);
            }
            for (std::size_t j = 8; j < metatile::entries_per_metatile_triple; ++j) {
                result.push_back(entries[input_offset + j]);
            }
            break;
        }
    }

    return result;
}

} // namespace porytiles
