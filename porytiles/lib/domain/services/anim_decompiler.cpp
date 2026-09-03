#include "porytiles/domain/services/anim_decompiler.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/algorithms/tile_extractors.hpp"
#include "porytiles/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_multi_palette_subtile_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_palette_resolution_strategy.hpp"
#include "porytiles/domain/config/frame_linking.hpp"
#include "porytiles/domain/config/import_transparency_mode.hpp"
#include "porytiles/domain/config/per_anim_overrides.hpp"
#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/anim_override_entry.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/packing/models/palette_hint.hpp"
#include "porytiles/domain/services/anim_key_frame_mangler.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/result/error.hpp"
#include "porytiles/xcut/config/config_value.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles;

[[nodiscard]] ChainableResult<std::size_t> internal_png_palette_strategy(
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &tileset_palettes,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &palette_printer)
{
    if (!anim.has_frames()) {
        panic("anim '" + anim.name() + "' has no frames");
    }

    const auto &representative_frame = anim.frames().begin()->second;
    const auto &representative_palette = representative_frame.palette();

    // Representative palette must have exactly 16 colors to match GBA palette format
    if (representative_palette.size() != palette::max_size) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag.formatter().format(
            "Representative frame '{}' internal palette size '{}': must be '{}'.",
            FormatParam{representative_frame.frame_name(), Style::bold},
            FormatParam{representative_palette.size(), Style::bold},
            FormatParam{palette::max_size, Style::bold}));
        err_msg.emplace_back("");
        err_msg.append_range(palette_printer.print_rgba_palette(representative_palette));
        return FormattableError{err_msg};
    }

    // Check for extrinsic transparency in non-slot-0 positions in representative palette
    std::vector<std::size_t> extrinsic_transparency_slots;
    for (std::size_t slot = 1; slot < palette::max_size; ++slot) {
        const Rgba32 &color = representative_palette.at(slot);
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            extrinsic_transparency_slots.push_back(slot);
        }
    }

    if (!extrinsic_transparency_slots.empty()) {
        std::string slot_list;
        for (const auto &slot : extrinsic_transparency_slots) {
            if (!slot_list.empty()) {
                slot_list += ", ";
            }
            slot_list += std::to_string(slot);
        }
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag.formatter().format(
            "Representative frame '{}' palette contains extrinsic transparency color '{}' in non-zero slot(s): {}",
            FormatParam{representative_frame.frame_name(), Style::bold},
            FormatParam{extrinsic_transparency.value().to_jasc_str(), Style::bold},
            FormatParam{slot_list, Style::bold}));
        err_msg.emplace_back("");
        err_msg.emplace_back("The extrinsic transparency color should only appear in slot 0.");
        err_msg.emplace_back("Either correct the PNG palette or change the extrinsic transparency setting.");
        err_msg.emplace_back("");
        err_msg.append_range(
            palette_printer.print_rgba_palette_with_highlights(representative_palette, extrinsic_transparency_slots));
        err_msg.append_range(format_config_note_with_separator(diag.formatter(), extrinsic_transparency));
        return FormattableError{err_msg};
    }

    // Check all frames share the same internal palette as the representative
    for (const auto &[frame_name, frame] : anim.frames()) {
        if (&frame == &representative_frame) {
            continue;
        }
        const auto &frame_palette = frame.palette();
        bool palettes_match = (frame_palette.size() == representative_palette.size());
        if (palettes_match) {
            for (std::size_t slot = 0; slot < representative_palette.size(); ++slot) {
                if (frame_palette.at(slot) != representative_palette.at(slot)) {
                    palettes_match = false;
                    break;
                }
            }
        }
        if (!palettes_match) {
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag.formatter().format(
                "Animation '{}' frame '{}' has an internal palette that does not match the representative frame '{}' "
                "palette.",
                FormatParam{anim.name(), Style::bold},
                FormatParam{frame_name, Style::bold},
                FormatParam{representative_frame.frame_name(), Style::bold}));
            err_msg.emplace_back("");
            err_msg.emplace_back(diag.formatter().format(
                "Representative frame '{}' palette:", FormatParam{representative_frame.frame_name(), Style::bold}));
            err_msg.append_range(palette_printer.print_rgba_palette(representative_palette));
            err_msg.emplace_back("");
            err_msg.emplace_back(diag.formatter().format("Frame '{}' palette:", FormatParam{frame_name, Style::bold}));
            err_msg.append_range(palette_printer.print_rgba_palette(frame_palette));
            return FormattableError{err_msg};
        }
    }

    // Now that we fully validated the representative palette, and we confirmed that all frame palettes match, we can
    // try to match the representative palette to one of the tileset palettes.
    for (std::size_t palette_idx = 0; palette_idx < tileset_palettes.size(); ++palette_idx) {
        bool matches = true;
        for (std::size_t slot = 1; slot < palette::max_size; ++slot) {
            const Rgba32 &png_palette_color = representative_palette.at(slot);

            // This should never happen, we returned early above if we hit this
            if (png_palette_color.is_transparent(extrinsic_transparency)) {
                panic("png_palette slot " + std::to_string(slot) + " is extrinsically transparent");
            }

            const Rgba32 &tileset_color = tileset_palettes[palette_idx].at(slot);
            if (png_palette_color != tileset_color) {
                matches = false;
                break;
            }
        }

        if (matches) {
            // Emit remark showing matched palette
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(diag.formatter().format(
                "Animation '{}' representative frame '{}' internal palette matched Porymap palette '{}':",
                FormatParam{anim.name(), Style::bold},
                FormatParam{representative_frame.frame_name(), Style::bold},
                FormatParam{palette_filename(palette_idx), Style::bold}));
            remark_lines.emplace_back("");
            remark_lines.append_range(palette_printer.print_rgba_palette(tileset_palettes[palette_idx]));
            diag.remark("animation-palette-resolution-strategy", remark_lines);
            return palette_idx;
        }
    }

    std::vector<std::string> err_msg{};
    err_msg.emplace_back(diag.formatter().format(
        "Failed to find matching palette for internal palette of representative frame '{}'.",
        FormatParam{representative_frame.frame_name(), Style::bold}));
    err_msg.emplace_back("");
    err_msg.append_range(palette_printer.print_rgba_palette(representative_palette));
    return FormattableError{err_msg};
}

