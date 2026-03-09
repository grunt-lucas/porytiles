#include "porytiles2/domain/services/primary_tileset_decompiler.hpp"

#include <array>
#include <format>
#include <iostream>
#include <memory>
#include <ranges>
#include <set>
#include <vector>

#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/config/per_anim_overrides.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/anim_decompiler.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/domain/services/metatile_decompiler.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
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
        std::format("Failed to triple-layerize Porymap component for tileset '{}'.", tileset.name()),
        std::unique_ptr<Tileset>);

    /*
     * Create the new Porymap component early so we can pass it for potential backporting during animation
     * decompilation. If mangle strategy is used, duplicate key frame tiles will be modified and backported to
     * tiles.png.
     *
     * IMPORTANT: This must happen BEFORE metatile decompilation so that mangled tiles are used when decompiling
     * metatiles.
     */
    auto new_porymap_component = std::make_unique<PorymapTilesetComponent>(tileset.porymap_component());

    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    std::size_t i = 0;
    for (const auto &pal : tileset.porytiles_component().pals()) {
        // Copy over Porytiles pals
        if (pal.has_value()) {
            new_porytiles_component->set_pal(i, pal.value());
        }
        i++;
    }

    // Copy metatile attributes from Porymap component to Porytiles component
    const auto &porymap_attributes = tileset.porymap_component().metatile_attributes_bin();
    for (std::size_t metatile_id = 0; metatile_id < porymap_attributes.size(); metatile_id++) {
        new_porytiles_component->insert_attribute(metatile_id, porymap_attributes[metatile_id]);
    }

    /*
     * Decompile animations from Porymap component to Porytiles component.
     *
     * IMPORTANT: This must happen BEFORE metatile decompilation so that any mangled key frame tiles are present in
     * new_porymap_component->tiles_png() when metatiles are decompiled
     */
    // Triple-layerize the metatiles for the AnimDecompiler (it uses triple-layer addressing for override extraction)
    new_porymap_component->metatiles_bin(tilemap_entries);

    if (const auto &porymap_animations = tileset.porymap_component().anims(); !porymap_animations.empty()) {
        // Accumulate canonical forms of previously-processed animations' key frame tiles for inter-animation detection
        std::set<PixelTile<IndexPixel>> inter_anim_canonical_tiles;

        AnimDecompiler anim_decompiler{config_, diag_, tile_printer_, pal_printer_};

        for (const auto &index_pixel_anim : porymap_animations | std::views::values) {
            // Run animation decompilation
            PT_TRY_ASSIGN_CHAIN_ERR(
                rgba_anim,
                anim_decompiler.decompile_animation(
                    tileset.name(), index_pixel_anim, inter_anim_canonical_tiles, *new_porymap_component),
                diag_->formatter().format(
                    "Failed to decompile animation '{}'.", FormatParam{index_pixel_anim.name(), Style::bold}),
                std::unique_ptr<Tileset>);

            /*
             * After successful decompilation, extract canonical tiles from this animation's (potentially mangled) key
             * frame range in tiles.png and add them to the accumulator for inter-animation duplicate detection.
             */
            const std::size_t anim_tile_offset = index_pixel_anim.params().tile_offset();
            if (anim_tile_offset == 0) {
                /*
                 * AnimDecompiler::decompile_animation() validates that tile_offset != 0 before returning success, so
                 * this branch should be unreachable. Retained as a defensive invariant.
                 */
                panic("anim '" + index_pixel_anim.name() + "' offset is 0");
            }
            const std::size_t anim_tile_count = index_pixel_anim.params().tile_count();
            const auto anim_tiles =
                extract_tiles_from_image(new_porymap_component->tiles_png(), anim_tile_offset, anim_tile_count);
            for (const auto &t : anim_tiles) {
                const CanonicalPixelTile canonical{t};
                const PixelTile<IndexPixel> &base = canonical;
                inter_anim_canonical_tiles.insert(base);
            }

            new_porytiles_component->add_anim(std::move(rgba_anim));
        }

        // Warn about per anim overrides that reference non-existent animations (after all animations processed)
        PT_UNWRAP_TILESET_CONFIG_PTR(config_, per_anim_overrides, tileset.name(), std::unique_ptr<Tileset>);
        for (const auto &key : per_anim_overrides.value() | std::views::keys) {
            if (!porymap_animations.contains(key)) {
                diag_->warning(
                    "unused-animation-config",
                    {diag_->formatter().format(
                        "Animation config for '{}' does not match any animation in tileset '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{tileset.name(), Style::bold})});
            }
        }
    }

    // Decompile metatiles AFTER animation processing so that any mangled key frame tiles are used
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            tilemap_entries, new_porymap_component->tiles_png(), tileset.porymap_component().pals()),
        std::format("Failed to decompile Porymap component for tileset '{}'.", tileset.name()),
        std::unique_ptr<Tileset>);

    // Convert metatiles into three layer images
    LayerImageMetatileizer<Rgba32> metatileizer{};
    constexpr std::size_t metatiles_per_row = 8; // Standard width for layer images (128 pixels)

    PT_TRY_ASSIGN_CHAIN_ERR(
        layer_images,
        metatileizer.demetatileize(metatiles, metatiles_per_row),
        format_->format(
            "Failed to demetatileize metatiles for tileset '{}'.", FormatParam{tileset.name(), Style::bold}),
        std::unique_ptr<Tileset>);

    auto &[bottom_image, middle_image, top_image] = layer_images;

    // Set each porytiles_component layer to the new image
    new_porytiles_component->bottom(bottom_image);
    new_porytiles_component->middle(middle_image);
    new_porytiles_component->top(top_image);

    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(new_porytiles_component), std::move(new_porymap_component));

    return new_tileset;
}

} // namespace porytiles2
