#include "porytiles2/domain/services/anim_decompiler.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_multi_pal_subtile_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/config/frame_linking.hpp"
#include "porytiles2/domain/config/per_anim_overrides.hpp"
#include "porytiles2/domain/models/anim_frame.hpp"
#include "porytiles2/domain/models/anim_override_entry.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/services/anim_key_frame_mangler.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles2;

[[nodiscard]] ChainableResult<std::size_t> internal_png_pal_strategy(
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &tileset_pals,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer)
{
    if (!anim.has_frames()) {
        panic("anim '" + anim.name() + "' has no frames");
    }

    const auto &representative_frame = anim.frames().begin()->second;
    const auto &representative_pal = representative_frame.palette();

    // Representative pal must have exactly 16 colors to match GBA palette format
    if (representative_pal.size() != pal::max_size) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag.formatter().format(
            "Representative frame '{}' internal palette size '{}': must be '{}'.",
            FormatParam{representative_frame.frame_name(), Style::bold},
            FormatParam{representative_pal.size(), Style::bold},
            FormatParam{pal::max_size, Style::bold}));
        err_msg.emplace_back("");
        err_msg.append_range(pal_printer.print_rgba_pal(representative_pal));
        return FormattableError{err_msg};
    }

    // Check for extrinsic transparency in non-slot-0 positions in representative pal
    std::vector<std::size_t> extrinsic_transparency_slots;
    for (std::size_t slot = 1; slot < pal::max_size; ++slot) {
        const Rgba32 &color = representative_pal.at(slot);
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
            pal_printer.print_rgba_pal_with_highlights(representative_pal, extrinsic_transparency_slots));
        err_msg.append_range(format_config_note_with_separator(diag.formatter(), extrinsic_transparency));
        return FormattableError{err_msg};
    }

    // Check all frames share the same internal palette as the representative
    for (const auto &[frame_name, frame] : anim.frames()) {
        if (&frame == &representative_frame) {
            continue;
        }
        const auto &frame_pal = frame.palette();
        bool palettes_match = (frame_pal.size() == representative_pal.size());
        if (palettes_match) {
            for (std::size_t slot = 0; slot < representative_pal.size(); ++slot) {
                if (frame_pal.at(slot) != representative_pal.at(slot)) {
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
            err_msg.append_range(pal_printer.print_rgba_pal(representative_pal));
            err_msg.emplace_back("");
            err_msg.emplace_back(diag.formatter().format("Frame '{}' palette:", FormatParam{frame_name, Style::bold}));
            err_msg.append_range(pal_printer.print_rgba_pal(frame_pal));
            return FormattableError{err_msg};
        }
    }

    /*
     * Now that we fully validated the representative pal, and we confirmed that all frame pals match, we can try to
     * match the representative pal to one of the tileset pals.
     */
    for (std::size_t pal_idx = 0; pal_idx < tileset_pals.size(); ++pal_idx) {
        bool matches = true;
        for (std::size_t slot = 1; slot < pal::max_size; ++slot) {
            const Rgba32 &png_pal_color = representative_pal.at(slot);

            // This should never happen, we returned early above if we hit this
            if (png_pal_color.is_transparent(extrinsic_transparency)) {
                panic("png_pal slot " + std::to_string(slot) + " is extrinsically transparent");
            }

            const Rgba32 &tileset_color = tileset_pals[pal_idx].at(slot);
            if (png_pal_color != tileset_color) {
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
                FormatParam{pal_filename(pal_idx), Style::bold}));
            remark_lines.emplace_back("");
            remark_lines.append_range(pal_printer.print_rgba_pal(tileset_pals[pal_idx]));
            diag.remark("animation-palette-resolution-strategy", remark_lines);
            return pal_idx;
        }
    }

    std::vector<std::string> err_msg{};
    err_msg.emplace_back(diag.formatter().format(
        "Failed to find matching palette for internal palette of representative frame '{}'.",
        FormatParam{representative_frame.frame_name(), Style::bold}));
    err_msg.emplace_back("");
    err_msg.append_range(pal_printer.print_rgba_pal(representative_pal));
    return FormattableError{err_msg};
}