[[nodiscard]] std::optional<std::size_t> extract_palette_index(AnimPaletteResolutionStrategy strategy)
{
    switch (strategy) {
    case AnimPaletteResolutionStrategy::palette_00:
        return 0;
    case AnimPaletteResolutionStrategy::palette_01:
        return 1;
    case AnimPaletteResolutionStrategy::palette_02:
        return 2;
    case AnimPaletteResolutionStrategy::palette_03:
        return 3;
    case AnimPaletteResolutionStrategy::palette_04:
        return 4;
    case AnimPaletteResolutionStrategy::palette_05:
        return 5;
    case AnimPaletteResolutionStrategy::palette_06:
        return 6;
    case AnimPaletteResolutionStrategy::palette_07:
        return 7;
    case AnimPaletteResolutionStrategy::palette_08:
        return 8;
    case AnimPaletteResolutionStrategy::palette_09:
        return 9;
    case AnimPaletteResolutionStrategy::palette_10:
        return 10;
    case AnimPaletteResolutionStrategy::palette_11:
        return 11;
    case AnimPaletteResolutionStrategy::palette_12:
        return 12;
    case AnimPaletteResolutionStrategy::palette_13:
        return 13;
    case AnimPaletteResolutionStrategy::palette_14:
        return 14;
    case AnimPaletteResolutionStrategy::palette_15:
        return 15;
    default:
        return std::nullopt;
    }
}

