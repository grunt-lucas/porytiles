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
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy_overrides.hpp"
#include "porytiles2/domain/models/anim_frame.hpp"
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
        std::ranges::copy(pal_printer.print_rgba_pal(representative_pal), std::back_inserter(err_msg));
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
        std::ranges::copy(
            pal_printer.print_rgba_pal_with_highlights(representative_pal, extrinsic_transparency_slots),
            std::back_inserter(err_msg));
        std::ranges::copy(
            format_config_note_with_separator(diag.formatter(), extrinsic_transparency), std::back_inserter(err_msg));
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
            std::ranges::copy(pal_printer.print_rgba_pal(representative_pal), std::back_inserter(err_msg));
            err_msg.emplace_back("");
            err_msg.emplace_back(diag.formatter().format("Frame '{}' palette:", FormatParam{frame_name, Style::bold}));
            std::ranges::copy(pal_printer.print_rgba_pal(frame_pal), std::back_inserter(err_msg));
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
            std::ranges::copy(pal_printer.print_rgba_pal(tileset_pals[pal_idx]), std::back_inserter(remark_lines));
            diag.remark("animation-palette-resolution-strategy", remark_lines);
            return pal_idx;
        }
    }

    std::vector<std::string> err_msg{};
    err_msg.emplace_back(diag.formatter().format(
        "Failed to find matching palette for internal palette of representative frame '{}'.",
        FormatParam{representative_frame.frame_name(), Style::bold}));
    err_msg.emplace_back("");
    std::ranges::copy(pal_printer.print_rgba_pal(representative_pal), std::back_inserter(err_msg));
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

