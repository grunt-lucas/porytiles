#include "porytiles2/domain/services/primary_tileset_decompiler.hpp"

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

ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetDecompiler::decompile(const Tileset &tileset) const
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_pals_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_pals_total, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_metatiles_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_per_metatile, tileset.name(), std::unique_ptr<Tileset>);

    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_, extrinsic_transparency};
    MetatileDecompiler metatile_decompiler{format_, diag_, tile_printer_, extrinsic_transparency};

    // Decompile Porymap tilemap entries
    PT_TRY_ASSIGN_CHAIN_ERR(
        tilemap_entries,
        layer_mode_converter.triple_layerize(tileset.porymap_component()),
        "failed to triple-layerize Porymap component for tileset " + tileset.name(),
        std::unique_ptr<Tileset>);

    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            tilemap_entries, tileset.porymap_component().tiles_png(), tileset.porymap_component().pals()),
        "failed to decompile Porymap component for tileset " + tileset.name(),
        std::unique_ptr<Tileset>);

    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    std::size_t i = 0;
    for (const auto &pal : tileset.porytiles_component().pals()) {
        // Copy over Porytiles pals
        if (pal.has_value()) {
            new_porytiles_component->set_pal(i, pal.value());
        }
        i++;
    }

    // Decompile animations from Porymap component to Porytiles component
    if (const auto &porymap_animations = tileset.porymap_component().anims(); !porymap_animations.empty()) {
        const auto &metatiles_bin = tileset.porymap_component().metatiles_bin();

        for (const auto &index_pixel_anim : porymap_animations | std::views::values) {
            AnimationDecompiler anim_decompiler{};
            // Decompile the IndexPixel animation to Rgba32 format
            // Palette index is recovered from metatile data by scanning for animation tile references
            Animation<Rgba32> rgba_anim = anim_decompiler.decompile_animation(
                index_pixel_anim,
                tileset.porymap_component().pals(),
                metatiles_bin,
                tileset.porymap_component().tiles_png(),
                extrinsic_transparency);

            new_porytiles_component->add_anim(std::move(rgba_anim));
        }
    }

    // Convert metatiles into three layer images
    LayerImageMetatileizer<Rgba32> metatileizer{};
    constexpr std::size_t metatiles_per_row = 8; // Standard width for layer images (128 pixels)

    PT_TRY_ASSIGN_CHAIN_ERR(
        layer_images,
        metatileizer.demetatileize(metatiles, metatiles_per_row),
        "failed to demetatileize metatiles for tileset " + tileset.name(),
        std::unique_ptr<Tileset>);

    auto &[bottom_image, middle_image, top_image] = layer_images;

    // Set each porytiles_component layer to the new image
    new_porytiles_component->bottom(bottom_image);
    new_porytiles_component->middle(middle_image);
    new_porytiles_component->top(top_image);

    // No changes here, this is an import operation - no writebacks into input assets
    auto new_porymap_component = std::make_unique<PorymapTilesetComponent>(tileset.porymap_component());

    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(new_porytiles_component), std::move(new_porymap_component));

    return new_tileset;
}

} // namespace porytiles2