/// @brief Resolves the palette index for a single subtile using the given strategy.
///
/// @details
/// Dispatches to the appropriate resolution logic based on the strategy: explicit palette index, scan local metatiles,
/// internal PNG palette matching, or scan all tilesets.
///
/// @param anim_name The animation name (for diagnostics)
/// @param subtile_index The subtile index within the animation (0-based)
/// @param tile_index The absolute tile index in tiles.png
/// @param metatiles_bin The metatile entries to scan
/// @param strategy The per-subtile strategy config value
/// @param anim The animation (needed for internal_png_palette strategy)
/// @param palettes The tileset palettes
/// @param tiles_png The tiles.png image
/// @param extrinsic_transparency The extrinsic transparency color
/// @param diag User diagnostics for reporting
/// @param palette_printer Palette printer for diagnostic output
/// @param tile_printer Tile printer for diagnostic output
/// @param internal_png_palette_cache Cached result from internal_png_palette_strategy (populated on first use)
/// @return The resolved palette index for this subtile
[[nodiscard]] ChainableResult<std::size_t> resolve_subtile_palette(
    const std::string &anim_name,
    std::size_t subtile_index,
    std::size_t tile_index,
    std::span<const TilemapEntry> metatiles_bin,
    const ConfigValue<AnimPaletteResolutionStrategy> &strategy,
    const ConfigValue<AnimMultiPaletteSubtileResolutionStrategy> &multi_palette_strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes,
    const Image<IndexPixel> &tiles_png,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &palette_printer,
    const TilePrinter &tile_printer,
    std::optional<std::size_t> &internal_png_palette_cache)
{
    // Check the palette_00..palette_15 strategies first: direct palette index assignment
    const auto explicit_palette = extract_palette_index(strategy.value());
    if (explicit_palette.has_value()) {
        diag.remark(
            "animation-palette-resolution-strategy",
            {diag.formatter().format(
                "Animation '{}' subtile {} using explicit palette '{}'.",
                FormatParam{anim_name, Style::bold},
                FormatParam{subtile_index, Style::bold},
                FormatParam{palette_filename(*explicit_palette), Style::bold})});
        diag.remark_note("animation-palette-resolution-strategy", format_config_note(diag.formatter(), strategy));
        return *explicit_palette;
    }

    switch (strategy.value()) {
    case AnimPaletteResolutionStrategy::scan_local_metatiles: {
        std::set<std::size_t> found_for_subtile{};

        for (const auto &entry : metatiles_bin) {
            if (entry.tile_index() == tile_index) {
                found_for_subtile.insert(entry.palette_index());
            }
        }

        if (found_for_subtile.empty()) {
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag.formatter().format(
                "Animation '{}' subtile {} at tile index '{}' is not referenced in local metatiles.",
                FormatParam{anim_name, Style::bold},
                FormatParam{subtile_index, Style::bold},
                FormatParam{tile_index, Style::bold}));
            err_msg.emplace_back(
                "Consider using a different palette resolution strategy (e.g. 'palette-00', "
                "'internal-png-palette', etc.).");
            err_msg.append_range(format_config_note_with_separator(diag.formatter(), strategy));
            return FormattableError{err_msg};
        }

        if (found_for_subtile.size() > 1) {
            // A single tile index can be referenced by multiple metatile entries with different palette indices.
            // This is valid GBA behavior. The hardware selects palette per metatile entry, not per tile.
            //
            // The multi_palette_strategy config determines how to handle this case.
            std::string palette_list;
            for (const auto &palette_idx : found_for_subtile) {
                if (!palette_list.empty()) {
                    palette_list += ", ";
                }
                palette_list += palette_filename(palette_idx);
            }

            switch (multi_palette_strategy.value()) {
            case AnimMultiPaletteSubtileResolutionStrategy::error: {
                std::vector<std::string> err_msg;
                err_msg.push_back(diag.formatter().format(
                    "Animation '{}' subtile {} at tile index '{}' is referenced with multiple palettes: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{subtile_index, Style::bold},
                    FormatParam{tile_index, Style::bold},
                    FormatParam{palette_list, Style::bold}));
                err_msg.emplace_back(
                    "Picking one palette arbitrarily would produce incorrect RGBA output in the layer PNGs.");

                const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_png, tile_index);
                err_msg.emplace_back("");
                for (const auto &palette_idx : found_for_subtile) {
                    const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                        index_tile, palettes.at(palette_idx), extrinsic_transparency.value());
                    err_msg.push_back(diag.formatter().format(
                        "Tile under palette '{}':", FormatParam{palette_filename(palette_idx), Style::bold}));
                    err_msg.append_range(tile_printer.print_tile(rgba_tile, extrinsic_transparency.value()));
                }

                err_msg.emplace_back("");
                err_msg.emplace_back(
                    "Consider using an explicit palette resolution strategy (e.g. 'palette-00') to resolve the "
                    "ambiguity.");
                err_msg.append_range(format_config_note_with_separator(diag.formatter(), strategy));
                return FormattableError{err_msg};
            }
            case AnimMultiPaletteSubtileResolutionStrategy::warning: {
                const std::size_t chosen_palette = *found_for_subtile.begin();

                std::vector<std::string> warn_msg;
                warn_msg.push_back(diag.formatter().format(
                    "Animation '{}' subtile {} at tile index '{}' is referenced with multiple palettes: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{subtile_index, Style::bold},
                    FormatParam{tile_index, Style::bold},
                    FormatParam{palette_list, Style::bold}));
                warn_msg.push_back(diag.formatter().format(
                    "Using palette '{}'. Set 'frame_linking: manual' to handle palette assignment via overrides.",
                    FormatParam{palette_filename(chosen_palette), Style::bold}));

                const PixelTile<IndexPixel> warn_index_tile = extract_single_tile(tiles_png, tile_index);
                warn_msg.emplace_back("");
                for (const auto &palette_idx : found_for_subtile) {
                    const PixelTile<Rgba32> rgba_tile = color_tile_from_index_tile(
                        warn_index_tile, palettes.at(palette_idx), extrinsic_transparency.value());
                    warn_msg.push_back(diag.formatter().format(
                        "Tile under palette '{}':", FormatParam{palette_filename(palette_idx), Style::bold}));
                    warn_msg.append_range(tile_printer.print_tile(rgba_tile, extrinsic_transparency.value()));
                }

                warn_msg.append_range(format_config_note_with_separator(diag.formatter(), multi_palette_strategy));
                diag.warning("animation-multi-pal-subtile", warn_msg);

                return chosen_palette;
            }
            case AnimMultiPaletteSubtileResolutionStrategy::split:
                return FormattableError{
                    "The 'split' mode for multi-palette subtile resolution is not yet implemented."};
            }
        }

        return *found_for_subtile.begin();
    }

    case AnimPaletteResolutionStrategy::internal_png_palette: {
        if (internal_png_palette_cache.has_value()) {
            return *internal_png_palette_cache;
        }
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag.formatter().format(
            "Palette resolution strategy '{}' failed.",
            FormatParam{to_string(AnimPaletteResolutionStrategy::internal_png_palette), Style::bold}));
        err_msg.append_range(format_config_note_with_separator(diag.formatter(), strategy));
        PT_TRY_ASSIGN_CHAIN_ERR(
            match,
            internal_png_palette_strategy(anim, palettes, extrinsic_transparency, diag, palette_printer),
            std::size_t,
            err_msg);
        internal_png_palette_cache = match;
        return match;
    }

    case AnimPaletteResolutionStrategy::scan_all_tilesets:
        panic("scan_all_tilesets not yet implemented");

    default:
        panic("unhandled AnimPaletteResolutionStrategy value");
    }
}