[[nodiscard]] ChainableResult<std::vector<std::size_t>> find_pals_for_anim_tiles(
    const std::string &anim_name,
    std::size_t tile_offset,
    std::size_t tile_count,
    std::span<const TilemapEntry> metatiles_bin,
    const ConfigValue<AnimPalResolutionStrategy> &strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    const Image<IndexPixel> &tiles_png,
    const ConfigValue<Rgba32> &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer,
    const TilePrinter &tile_printer)
{
    // Check pal_N strategies first — return uniform palette index for all subtiles
    const auto explicit_pal = extract_pal_index(strategy.value());
    if (explicit_pal.has_value()) {
        diag.remark(
            "animation-palette-resolution-strategy",
            {diag.formatter().format(
                "Animation '{}' using explicit palette '{}'.",
                FormatParam{anim_name, Style::bold},
                FormatParam{pal_filename(*explicit_pal), Style::bold})});
        diag.remark_note("animation-palette-resolution-strategy", format_config_note(diag.formatter(), strategy));
        return std::vector<std::size_t>(tile_count, *explicit_pal);
    }

    switch (strategy.value()) {
    case AnimPalResolutionStrategy::scan_local_metatiles: {
        std::vector<std::size_t> per_tile_pals(tile_count);
        std::vector<bool> resolved(tile_count, false);

        /*
         * First pass: scan each subtile independently to find its palette index from metatile references. Some
         * subtiles may not be directly referenced in metatiles (e.g. they're only visible during animation
         * playback or are referenced by paired tilesets). These are marked unresolved for the second pass.
         */
        for (std::size_t i = 0; i < tile_count; ++i) {
            const std::size_t tile_index = tile_offset + i;
            std::set<std::size_t> found_for_subtile{};

            for (const auto &entry : metatiles_bin) {
                if (entry.tile_index() == tile_index) {
                    found_for_subtile.insert(entry.pal_index());
                }
            }

            if (found_for_subtile.empty()) {
                // TODO: does this make sense anymore?
                // Mark as unresolved — will be checked after the first pass
                continue;
            }

            if (found_for_subtile.size() > 1) {
                /*
                 * A single tile index can be referenced by multiple metatile entries with different palette indices.
                 * This is valid GBA behavior — the hardware selects palette per metatile entry, not per tile.
                 *
                 * TODO: ANIM: a more sophisticated approach could support multi-palette variants per subtile. For now,
                 * we treat this as an error because picking one palette arbitrarily would produce incorrect RGBA output
                 * in the layer PNGs, breaking recompilation (the other palette version would be lost).
                 *
                 * We need to figure out a better way to handle this.
                 */
                std::string pal_list;
                for (const auto &pal_idx : found_for_subtile) {
                    if (!pal_list.empty()) {
                        pal_list += ", ";
                    }
                    pal_list += pal_filename(pal_idx);
                }

                std::vector<std::string> err_msg;
                err_msg.push_back(diag.formatter().format(
                    "Animation '{}' subtile at tile index '{}' is referenced with multiple palettes: {}.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{tile_index, Style::bold},
                    FormatParam{pal_list, Style::bold}));
                err_msg.emplace_back(
                    "Picking one palette arbitrarily would produce incorrect RGBA output in the layer PNGs.");

                // Show the tile rendered under each conflicting palette for visual comparison
                const PixelTile<IndexPixel> index_tile = extract_single_tile(tiles_png, tile_index);
                err_msg.emplace_back("");
                for (const auto &pal_idx : found_for_subtile) {
                    const PixelTile<Rgba32> rgba_tile =
                        color_tile_from_index_tile(index_tile, pals.at(pal_idx), extrinsic_transparency.value());
                    err_msg.push_back(diag.formatter().format(
                        "Tile under palette '{}':", FormatParam{pal_filename(pal_idx), Style::bold}));
                    std::ranges::copy(
                        tile_printer.print_tile(rgba_tile, extrinsic_transparency.value()),
                        std::back_inserter(err_msg));
                }

                err_msg.emplace_back("");
                err_msg.emplace_back(
                    "Consider using an explicit palette resolution strategy (e.g. 'palette-00') to resolve the "
                    "ambiguity.");
                std::ranges::copy(
                    format_config_note_with_separator(diag.formatter(), strategy), std::back_inserter(err_msg));
                return FormattableError{err_msg};
            }

            per_tile_pals[i] = *found_for_subtile.begin();
            resolved[i] = true;
        }

        /*
         * After the first pass, check for unresolved subtiles. If some subtiles are not referenced in any local
         * metatile, we cannot determine their palette.
         *
         * TODO: ANIM: could inherit palette from the first resolved sibling subtile, since vanilla games sometimes
         * have animation subtiles that only appear during playback and aren't directly in any metatile entry.
         */
        std::vector<std::size_t> unresolved_indices;
        for (std::size_t i = 0; i < tile_count; ++i) {
            if (!resolved[i]) {
                unresolved_indices.push_back(tile_offset + i);
            }
        }

        if (unresolved_indices.size() == tile_count) {
            // No subtile in the entire range is referenced — error
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag.formatter().format(
                "Animation '{}' tile index range [{},{}] is not referenced in local metatiles.",
                FormatParam{anim_name, Style::bold},
                FormatParam{tile_offset, Style::bold},
                FormatParam{tile_offset + tile_count - 1, Style::bold}));
            err_msg.emplace_back(
                "Consider using a different palette resolution strategy (e.g. 'palette-00', "
                "'internal-png-palette', etc.).");
            std::ranges::copy(
                format_config_note_with_separator(diag.formatter(), strategy), std::back_inserter(err_msg));
            return FormattableError{err_msg};
        }

        if (!unresolved_indices.empty()) {
            std::string index_list;
            for (const auto &idx : unresolved_indices) {
                if (!index_list.empty()) {
                    index_list += ", ";
                }
                index_list += std::to_string(idx);
            }
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag.formatter().format(
                "Animation '{}' subtile(s) at tile index(es) {} not referenced in local metatiles.",
                FormatParam{anim_name, Style::bold},
                FormatParam{index_list, Style::bold}));
            err_msg.emplace_back(
                "Consider using an explicit palette resolution strategy (e.g. 'palette-00') for this animation.");
            std::ranges::copy(
                format_config_note_with_separator(diag.formatter(), strategy), std::back_inserter(err_msg));
            return FormattableError{err_msg};
        }

        // Emit a remark if multiple distinct palettes are used across subtiles
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

        return per_tile_pals;
    }

    case AnimPalResolutionStrategy::internal_png_pal: {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag.formatter().format(
            "Palette resolution strategy '{}' failed.",
            FormatParam{to_string(AnimPalResolutionStrategy::internal_png_pal), Style::bold}));
        std::ranges::copy(format_config_note_with_separator(diag.formatter(), strategy), std::back_inserter(err_msg));
        PT_TRY_ASSIGN_CHAIN_ERR(
            match,
            internal_png_pal_strategy(anim, pals, extrinsic_transparency, diag, pal_printer),
            err_msg,
            std::vector<std::size_t>);
        return std::vector<std::size_t>(tile_count, match);
    }

    case AnimPalResolutionStrategy::scan_all_tilesets:
        panic("scan_all_tilesets not yet implemented");

    default:
        panic("unhandled AnimPalResolutionStrategy value");
    }
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
    // Unwrap config values
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, global_anim_pal_resolution_strategy, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, anim_pal_resolution_strategy_overrides, tileset_name, Animation<Rgba32>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, anim_key_frame_resolution_strategy, tileset_name, Animation<Rgba32>);

    // Resolve per-animation palette strategy override
    ConfigValue<AnimPalResolutionStrategy> effective_pal_strategy = global_anim_pal_resolution_strategy;
    const auto &overrides_map = anim_pal_resolution_strategy_overrides.value();
    if (auto it = overrides_map.find(anim.name()); it != overrides_map.end()) {
        /*
         * TODO: this source_key may not be correct if we ever add other provider handling for Animation Palette
         * Resolution Strategy Override, it's hardcoded to the YAML format.
         */
        effective_pal_strategy = ConfigValue<AnimPalResolutionStrategy>{
            it->second,
            "Animation Palette Resolution Strategy Override (" + anim.name() + ")",
            "tileset.animations.palette_resolution_strategy_overrides." + anim.name(),
            anim_pal_resolution_strategy_overrides.source(),
            anim_pal_resolution_strategy_overrides.source_details()};
    }

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

    // Recover per-subtile palette indices by scanning metatiles for each animation tile
    PT_TRY_ASSIGN_CHAIN_ERR(
        pal_indices,
        find_pals_for_anim_tiles(
            anim.name(),
            tile_offset,
            tile_count,
            metatiles_bin,
            effective_pal_strategy,
            anim,
            pals,
            tiles_png,
            extrinsic_transparency,
            *diag_,
            *pal_printer_,
            *tile_printer_),
        diag_->formatter().format("Failed to find palette for animation '{}'.", FormatParam{anim.name(), Style::bold}),
        Animation<Rgba32>);

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
     * Check for duplicate key frame tiles using canonical (flip-equivalent) comparison. Detects:
     * - Inter-animation duplicates: animation tile matches another animation's key frame tile
     * - Cross-range duplicates: animation tile matches a non-animation tile in tiles.png
     * - Intra-animation duplicates: two animation tiles are flip-equivalent to each other
     */
    if (has_duplicate_key_frame_tiles(key_frame_index_tiles, inter_anim_canonical_tiles, existing_canonical_tiles)) {
        switch (anim_key_frame_resolution_strategy.value()) {
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
            std::ranges::copy(
                format_config_note_with_separator(diag_->formatter(), anim_key_frame_resolution_strategy),
                std::back_inserter(err_msg));
            return FormattableError{err_msg};
        }

        case AnimKeyFrameResolutionStrategy::warning: {
            // TODO: impl
            panic("warning not yet implemented");
        }

        case AnimKeyFrameResolutionStrategy::manual_override: {
            panic("manual_override not yet implemented");
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
                diag_->formatter().format(
                    "Failed to mangle duplicate key frame tiles for animation '{}'.",
                    FormatParam{anim.name(), Style::bold}),
                Animation<Rgba32>);
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
