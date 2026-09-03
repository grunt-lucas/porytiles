#include "porytiles/domain/services/primary_tileset_decompiler.hpp"

#include <array>
#include <format>
#include <iostream>
#include <memory>
#include <ranges>
#include <set>
#include <vector>

#include "porytiles/domain/algorithms/role_pin_round_trip.hpp"
#include "porytiles/domain/config/import_transparency_mode.hpp"
#include "porytiles/domain/config/per_anim_overrides.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/anim_decompiler.hpp"
#include "porytiles/domain/services/layer_image_metatileizer.hpp"
#include "porytiles/domain/services/layer_mode_converter.hpp"
#include "porytiles/domain/services/metatile_decompiler.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/config_value.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace porytiles {
ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetDecompiler::decompile(const Tileset &tileset) const
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_palettes_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_palettes_total, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_metatiles_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_in_primary, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_per_metatile, tileset.name(), std::unique_ptr<Tileset>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, import_transparency, tileset.name(), std::unique_ptr<Tileset>);

    LayerModeConverter layer_mode_converter{format_, diag_, tile_printer_, extrinsic_transparency};
    MetatileDecompiler metatile_decompiler{format_, diag_, tile_printer_, extrinsic_transparency};

    // Decompile Porymap tilemap entries
    PT_TRY_ASSIGN_CHAIN_ERR(
        tilemap_entries,
        layer_mode_converter.triple_layerize(tileset.porymap_component()),
        std::unique_ptr<Tileset>,
        std::format("Failed to triple-layerize Porymap component for tileset '{}'.", tileset.name()));

    // The layer group triple-layerization synthesized for each metatile (none in triple-layer mode).
    PT_TRY_ASSIGN_CHAIN_ERR(
        synthesized_layers,
        layer_mode_converter.synthesized_layers(tileset.porymap_component()),
        std::unique_ptr<Tileset>,
        std::format("Failed to determine synthesized layers for tileset '{}'.", tileset.name()));

    // Create the new Porymap component early so we can pass it for potential backporting during animation
    // decompilation. If mangle strategy is used, duplicate key frame tiles will be modified and backported to
    // tiles.png.
    //
    // IMPORTANT: This must happen BEFORE metatile decompilation so that mangled tiles are used when decompiling
    // metatiles.
    auto new_porymap_component = std::make_unique<PorymapTilesetComponent>(tileset.porymap_component());

    auto new_porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    std::size_t i = 0;
    for (const auto &palette : tileset.porytiles_component().palettes()) {
        // Copy over Porytiles palettes
        if (palette.has_value()) {
            new_porytiles_component->set_palette(i, palette.value());
        }
        i++;
    }

    // Copy metatile attributes from Porymap component to Porytiles component, applying the layer_type pin round-trip.
    // Each bin-decoded attribute is unpinned. Whether it gets pinned (and thus survives as an explicit CSV value) is
    // decided from the prior attributes.csv state recorded on the loaded component. On a fresh import (no
    // CSV) the state defaults to no_csv, so every row is pinned from the bin.
    const PriorPinColumnState prior_pin_state =
        tileset.porytiles_component().prior_pin_column_state(FieldRole::layer_type);
    const auto &porymap_attributes = tileset.porymap_component().metatile_attributes_bin();
    for (std::size_t metatile_id = 0; metatile_id < porymap_attributes.size(); metatile_id++) {
        new_porytiles_component->insert_attribute(
            metatile_id,
            merge_prior_layer_type_pin(
                porymap_attributes[metatile_id],
                prior_pin_state,
                tileset.porytiles_component().get_attribute(metatile_id)));
    }

    // Decompile animations from Porymap component to Porytiles component.
    //
    // IMPORTANT: This must happen BEFORE metatile decompilation so that any mangled key frame tiles are present in
    // new_porymap_component->tiles_png() when metatiles are decompiled
    // Triple-layerize the metatiles for the AnimDecompiler (it uses triple-layer addressing for override extraction)
    new_porymap_component->metatiles_bin(tilemap_entries);

    if (const auto &porymap_animations = tileset.porymap_component().anims(); !porymap_animations.empty()) {
        // Save canonical resolved RGBA forms of previously-processed animation key frame tiles for inter-animation
        // duplicate detection
        std::set<PixelTile<Rgba32>> inter_anim_canonical_tiles;

        AnimDecompiler anim_decompiler{config_, diag_, tile_printer_, palette_printer_};

        for (const auto &index_pixel_anim : porymap_animations | std::views::values) {
            // Run animation decompilation
            PT_TRY_ASSIGN_CHAIN_ERR(
                rgba_anim,
                anim_decompiler.decompile_animation(
                    tileset.name(), index_pixel_anim, inter_anim_canonical_tiles, *new_porymap_component),
                std::unique_ptr<Tileset>,
                diag_->formatter().format(
                    "Failed to decompile animation '{}'.", FormatParam{index_pixel_anim.name(), Style::bold}));

            // After successful decompilation, record this animation's key frame tiles (post-mangle: the decompiler
            // resolves them after any mangling) for inter-animation duplicate detection in later animations.
            // Manual-linked animations aren't handled here, they have no key frames. Their tiles.png range is
            // still covered by the cross-range duplicate check inside decompile_animation.
            if (rgba_anim.has_key_frame()) {
                for (const auto &t : rgba_anim.key_frame().tiles()) {
                    const CanonicalPixelTile<Rgba32> canonical{t};
                    const PixelTile<Rgba32> &base = canonical;
                    inter_anim_canonical_tiles.insert(base);
                }
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

    // Restore original metatiles_bin (line 81 set triple-layer format for AnimDecompiler, but output must match input)
    new_porymap_component->metatiles_bin(tileset.porymap_component().metatiles_bin());

    // Decompile metatiles AFTER animation processing so that any mangled key frame tiles are used
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatile_decompiler.decompile_metatiles(
            tilemap_entries,
            new_porymap_component->tiles_png(),
            tileset.porymap_component().palettes(),
            import_transparency.value(),
            synthesized_layers),
        std::unique_ptr<Tileset>,
        std::format("Failed to decompile Porymap component for tileset '{}'.", tileset.name()));

    // Convert metatiles into three layer images
    LayerImageMetatileizer<Rgba32> metatileizer{};
    constexpr std::size_t metatiles_per_row = 8; // Standard width for layer images (128 pixels)

    PT_TRY_ASSIGN_CHAIN_ERR(
        layer_images,
        metatileizer.demetatileize(metatiles, metatiles_per_row),
        std::unique_ptr<Tileset>,
        format_->format(
            "Failed to demetatileize metatiles for tileset '{}'.", FormatParam{tileset.name(), Style::bold}));

    auto &[bottom_image, middle_image, top_image] = layer_images;

    // Set each porytiles_component layer to the new image
    new_porytiles_component->bottom(bottom_image);
    new_porytiles_component->middle(middle_image);
    new_porytiles_component->top(top_image);

    auto new_tileset =
        std::make_unique<Tileset>(tileset.name(), std::move(new_porytiles_component), std::move(new_porymap_component));

    return new_tileset;
}

} // namespace porytiles