[[nodiscard]] std::optional<std::size_t> extract_pal_index(AnimPalResolutionStrategy strategy)
{
    switch (strategy) {
    case AnimPalResolutionStrategy::palette_00:
        return 0;
    case AnimPalResolutionStrategy::palette_01:
        return 1;
    case AnimPalResolutionStrategy::palette_02:
        return 2;
    case AnimPalResolutionStrategy::palette_03:
        return 3;
    case AnimPalResolutionStrategy::palette_04:
        return 4;
    case AnimPalResolutionStrategy::palette_05:
        return 5;
    case AnimPalResolutionStrategy::palette_06:
        return 6;
    case AnimPalResolutionStrategy::palette_07:
        return 7;
    case AnimPalResolutionStrategy::palette_08:
        return 8;
    case AnimPalResolutionStrategy::palette_09:
        return 9;
    case AnimPalResolutionStrategy::palette_10:
        return 10;
    case AnimPalResolutionStrategy::palette_11:
        return 11;
    case AnimPalResolutionStrategy::palette_12:
        return 12;
    case AnimPalResolutionStrategy::palette_13:
        return 13;
    case AnimPalResolutionStrategy::palette_14:
        return 14;
    case AnimPalResolutionStrategy::palette_15:
        return 15;
    default:
        return std::nullopt;
    }
}

/**
 * @brief Resolves the palette index for a single subtile using the given strategy.
 *
 * @details
 * Dispatches to the appropriate resolution logic based on the strategy: explicit palette index, scan local metatiles,
 * internal PNG palette matching, or scan all tilesets.
 *
 * @param anim_name The animation name (for diagnostics)
 * @param subtile_index The subtile index within the animation (0-based)
 * @param tile_index The absolute tile index in tiles.png
 * @param metatiles_bin The metatile entries to scan
 * @param strategy The per-subtile strategy config value
 * @param anim The animation (needed for internal_png_pal strategy)
 * @param pals The tileset palettes
 * @param tiles_png The tiles.png image
 * @param extrinsic_transparency The extrinsic transparency color
 * @param diag User diagnostics for reporting
 * @param pal_printer Palette printer for diagnostic output
 * @param tile_printer Tile printer for diagnostic output
 * @param internal_png_pal_cache Cached result from internal_png_pal_strategy (populated on first use)
 * @return The resolved palette index for this subtile
 */