[[nodiscard]] ChainableResult<std::vector<std::size_t>> find_palettes_for_anim_tiles(
    const std::string &anim_name,
    std::size_t tile_offset,
    std::size_t tile_count,
    std::span<const TilemapEntry> metatiles_bin,
    const std::vector<ConfigValue<AnimPaletteResolutionStrategy>> &per_subtile_strategies,
    const ConfigValue<AnimMultiPaletteSubtileResolutionStrategy> &multi_palette_strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes,
    const Image<IndexPixel> &tiles_png,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &palette_printer,
    const TilePrinter &tile_printer)
{
    if (per_subtile_strategies.size() != tile_count) {
        panic(
            "per_subtile_strategies size " + std::to_string(per_subtile_strategies.size()) + " != tile_count " +
            std::to_string(tile_count));
    }

    std::vector<std::size_t> per_tile_palettes(tile_count);
    std::optional<std::size_t> internal_png_palette_cache;

    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_index = tile_offset + i;
        PT_TRY_ASSIGN_CHAIN_ERR(
            palette_idx,
            resolve_subtile_palette(
                anim_name,
                i,
                tile_index,
                metatiles_bin,
                per_subtile_strategies[i],
                multi_palette_strategy,
                anim,
                palettes,
                tiles_png,
                extrinsic_transparency,
                diag,
                palette_printer,
                tile_printer,
                internal_png_palette_cache),
            std::vector<std::size_t>,
            diag.formatter().format(
                "Failed to resolve palette for animation '{}' subtile {}.",
                FormatParam{anim_name, Style::bold},
                FormatParam{i, Style::bold}));
        per_tile_palettes[i] = palette_idx;
    }

    // Emit a remark if multiple distinct palettes are used across subtiles
    if (tile_count > 1) {
        const std::size_t first_palette = per_tile_palettes.at(0);
        const bool uses_multiple_palettes =
            !std::ranges::all_of(per_tile_palettes, [&](std::size_t idx) { return idx == first_palette; });
        if (uses_multiple_palettes) {
            std::set<std::size_t> unique_palettes{per_tile_palettes.begin(), per_tile_palettes.end()};
            std::string palette_list;
            for (const auto &palette_idx : unique_palettes) {
                if (!palette_list.empty()) {
                    palette_list += ", ";
                }
                palette_list += palette_filename(palette_idx);
            }
            diag.remark(
                "animation-palette-resolution-strategy",
                {diag.formatter().format(
                    "Animation '{}' uses multiple palettes across subtiles: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{palette_list, Style::bold})});
        }
    }

    return per_tile_palettes;
}

struct DuplicateInfo {
    std::vector<std::size_t> inter_anim_indices;
    std::vector<std::size_t> cross_range_indices;
    std::vector<std::pair<std::size_t, std::size_t>> intra_anim_pairs;

    [[nodiscard]] bool any() const
    {
        return !inter_anim_indices.empty() || !cross_range_indices.empty() || !intra_anim_pairs.empty();
    }
};

/// @brief Categorizes duplicate key frame tiles into inter-animation, cross-range, and intra-animation duplicates.
///
/// @details
/// Inter-animation duplicates are key frame tiles matching another animation's key frame tile. Cross-range duplicates
/// are key frame tiles matching an external (non-animation) tile in tiles.png, decoded under the subtile's own
/// palette. Intra-animation duplicates are pairs of key frame tiles matching each other.
///
/// Inter-animation tiles are checked before cross-range tiles so that the more specific category wins when a tile
/// appears in both sets.
///
/// All comparison happens on canonical decoded RGBA tiles, matching how the compile pipeline compares key frame tiles,
/// i.e. by the rgba form. Two index tiles that reference different palette slots holding the same color are therefore
/// duplicates, as are flip-equivalent tiles.
[[nodiscard]] DuplicateInfo categorize_duplicate_key_frame_tiles(
    const std::vector<PixelTile<Rgba32>> &key_frame_canonical_rgba_tiles,
    const std::set<PixelTile<Rgba32>> &inter_anim_canonical_tiles,
    const std::vector<const std::set<PixelTile<Rgba32>> *> &external_canonical_rgba_tiles)
{
    DuplicateInfo info;

    // Map from canonical decoded tile to first index seen
    std::map<PixelTile<Rgba32>, std::size_t> seen;

    for (std::size_t i = 0; i < key_frame_canonical_rgba_tiles.size(); ++i) {
        const PixelTile<Rgba32> &base = key_frame_canonical_rgba_tiles[i];

        // Check inter-animation before cross-range (more specific category wins)
        if (inter_anim_canonical_tiles.contains(base)) {
            info.inter_anim_indices.push_back(i);
        }
        else if (external_canonical_rgba_tiles[i]->contains(base)) {
            info.cross_range_indices.push_back(i);
        }

        auto [it, inserted] = seen.emplace(base, i);
        if (!inserted) {
            info.intra_anim_pairs.emplace_back(it->second, i);
        }
    }

    return info;
}

/// @brief Backports mangle records to the tiles.png image in the Porymap component.
///
/// @param component The Porymap component containing tiles_png to modify
/// @param base_tile_offset The tile offset of the animation in tiles.png
/// @param records The mangle records describing pixel changes
void backport_mangles_to_tiles_png(
    PorymapTilesetComponent &component, std::size_t base_tile_offset, const std::set<TileMangleRecord> &records)
{
    Image<IndexPixel> tiles_img = component.tiles_png();
    constexpr std::size_t tiles_per_row = metatile::metatiles_per_row * metatile::tiles_per_side;

    // Mangle records are non-overlapping: each targets a distinct tile_index (guaranteed by mangle_duplicates).
    // Sequential application is therefore safe and order-independent, producing results consistent with the in-memory
    // key frame tiles that were mangled during decompilation.
    for (const auto &record : records) {
        const std::size_t global_tile_idx = base_tile_offset + record.tile_index;
        const std::size_t tile_row = global_tile_idx / tiles_per_row;
        const std::size_t tile_col = global_tile_idx % tiles_per_row;

        for (const auto &change : record.pixel_changes) {
            const auto [pixel_row, pixel_col] = tile::index_to_row_col(change.pixel_index);
            const std::size_t img_row = tile_row * tile::side_length_pix + pixel_row;
            const std::size_t img_col = tile_col * tile::side_length_pix + pixel_col;

            tiles_img.set(img_row, img_col, change.mangled_pixel);
        }
    }

    component.tiles_png(tiles_img);
}

} // namespace

namespace porytiles {

ChainableResult<Animation<Rgba32>> AnimDecompiler::decompile_animation(
    const std::string &tileset_name,
    const Animation<IndexPixel> &anim,
    const std::set<PixelTile<Rgba32>> &inter_anim_canonical_tiles,
    PorymapTilesetComponent &porymap_component) const
{
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_anim_palette_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_anim_key_frame_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(
        config_, global_anim_multi_palette_subtile_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_frame_linking, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, per_anim_overrides, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, import_transparency, tileset_name, Animation<Rgba32>);

    // Animation frames have no missing layer group, so the mixed import transparency mode writes their transparent
    // pixels the same way as the extrinsic mode. Duplicate detection below keeps using the extrinsic color because it
    // compares canonical forms, which handle all transparency the same.
    const Rgba32 frame_transparent_color =
        import_transparency.value() == ImportTransparencyMode::alpha ? Rgba32{} : extrinsic_transparency.value();

    // Read data from porymap_component
    const auto &palettes = porymap_component.palettes();
    const auto &metatiles_bin = porymap_component.metatiles_bin();
    const auto &tiles_png = porymap_component.tiles_png();

    Animation<Rgba32> result{anim.name()};
    result.params(anim.params());

    // Get the tile offset from animation params to determine which tile index to look for in metatiles
    const std::size_t tile_offset = anim.params().tile_offset();
    const std::size_t tile_count = anim.params().tile_count();

    // Validate tile range before any arithmetic that assumes tile_count > 0
    if (tile_count == 0) {
        return FormattableError{
            std::vector<std::string>{
                "Animation '{}' has a tile count of '{}'.",
                "The animation's generated C code may not have been parsed correctly, or the animation defines no "
                "parameters."},
            std::vector<std::vector<FormatParam>>{
                {FormatParam{anim.name(), Style::bold}, FormatParam{tile_count, Style::bold}}, {}}};
    }
    if (tile_offset == 0) {
        return FormattableError{
            std::vector<std::string>{
                "Animation '{}' has a tile offset of '{}'.",
                "Tile 0 in tiles.png is reserved and cannot be an animation tile."},
            std::vector<std::vector<FormatParam>>{
                {FormatParam{anim.name(), Style::bold}, FormatParam{tile_offset, Style::bold}}, {}}};
    }

    // Build per-subtile palette resolution strategies using a three-tier cascade:
    //   1. Per-tile (per_tile_palette_resolution_strategies[i]): most specific
    //   2. Per-anim (palette_resolution_strategy): middle tier
    //   3. Global (global_anim_palette_resolution_strategy): least specific fallback
    std::vector<ConfigValue<AnimPaletteResolutionStrategy>> per_subtile_strategies;
    per_subtile_strategies.reserve(tile_count);

    const auto &configs_map = per_anim_overrides.value();
    const auto anim_cfg_it = configs_map.find(anim.name());
    const PerAnimOverride *anim_cfg_ptr = (anim_cfg_it != configs_map.end()) ? &anim_cfg_it->second : nullptr;

    if (anim_cfg_ptr != nullptr) {
        const PerAnimOverride &anim_cfg = *anim_cfg_ptr;

        // Determine the "effective default" for this animation: per-anim if set, otherwise global
        const ConfigValue<AnimPaletteResolutionStrategy> effective_default =
            anim_cfg.palette_resolution_strategy.has_value()
                ? per_anim_overrides.derive(anim_cfg.palette_resolution_strategy)
                : global_anim_palette_resolution_strategy;

        if (!anim_cfg.per_tile_palette_resolution_strategies.empty()) {
            if (anim_cfg.per_tile_palette_resolution_strategies.size() != tile_count) {
                return FormattableError{
                    std::vector<std::string>{
                        "Animation '{}' config 'per_tile_palette_resolution_strategies' has '{}' entries, but "
                        "animation has '{}' subtiles.",
                        "The per_tile_palette_resolution_strategies list must have exactly one entry per subtile."},
                    std::vector<std::vector<FormatParam>>{
                        {FormatParam{anim.name(), Style::bold},
                         FormatParam{anim_cfg.per_tile_palette_resolution_strategies.size(), Style::bold},
                         FormatParam{tile_count, Style::bold}},
                        {}}};
            }
            for (std::size_t i = 0; i < tile_count; ++i) {
                if (anim_cfg.per_tile_palette_resolution_strategies[i].has_value()) {
                    per_subtile_strategies.push_back(
                        per_anim_overrides.derive(anim_cfg.per_tile_palette_resolution_strategies[i]));
                }
                else {
                    per_subtile_strategies.push_back(effective_default);
                }
            }
        }
        else {
            // AnimConfig exists but has no per-tile strategies. Use effective default for all subtiles
            for (std::size_t i = 0; i < tile_count; ++i) {
                per_subtile_strategies.push_back(effective_default);
            }
        }
    }
    else {
        // No AnimConfig for this animation. Use global for all subtiles
        for (std::size_t i = 0; i < tile_count; ++i) {
            per_subtile_strategies.push_back(global_anim_palette_resolution_strategy);
        }
    }

    // Compute the effective multi-palette subtile resolution strategy: per-anim override wins, otherwise global
    // fallback.
    const ConfigValue<AnimMultiPaletteSubtileResolutionStrategy> effective_multi_palette_strategy =
        (anim_cfg_ptr != nullptr && anim_cfg_ptr->multi_palette_subtile_resolution_strategy.has_value())
            ? per_anim_overrides.derive(anim_cfg_ptr->multi_palette_subtile_resolution_strategy)
            : global_anim_multi_palette_subtile_resolution_strategy;

    // Resolve effective FrameLinking for this animation
    const ConfigValue<FrameLinking> effective_linking = (anim_cfg_ptr != nullptr && anim_cfg_ptr->linking.has_value())
                                                            ? per_anim_overrides.derive(anim_cfg_ptr->linking)
                                                            : global_frame_linking;

    if (effective_linking == FrameLinking::hybrid) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag_->formatter().format(
            "Hybrid frame linking is not yet implemented (animation '{}').", FormatParam{anim.name(), Style::bold}));
        err_msg.emplace_back("Use 'automatic' or 'manual' frame linking until hybrid support.");
        err_msg.append_range(format_config_note_with_separator(diag_->formatter(), effective_linking));
        return FormattableError{err_msg};
    }

    // Manual mode: extract override entries from metatiles_bin and skip key frame generation.
    // Regular frames are still decompiled using the per-subtile palette resolution cascade.
    if (effective_linking == FrameLinking::manual) {
        std::vector<AnimOverrideEntry> overrides;
        const std::size_t num_metatiles = metatiles_bin.size() / metatile::entries_per_metatile_triple;
        for (std::size_t mt_idx = 0; mt_idx < num_metatiles; ++mt_idx) {
            for (std::size_t local_idx = 0; local_idx < metatile::entries_per_metatile_triple; ++local_idx) {
                const auto &entry = metatiles_bin[mt_idx * metatile::entries_per_metatile_triple + local_idx];
                if (entry.tile_index() >= tile_offset && entry.tile_index() < tile_offset + tile_count) {
                    auto [layer, subtile] = metatile::from_internal_tile_index(local_idx);
                    overrides.push_back(
                        AnimOverrideEntry{
                            mt_idx,
                            layer,
                            subtile,
                            entry.tile_index() - tile_offset,
                            entry.palette_index(),
                            entry.h_flip(),
                            entry.v_flip()});
                }
            }
        }

        AnimParams result_params = anim.params();
        result_params.overrides(std::move(overrides));
        result.params(std::move(result_params));

        // Decompile regular frames using per-subtile palette resolution
        PT_TRY_ASSIGN_CHAIN_ERR(
            manual_palette_indices,
            find_palettes_for_anim_tiles(
                anim.name(),
                tile_offset,
                tile_count,
                metatiles_bin,
                per_subtile_strategies,
                effective_multi_palette_strategy,
                anim,
                palettes,
                tiles_png,
                extrinsic_transparency,
                *diag_,
                *palette_printer_,
                *tile_printer_),
            Animation<Rgba32>,
            diag_->formatter().format(
                "Failed to find palette for animation '{}'.", FormatParam{anim.name(), Style::bold}));

        for (const auto &frame : anim.frames_values()) {
            std::vector<PixelTile<Rgba32>> rgba_tiles;
            rgba_tiles.reserve(frame.tiles().size());
            for (std::size_t i = 0; i < frame.tiles().size(); ++i) {
                rgba_tiles.push_back(color_tile_from_index_tile(
                    frame.tiles()[i], palettes.at(manual_palette_indices[i]), frame_transparent_color));
            }
            AnimFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
            result.put_frame(frame.frame_name(), std::move(rgba_frame));
        }

        return result;
    }

    // Recover per-subtile palette indices
    PT_TRY_ASSIGN_CHAIN_ERR(
        palette_indices,
        find_palettes_for_anim_tiles(
            anim.name(),
            tile_offset,
            tile_count,
            metatiles_bin,
            per_subtile_strategies,
            effective_multi_palette_strategy,
            anim,
            palettes,
            tiles_png,
            extrinsic_transparency,
            *diag_,
            *palette_printer_,
            *tile_printer_),
        Animation<Rgba32>,
        diag_->formatter().format("Failed to find palette for animation '{}'.", FormatParam{anim.name(), Style::bold}));

    // Build per-tile palette pointer vector for the mangler and conversion
    std::vector<const Palette<Rgba32, palette::max_size> *> palette_ptrs;
    palette_ptrs.reserve(palette_indices.size());
    for (std::size_t idx : palette_indices) {
        palette_ptrs.push_back(&palettes.at(idx));
    }

    // Extract key frame tiles from tiles.png
    std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
        extract_tiles_from_image(tiles_png, tile_offset, tile_count);

    // Decode each key frame subtile to its canonical RGBA form. Duplicate detection and mangling both operate on
    // decoded colors because the compile-side key frame validation compares key.png tiles by color: two index tiles
    // that reference different palette slots holding the same color are duplicates there, even though they differ in
    // index space.
    std::vector<PixelTile<Rgba32>> key_frame_canonical_rgba_tiles;
    key_frame_canonical_rgba_tiles.reserve(tile_count);
    for (std::size_t i = 0; i < key_frame_index_tiles.size(); ++i) {
        key_frame_canonical_rgba_tiles.push_back(canonical_color_tile_from_index_tile(
            key_frame_index_tiles[i], *palette_ptrs[i], extrinsic_transparency.value()));
    }

    // Build, per distinct subtile palette, the canonical RGBA forms of all tiles.png tiles OUTSIDE the current
    // animation's key frame range. Tiles carry no palette of their own in tiles.png, so each one is resolved under the
    // palettes this animation's subtiles actually use: subtile i is compared against external tiles resolved under
    // subtile i's palette.
    const std::size_t total_tiles =
        (tiles_png.height() / tile::side_length_pix) * (tiles_png.width() / tile::side_length_pix);
    const std::set<std::size_t> distinct_palette_indices{palette_indices.begin(), palette_indices.end()};
    std::map<std::size_t, std::set<PixelTile<Rgba32>>> external_rgba_by_palette;
    for (const std::size_t palette_idx : distinct_palette_indices) {
        auto &tile_set = external_rgba_by_palette[palette_idx];
        for (std::size_t i = 0; i < total_tiles; ++i) {
            if (i >= tile_offset && i < tile_offset + tile_count) {
                continue;
            }
            tile_set.insert(canonical_color_tile_from_index_tile(
                extract_single_tile(tiles_png, i), palettes.at(palette_idx), extrinsic_transparency.value()));
        }
    }

    // Combined per-palette sets (external plus inter-animation) back the mangler's uniqueness checks. The
    // external-only sets stay separate so duplicate categorization can distinguish cross-range duplicates from
    // inter-animation duplicates.
    std::map<std::size_t, std::set<PixelTile<Rgba32>>> combined_rgba_by_palette = external_rgba_by_palette;
    for (auto &tile_set : combined_rgba_by_palette | std::views::values) {
        tile_set.insert(inter_anim_canonical_tiles.begin(), inter_anim_canonical_tiles.end());
    }

    std::vector<const std::set<PixelTile<Rgba32>> *> external_per_subtile;
    std::vector<const std::set<PixelTile<Rgba32>> *> combined_per_subtile;
    external_per_subtile.reserve(tile_count);
    combined_per_subtile.reserve(tile_count);
    for (const std::size_t palette_idx : palette_indices) {
        external_per_subtile.push_back(&external_rgba_by_palette.at(palette_idx));
        combined_per_subtile.push_back(&combined_rgba_by_palette.at(palette_idx));
    }

    // Compute the effective key frame resolution strategy: per-anim override wins, otherwise global fallback.
    const ConfigValue<AnimKeyFrameResolutionStrategy> effective_key_frame_strategy =
        (anim_cfg_ptr != nullptr && anim_cfg_ptr->key_frame_resolution_strategy.has_value())
            ? per_anim_overrides.derive(anim_cfg_ptr->key_frame_resolution_strategy)
            : global_anim_key_frame_resolution_strategy;

    // Detect duplicate key frame tiles on canonical decoded RGBA forms. Detects:
    // - Inter-animation duplicates: animation tile matches another animation's key frame tile
    // - Cross-range duplicates: animation tile matches a non-animation tile in tiles.png
    // - Intra-animation duplicates: two animation tiles match each other
    const auto dup_info = categorize_duplicate_key_frame_tiles(
        key_frame_canonical_rgba_tiles, inter_anim_canonical_tiles, external_per_subtile);
    if (dup_info.any()) {
        switch (effective_key_frame_strategy.value()) {
        case AnimKeyFrameResolutionStrategy::error: {
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag_->formatter().format(
                "Animation '{}' has duplicate key frame tiles:", FormatParam{anim.name(), Style::bold}));
            for (const auto &idx : dup_info.inter_anim_indices) {
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Tile {} matches another animation's key frame tile.", FormatParam{idx, Style::bold}));
            }
            for (const auto &idx : dup_info.cross_range_indices) {
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Tile {} matches a non-animation tile in tiles.png.", FormatParam{idx, Style::bold}));
            }
            for (const auto &[i, j] : dup_info.intra_anim_pairs) {
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Tile {} and tile {} match.", FormatParam{i, Style::bold}, FormatParam{j, Style::bold}));
            }
            err_msg.emplace_back("");
            err_msg.emplace_back(
                "Tiles are compared by resolved color: flip-equivalent tiles, and tiles that reference different "
                "palette slots holding the same color, count as duplicates.");
            err_msg.emplace_back("");
            err_msg.emplace_back("Consider using 'mangle' strategy to auto-resolve.");
            err_msg.append_range(format_config_note_with_separator(diag_->formatter(), effective_key_frame_strategy));
            return FormattableError{err_msg};
        }

        case AnimKeyFrameResolutionStrategy::warning: {
            panic("warning not yet implemented");
        }

        case AnimKeyFrameResolutionStrategy::mangle: {
            AnimKeyFrameMangler mangler{diag_, tile_printer_};
            PT_TRY_ASSIGN_CHAIN_ERR(
                mangle_result,
                mangler.mangle_duplicates(
                    anim.name(),
                    std::move(key_frame_index_tiles),
                    palette_ptrs,
                    extrinsic_transparency.value(),
                    combined_per_subtile),
                Animation<Rgba32>,
                diag_->formatter().format(
                    "Failed to mangle duplicate key frame tiles for animation '{}'.",
                    FormatParam{anim.name(), Style::bold}));
            key_frame_index_tiles = std::move(mangle_result.tiles);

            // Backport changes to tiles.png
            if (!mangle_result.mangle_records.empty()) {
                backport_mangles_to_tiles_png(porymap_component, tile_offset, mangle_result.mangle_records);
            }
            break;
        }

        default:
            panic("unhandled AnimKeyFrameResolutionStrategy value");
        }
    }

    // Decompile key frame tiles to Rgba32 using per-subtile palettes
    std::vector<PixelTile<Rgba32>> key_frame_rgba_tiles;
    key_frame_rgba_tiles.reserve(key_frame_index_tiles.size());
    for (std::size_t i = 0; i < key_frame_index_tiles.size(); ++i) {
        key_frame_rgba_tiles.push_back(color_tile_from_index_tile(
            key_frame_index_tiles[i], palettes.at(palette_indices[i]), frame_transparent_color));
    }

    // Set the key frame on the result
    AnimFrame key_frame{"key", std::move(key_frame_rgba_tiles)};
    result.key_frame(std::move(key_frame));

    for (const auto &frame : anim.frames_values()) {
        if (frame.tiles().size() != tile_count) {
            panic(
                "frame '" + frame.frame_name() + "' tile count " + std::to_string(frame.tiles().size()) +
                " != animation tile_count " + std::to_string(tile_count));
        }

        std::vector<PixelTile<Rgba32>> rgba_tiles;
        rgba_tiles.reserve(frame.tiles().size());

        for (std::size_t i = 0; i < frame.tiles().size(); ++i) {
            rgba_tiles.push_back(
                color_tile_from_index_tile(frame.tiles()[i], palettes.at(palette_indices[i]), frame_transparent_color));
        }

        AnimFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
        result.put_frame(frame.frame_name(), std::move(rgba_frame));
    }

    return result;
}

} // namespace porytiles