[[nodiscard]] ChainableResult<std::size_t> resolve_subtile_palette(
    const std::string &anim_name,
    std::size_t subtile_index,
    std::size_t tile_index,
    std::span<const TilemapEntry> metatiles_bin,
    const ConfigValue<AnimPalResolutionStrategy> &strategy,
    const ConfigValue<AnimMultiPalSubtileResolutionStrategy> &multi_pal_strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    const Image<IndexPixel> &tiles_png,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer,
    const TilePrinter &tile_printer,
    std::optional<std::size_t> &internal_png_pal_cache)
{
    // Check pal_N strategies first: direct palette index assignment
    const auto explicit_pal = extract_pal_index(strategy.value());
    if (explicit_pal.has_value()) {
        diag.remark(
            "animation-palette-resolution-strategy",
            {diag.formatter().format(
                "Animation '{}' subtile {} using explicit palette '{}'.",
                FormatParam{anim_name, Style::bold},
                FormatParam{subtile_index, Style::bold},
                FormatParam{pal_filename(*explicit_pal), Style::bold})});
        diag.remark_note("animation-palette-resolution-strategy", format_config_note(diag.formatter(), strategy));
        return *explicit_pal;
    }

    switch (strategy.value()) {
    case AnimPalResolutionStrategy::scan_local_metatiles: {
        std::set<std::size_t> found_for_subtile{};

        for (const auto &entry : metatiles_bin) {
            if (entry.tile_index() == tile_index) {
                found_for_subtile.insert(entry.pal_index());
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
            /*
             * A single tile index can be referenced by multiple metatile entries with different palette indices.
             * This is valid GBA behavior. The hardware selects palette per metatile entry, not per tile.
             *
             * The multi_pal_strategy config determines how to handle this case.
             */
            std::string pal_list;
            for (const auto &pal_idx : found_for_subtile) {
                if (!pal_list.empty()) {
                    pal_list += ", ";
                }
                pal_list += pal_filename(pal_idx);
            }

            switch (multi_pal_strategy.value()) {
            case AnimMultiPalSubtileResolutionStrategy::error: {
                std::vector<std::string> err_msg;
                err_msg.push_back(diag.formatter().format(
                    "Animation '{}' subtile {} at tile index '{}' is referenced with multiple palettes: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{subtile_index, Style::bold},
                    FormatParam{tile_index, Style::bold},
                    FormatParam{pal_list, Style::bold}));
                err_msg.emplace_back(
                    "Picking one palette arbitrarily would produce incorrect RGBA output in the layer PNGs.");

                const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_png, tile_index);
                err_msg.emplace_back("");
                for (const auto &pal_idx : found_for_subtile) {
                    const PixelTile<Rgba32> rgba_tile =
                        color_tile_from_index_tile(index_tile, pals.at(pal_idx), extrinsic_transparency.value());
                    err_msg.push_back(diag.formatter().format(
                        "Tile under palette '{}':", FormatParam{pal_filename(pal_idx), Style::bold}));
                    err_msg.append_range(tile_printer.print_tile(rgba_tile, extrinsic_transparency.value()));
                }

                err_msg.emplace_back("");
                err_msg.emplace_back(
                    "Consider using an explicit palette resolution strategy (e.g. 'palette-00') to resolve the "
                    "ambiguity.");
                err_msg.append_range(format_config_note_with_separator(diag.formatter(), strategy));
                return FormattableError{err_msg};
            }
            case AnimMultiPalSubtileResolutionStrategy::warning: {
                const std::size_t chosen_pal = *found_for_subtile.begin();

                std::vector<std::string> warn_msg;
                warn_msg.push_back(diag.formatter().format(
                    "Animation '{}' subtile {} at tile index '{}' is referenced with multiple palettes: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{subtile_index, Style::bold},
                    FormatParam{tile_index, Style::bold},
                    FormatParam{pal_list, Style::bold}));
                warn_msg.push_back(diag.formatter().format(
                    "Using palette '{}'. Set 'frame_linking: manual' to handle palette assignment via overrides.",
                    FormatParam{pal_filename(chosen_pal), Style::bold}));

                const PixelTile<IndexPixel> warn_index_tile = extract_single_tile(tiles_png, tile_index);
                warn_msg.emplace_back("");
                for (const auto &pal_idx : found_for_subtile) {
                    const PixelTile<Rgba32> rgba_tile =
                        color_tile_from_index_tile(warn_index_tile, pals.at(pal_idx), extrinsic_transparency.value());
                    warn_msg.push_back(diag.formatter().format(
                        "Tile under palette '{}':", FormatParam{pal_filename(pal_idx), Style::bold}));
                    warn_msg.append_range(tile_printer.print_tile(rgba_tile, extrinsic_transparency.value()));
                }

                warn_msg.append_range(format_config_note_with_separator(diag.formatter(), multi_pal_strategy));
                diag.warning("animation-multi-pal-subtile", warn_msg);

                return chosen_pal;
            }
            case AnimMultiPalSubtileResolutionStrategy::split:
                return FormattableError{"The 'split' mode for multi-pal subtile resolution is not yet implemented."};
            }
        }

        return *found_for_subtile.begin();
    }

    case AnimPalResolutionStrategy::internal_png_pal: {
        if (internal_png_pal_cache.has_value()) {
            return *internal_png_pal_cache;
        }
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag.formatter().format(
            "Palette resolution strategy '{}' failed.",
            FormatParam{to_string(AnimPalResolutionStrategy::internal_png_pal), Style::bold}));
        err_msg.append_range(format_config_note_with_separator(diag.formatter(), strategy));
        PT_TRY_ASSIGN_CHAIN_ERR(
            match,
            internal_png_pal_strategy(anim, pals, extrinsic_transparency, diag, pal_printer),
            std::size_t,
            err_msg);
        internal_png_pal_cache = match;
        return match;
    }

    case AnimPalResolutionStrategy::scan_all_tilesets:
        panic("scan_all_tilesets not yet implemented");

    default:
        panic("unhandled AnimPalResolutionStrategy value");
    }
}

[[nodiscard]] ChainableResult<std::vector<std::size_t>> find_pals_for_anim_tiles(
    const std::string &anim_name,
    std::size_t tile_offset,
    std::size_t tile_count,
    std::span<const TilemapEntry> metatiles_bin,
    const std::vector<ConfigValue<AnimPalResolutionStrategy>> &per_subtile_strategies,
    const ConfigValue<AnimMultiPalSubtileResolutionStrategy> &multi_pal_strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    const Image<IndexPixel> &tiles_png,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer,
    const TilePrinter &tile_printer)
{
    if (per_subtile_strategies.size() != tile_count) {
        panic(
            "per_subtile_strategies size " + std::to_string(per_subtile_strategies.size()) + " != tile_count " +
            std::to_string(tile_count));
    }

    std::vector<std::size_t> per_tile_pals(tile_count);
    std::optional<std::size_t> internal_png_pal_cache;

    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_index = tile_offset + i;
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_idx,
            resolve_subtile_palette(
                anim_name,
                i,
                tile_index,
                metatiles_bin,
                per_subtile_strategies[i],
                multi_pal_strategy,
                anim,
                pals,
                tiles_png,
                extrinsic_transparency,
                diag,
                pal_printer,
                tile_printer,
                internal_png_pal_cache),
            std::vector<std::size_t>,
            diag.formatter().format(
                "Failed to resolve palette for animation '{}' subtile {}.",
                FormatParam{anim_name, Style::bold},
                FormatParam{i, Style::bold}));
        per_tile_pals[i] = pal_idx;
    }

    // Emit a remark if multiple distinct palettes are used across subtiles
    if (tile_count > 1) {
        const std::size_t first_pal = per_tile_pals.at(0);
        const bool uses_multiple_palettes =
            !std::ranges::all_of(per_tile_pals, [&](std::size_t idx) { return idx == first_pal; });
        if (uses_multiple_palettes) {
            std::set<std::size_t> unique_pals{per_tile_pals.begin(), per_tile_pals.end()};
            std::string pal_list;
            for (const auto &pal_idx : unique_pals) {
                if (!pal_list.empty()) {
                    pal_list += ", ";
                }
                pal_list += pal_filename(pal_idx);
            }
            diag.remark(
                "animation-palette-resolution-strategy",
                {diag.formatter().format(
                    "Animation '{}' uses multiple palettes across subtiles: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{pal_list, Style::bold})});
        }
    }

    return per_tile_pals;
}

/**
 * @brief Checks whether any key frame tiles are duplicates, considering cross-range, inter-animation, and
 * intra-animation duplicates.
 *
 * @details
 * A duplicate is detected if a key frame tile's canonical form matches either:
 * - An inter-animation tile (another animation's key frame tile)
 * - An external tile (cross-range duplicate: animation tile matches a non-animation tile in tiles.png)
 * - An earlier key frame tile (intra-animation duplicate: two animation tiles are flip-equivalent)
 *
 * @param key_frame_tiles The animation key frame tiles to check
 * @param inter_anim_canonical_tiles Canonical forms of other animations' key frame tiles
 * @param external_canonical_tiles Canonical forms of all non-animation tiles in tiles.png
 * @return True if any duplicate is found
 */
[[nodiscard]] bool has_duplicate_key_frame_tiles(
    const std::vector<PixelTile<IndexPixel>> &key_frame_tiles,
    const std::set<PixelTile<IndexPixel>> &inter_anim_canonical_tiles,
    const std::set<PixelTile<IndexPixel>> &external_canonical_tiles)
{
    std::set<PixelTile<IndexPixel>> seen;
    for (const auto &tile : key_frame_tiles) {
        const CanonicalPixelTile canonical{tile};
        const PixelTile<IndexPixel> &base = canonical;
        if (inter_anim_canonical_tiles.contains(base)) {
            return true;
        }
        if (external_canonical_tiles.contains(base)) {
            return true;
        }
        if (!seen.insert(base).second) {
            return true;
        }
    }
    return false;
}

struct DuplicateInfo {
    std::vector<std::size_t> inter_anim_indices;
    std::vector<std::size_t> cross_range_indices;
    std::vector<std::pair<std::size_t, std::size_t>> intra_anim_pairs;
};

/**
 * @brief Categorizes duplicate key frame tiles into inter-animation, cross-range, and intra-animation duplicates.
 *
 * @details
 * Inter-animation duplicates are key frame tiles whose canonical form matches another animation's key frame tile.
 * Cross-range duplicates are key frame tiles whose canonical form matches an external (non-animation) tile.
 * Intra-animation duplicates are pairs of key frame tiles that are flip-equivalent to each other.
 *
 * Inter-animation tiles are checked before cross-range tiles so that the more specific category wins when a tile
 * appears in both sets.
 *
 * @param key_frame_tiles The animation key frame tiles to categorize
 * @param inter_anim_canonical_tiles Canonical forms of other animations' key frame tiles
 * @param external_canonical_tiles Canonical forms of all non-animation tiles in tiles.png
 * @return DuplicateInfo with indices categorized by duplicate type
 */
[[nodiscard]] DuplicateInfo categorize_duplicate_key_frame_tiles(
    const std::vector<PixelTile<IndexPixel>> &key_frame_tiles,
    const std::set<PixelTile<IndexPixel>> &inter_anim_canonical_tiles,
    const std::set<PixelTile<IndexPixel>> &external_canonical_tiles)
{
    DuplicateInfo info;

    // Map from canonical base tile to first index seen
    std::map<PixelTile<IndexPixel>, std::size_t> seen;

    for (std::size_t i = 0; i < key_frame_tiles.size(); ++i) {
        const CanonicalPixelTile canonical{key_frame_tiles[i]};
        const PixelTile<IndexPixel> &base = canonical;

        // Check inter-animation before cross-range (more specific category wins)
        if (inter_anim_canonical_tiles.contains(base)) {
            info.inter_anim_indices.push_back(i);
        }
        else if (external_canonical_tiles.contains(base)) {
            info.cross_range_indices.push_back(i);
        }

        auto [it, inserted] = seen.emplace(base, i);
        if (!inserted) {
            info.intra_anim_pairs.emplace_back(it->second, i);
        }
    }

    return info;
}

/**
 * @brief Backports mangle records to the tiles.png image in the Porymap component.
 *
 * @param component The Porymap component containing tiles_png to modify
 * @param base_tile_offset The tile offset of the animation in tiles.png
 * @param records The mangle records describing pixel changes
 */
void backport_mangles_to_tiles_png(
    PorymapTilesetComponent &component, std::size_t base_tile_offset, const std::set<TileMangleRecord> &records)
{
    Image<IndexPixel> tiles_img = component.tiles_png();
    constexpr std::size_t tiles_per_row = metatile::metatiles_per_row * metatile::tiles_per_side;

    /*
     * Mangle records are non-overlapping: each targets a distinct tile_index (guaranteed by mangle_duplicates).
     * Sequential application is therefore safe and order-independent, producing results consistent with the in-memory
     * key frame tiles that were mangled during decompilation.
     */
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

namespace porytiles2 {

ChainableResult<Animation<Rgba32>> AnimDecompiler::decompile_animation(
    const std::string &tileset_name,
    const Animation<IndexPixel> &anim,
    const std::set<PixelTile<IndexPixel>> &inter_anim_canonical_tiles,
    PorymapTilesetComponent &porymap_component) const
{
    /*
     * TODO: PorymapTilesetComponent &porymap_component isn't 'const' here because we need to edit tiles.png to support
     * key frame mangle backporting. However, I don't love this. The calling PrimaryTilesetDecompiler passes
     * triple-layerized metatile entries into this function via the component, and then restores them after. It works by
     * accident because the AnimDecompiler doesn't modify metatile tilemap entries. But that's not clear just by looking
     * at the signature. We should probably break this out so that we can pass const stuff as const, and non-const stuff
     * as non-const.
     */

    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_anim_pal_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_anim_key_frame_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(
        config_, global_anim_multi_pal_subtile_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_frame_linking, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, per_anim_overrides, tileset_name, Animation<Rgba32>);

    // Read data from porymap_component
    const auto &pals = porymap_component.pals();
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

    /*
     * Build per-subtile palette resolution strategies using a three-tier cascade:
     *   1. Per-tile (per_tile_pal_resolution_strategies[i]): most specific
     *   2. Per-anim (pal_resolution_strategy): middle tier
     *   3. Global (global_anim_pal_resolution_strategy): least specific fallback
     */
    std::vector<ConfigValue<AnimPalResolutionStrategy>> per_subtile_strategies;
    per_subtile_strategies.reserve(tile_count);

    const auto &configs_map = per_anim_overrides.value();
    const auto anim_cfg_it = configs_map.find(anim.name());
    const PerAnimOverride *anim_cfg_ptr = (anim_cfg_it != configs_map.end()) ? &anim_cfg_it->second : nullptr;

    if (anim_cfg_ptr != nullptr) {
        const PerAnimOverride &anim_cfg = *anim_cfg_ptr;

        // Determine the "effective default" for this animation: per-anim if set, otherwise global
        const ConfigValue<AnimPalResolutionStrategy> effective_default =
            anim_cfg.pal_resolution_strategy.has_value() ? per_anim_overrides.derive(anim_cfg.pal_resolution_strategy)
                                                         : global_anim_pal_resolution_strategy;

        if (!anim_cfg.per_tile_pal_resolution_strategies.empty()) {
            if (anim_cfg.per_tile_pal_resolution_strategies.size() != tile_count) {
                return FormattableError{
                    std::vector<std::string>{
                        "Animation '{}' config 'per_tile_palette_resolution_strategies' has '{}' entries, but "
                        "animation has '{}' subtiles.",
                        "The per_tile_palette_resolution_strategies list must have exactly one entry per subtile."},
                    std::vector<std::vector<FormatParam>>{
                        {FormatParam{anim.name(), Style::bold},
                         FormatParam{anim_cfg.per_tile_pal_resolution_strategies.size(), Style::bold},
                         FormatParam{tile_count, Style::bold}},
                        {}}};
            }
            for (std::size_t i = 0; i < tile_count; ++i) {
                if (anim_cfg.per_tile_pal_resolution_strategies[i].has_value()) {
                    per_subtile_strategies.push_back(
                        per_anim_overrides.derive(anim_cfg.per_tile_pal_resolution_strategies[i]));
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
            per_subtile_strategies.push_back(global_anim_pal_resolution_strategy);
        }
    }

    /*
     * Compute the effective multi-pal subtile resolution strategy: per-anim override wins, otherwise global fallback.
     */
    const ConfigValue<AnimMultiPalSubtileResolutionStrategy> effective_multi_pal_strategy =
        (anim_cfg_ptr != nullptr && anim_cfg_ptr->multi_pal_subtile_resolution_strategy.has_value())
            ? per_anim_overrides.derive(anim_cfg_ptr->multi_pal_subtile_resolution_strategy)
            : global_anim_multi_pal_subtile_resolution_strategy;

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

    /*
     * Manual mode: extract override entries from metatiles_bin and skip key frame generation.
     * Regular frames are still decompiled using the per-subtile palette resolution cascade.
     */
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
                            entry.pal_index(),
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
            manual_pal_indices,
            find_pals_for_anim_tiles(
                anim.name(),
                tile_offset,
                tile_count,
                metatiles_bin,
                per_subtile_strategies,
                effective_multi_pal_strategy,
                anim,
                pals,
                tiles_png,
                extrinsic_transparency,
                *diag_,
                *pal_printer_,
                *tile_printer_),
            Animation<Rgba32>,
            diag_->formatter().format(
                "Failed to find palette for animation '{}'.", FormatParam{anim.name(), Style::bold}));

        for (const auto &frame : anim.frames_values()) {
            std::vector<PixelTile<Rgba32>> rgba_tiles;
            rgba_tiles.reserve(frame.tiles().size());
            for (std::size_t i = 0; i < frame.tiles().size(); ++i) {
                rgba_tiles.push_back(color_tile_from_index_tile(
                    frame.tiles()[i], pals.at(manual_pal_indices[i]), extrinsic_transparency.value()));
            }
            AnimFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
            result.put_frame(frame.frame_name(), std::move(rgba_frame));
        }

        return result;
    }

    // Recover per-subtile palette indices
    PT_TRY_ASSIGN_CHAIN_ERR(
        pal_indices,
        find_pals_for_anim_tiles(
            anim.name(),
            tile_offset,
            tile_count,
            metatiles_bin,
            per_subtile_strategies,
            effective_multi_pal_strategy,
            anim,
            pals,
            tiles_png,
            extrinsic_transparency,
            *diag_,
            *pal_printer_,
            *tile_printer_),
        Animation<Rgba32>,
        diag_->formatter().format("Failed to find palette for animation '{}'.", FormatParam{anim.name(), Style::bold}));

    // Build per-tile palette pointer vector for the mangler and conversion
    std::vector<const Palette<Rgba32, pal::max_size> *> pal_ptrs;
    pal_ptrs.reserve(pal_indices.size());
    for (std::size_t idx : pal_indices) {
        pal_ptrs.push_back(&pals.at(idx));
    }

    // Extract key frame tiles from tiles.png
    std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
        extract_tiles_from_image(tiles_png, tile_offset, tile_count);

    /*
     * Build canonical tile set for all tiles in tiles.png OUTSIDE the current animation's key frame range, then merge
     * in inter-animation canonical tiles. The combined set is used by the mangler to avoid producing canonical
     * collisions with any already-used tile. For duplicate *categorization* (inter-anim vs cross-range), the separate
     * inter_anim_canonical_tiles parameter is checked first so that tiles appearing in both sets are correctly reported
     * as inter-animation duplicates rather than cross-range duplicates.
     */
    const std::size_t total_tiles =
        (tiles_png.height() / tile::side_length_pix) * (tiles_png.width() / tile::side_length_pix);
    std::set<PixelTile<IndexPixel>> existing_canonical_tiles;
    for (std::size_t i = 0; i < total_tiles; ++i) {
        if (i >= tile_offset && i < tile_offset + tile_count) {
            continue;
        }
        const CanonicalPixelTile canonical{extract_single_tile(tiles_png, i)};
        const PixelTile<IndexPixel> &base = canonical;
        existing_canonical_tiles.insert(base);
    }

    existing_canonical_tiles.insert(inter_anim_canonical_tiles.begin(), inter_anim_canonical_tiles.end());

    /*
     * Compute the effective key frame resolution strategy: per-anim override wins, otherwise global fallback.
     */
    const ConfigValue<AnimKeyFrameResolutionStrategy> effective_key_frame_strategy =
        (anim_cfg_ptr != nullptr && anim_cfg_ptr->key_frame_resolution_strategy.has_value())
            ? per_anim_overrides.derive(anim_cfg_ptr->key_frame_resolution_strategy)
            : global_anim_key_frame_resolution_strategy;

    /*
     * Check for duplicate key frame tiles using canonical (flip-equivalent) comparison. Detects:
     * - Inter-animation duplicates: animation tile matches another animation's key frame tile
     * - Cross-range duplicates: animation tile matches a non-animation tile in tiles.png
     * - Intra-animation duplicates: two animation tiles are flip-equivalent to each other
     */
    if (has_duplicate_key_frame_tiles(key_frame_index_tiles, inter_anim_canonical_tiles, existing_canonical_tiles)) {
        switch (effective_key_frame_strategy.value()) {
        case AnimKeyFrameResolutionStrategy::error: {
            const auto dup_info = categorize_duplicate_key_frame_tiles(
                key_frame_index_tiles, inter_anim_canonical_tiles, existing_canonical_tiles);
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag_->formatter().format(
                "Animation '{}' has duplicate key frame tiles (flip-equivalent tiles are considered duplicates):",
                FormatParam{anim.name(), Style::bold}));
            for (const auto &idx : dup_info.inter_anim_indices) {
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Tile {} is flip-equivalent to another animation's key frame tile.",
                    FormatParam{idx, Style::bold}));
            }
            for (const auto &idx : dup_info.cross_range_indices) {
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Tile {} is flip-equivalent to a non-animation tile in tiles.png.",
                    FormatParam{idx, Style::bold}));
            }
            for (const auto &[i, j] : dup_info.intra_anim_pairs) {
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Tile {} and tile {} are flip-equivalent.",
                    FormatParam{i, Style::bold},
                    FormatParam{j, Style::bold}));
            }
            err_msg.emplace_back("");
            err_msg.emplace_back("Consider using 'mangle' strategy to auto-resolve.");
            err_msg.append_range(format_config_note_with_separator(diag_->formatter(), effective_key_frame_strategy));
            return FormattableError{err_msg};
        }

        case AnimKeyFrameResolutionStrategy::warning: {
            // TODO: impl
            panic("warning not yet implemented");
        }

        case AnimKeyFrameResolutionStrategy::mangle: {
            AnimKeyFrameMangler mangler{diag_, tile_printer_};
            PT_TRY_ASSIGN_CHAIN_ERR(
                mangle_result,
                mangler.mangle_duplicates(
                    anim.name(),
                    std::move(key_frame_index_tiles),
                    pal_ptrs,
                    extrinsic_transparency.value(),
                    existing_canonical_tiles),
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
            key_frame_index_tiles[i], pals.at(pal_indices[i]), extrinsic_transparency.value()));
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
                color_tile_from_index_tile(frame.tiles()[i], pals.at(pal_indices[i]), extrinsic_transparency.value()));
        }

        AnimFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
        result.put_frame(frame.frame_name(), std::move(rgba_frame));
    }

    return result;
}

} // namespace porytiles2
