#include "porytiles2/domain/services/animation_decompiler.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>
#include <set>
#include <string>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/xcut/config/config_value.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Extracts animation tiles from a tiles.png image.
 *
 * @details
 * Extracts tiles from the specified offset in tiles.png for the given tile count. This is used to get the keyframe
 * tiles for an animation given its parameters.
 *
 * @param tiles_png The tiles.png image (indexed format)
 * @param tile_offset Starting tile index in tiles.png
 * @param tile_count Number of tiles to extract
 * @return Vector of extracted tiles
 */
std::vector<PixelTile<IndexPixel>>
extract_animation_tiles(const Image<IndexPixel> &tiles_png, std::size_t tile_offset, std::size_t tile_count)
{
    std::vector<PixelTile<IndexPixel>> result;
    result.reserve(tile_count);

    // tiles.png is 128 pixels wide (16 tiles per row)
    constexpr std::size_t tiles_per_row = 16;

    const std::size_t img_height = tiles_png.height();
    const std::size_t total_tiles = (tiles_png.width() / tile::side_length_pix) * (img_height / tile::side_length_pix);

    if (tile_offset + tile_count > total_tiles) {
        panic("tile_offset + tile_count exceeds tiles in tiles.png");
    }

    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_idx = tile_offset + i;
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;

        const std::size_t pixel_x = tile_col * tile::side_length_pix;
        const std::size_t pixel_y = tile_row * tile::side_length_pix;

        PixelTile<IndexPixel> tile;
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                tile.set(row, col, tiles_png.at(pixel_y + row, pixel_x + col));
            }
        }

        result.push_back(std::move(tile));
    }

    return result;
}

/**
 * @brief Matches a PNG internal palette against tileset palettes using exact slot-by-slot comparison.
 *
 * @details
 * When Porymap saves an indexed PNG, pixel indices directly correspond to palette slot positions. Therefore, the
 * internal PNG palette must be an exact slot-by-slot copy of the tileset palette for the indices to work correctly.
 * This function verifies that constraint by comparing each slot (except slot 0, which is always transparency).
 *
 * The function performs the following checks:
 * 1. PNG palette must have exactly 16 colors (pal::max_size)
 * 2. Extrinsic transparency color must not appear in slots 1-15 (warning emitted if found)
 * 3. Each non-transparent color in slots 1-15 must exactly match the corresponding tileset palette slot
 *
 * Intrinsically transparent colors (alpha == 0) are skipped during comparison since they represent unused slots.
 *
 * @param png_pal The PNG file's internal palette
 * @param tileset_pals The tileset's palettes to match against
 * @param extrinsic_transparency The RGBA color representing transparency
 * @param diag The diagnostics interface for emitting warnings and remarks
 * @param pal_printer The palette printer for formatting palette output
 * @return The matching palette index, or std::nullopt if no match found
 */
[[nodiscard]] std::optional<std::size_t> find_palette_index_from_png_palette(
    const Palette<Rgba32> &png_pal,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &tileset_pals,
    const Rgba32 &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer)
{
    // PNG palette must have exactly 16 colors to match GBA palette format
    if (png_pal.size() != pal::max_size) {
        // TODO: warn here, we'll end up falling back to default_pal behavior
        return std::nullopt;
    }

    // Check for extrinsic transparency in non-slot-0 positions
    std::vector<std::size_t> extrinsic_transparency_slots;
    for (std::size_t slot = 1; slot < pal::max_size; ++slot) {
        const Rgba32 &color = png_pal.at(slot);
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
        std::vector<std::string> warning_lines;
        warning_lines.emplace_back(diag.formatter().format(
            "PNG palette contains extrinsic transparency color '{}' in non-zero slot(s): {}",
            FormatParam{extrinsic_transparency.to_jasc_str(), Style::bold},
            FormatParam{slot_list, Style::bold}));
        warning_lines.emplace_back("");
        warning_lines.emplace_back("The extrinsic transparency color should only appear in slot 0.");
        warning_lines.emplace_back("Either correct the PNG palette or change the extrinsic transparency setting.");
        warning_lines.emplace_back("");
        std::ranges::copy(
            pal_printer.print_rgba_pal_with_highlights(png_pal, extrinsic_transparency_slots),
            std::back_inserter(warning_lines));
        diag.warning("animation-palette-extrinsic-transparency", warning_lines);
        return std::nullopt;
    }

    // Slot-by-slot matching (skip slot 0 which is transparency)
    for (std::size_t pal_idx = 0; pal_idx < tileset_pals.size(); ++pal_idx) {
        bool matches = true;
        for (std::size_t slot = 1; slot < pal::max_size; ++slot) {
            const Rgba32 &png_pal_color = png_pal.at(slot);

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
            // TODO: more context here, it prints and user has no idea what it's talking about
            std::vector<std::string> remark_lines;
            remark_lines.emplace_back(diag.formatter().format(
                "PNG internal palette matched tileset palette {}", FormatParam{pal_idx, Style::bold}));
            remark_lines.emplace_back("");
            std::ranges::copy(pal_printer.print_rgba_palette(tileset_pals[pal_idx]), std::back_inserter(remark_lines));
            diag.remark("animation-palette-match", remark_lines);
            return pal_idx;
        }
    }

    return std::nullopt;
}

/**
 * @brief Searches animation frames for one with an internal palette and matches it against tileset palettes.
 *
 * @details
 * Iterates through all frames of the animation, looking for frames that have an internal PNG palette set. When found,
 * attempts to match that palette against the tileset palettes using find_palette_index_from_png_palette().
 *
 * @param anim The animation to search
 * @param tileset_pals The tileset's palettes to match against
 * @param extrinsic_transparency The RGBA color representing transparency
 * @param diag The diagnostics interface for emitting warnings and remarks
 * @param pal_printer The palette printer for formatting palette output
 * @return The matching palette index, or std::nullopt if no frame has a matching palette
 */
[[nodiscard]] std::optional<std::size_t> find_palette_from_animation_frames(
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &tileset_pals,
    const Rgba32 &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer)
{
    for (const auto &frame : anim.frames_values()) {
        if (frame.has_palette()) {
            auto match = find_palette_index_from_png_palette(
                frame.palette(), tileset_pals, extrinsic_transparency, diag, pal_printer);
            if (match.has_value()) {
                return match;
            }
        }
    }
    return std::nullopt;
}

/**
 * @brief Finds the palette index for animation tiles by scanning metatile entries.
 *
 * @details
 * Scans all metatile entries to find which palette indices are used when referencing tiles in the animation's
 * tile range (from tile_offset to tile_offset + tile_count - 1). All animation tiles must use the same palette.
 *
 * If no metatile references are found, the behavior depends on the strategy parameter:
 * - default_pal: Returns palette 0
 * - internal_png_palette: Attempts to match frame PNG internal palettes against tileset palettes
 * - full_tileset_scan: Not yet implemented (panics)
 *
 * @param anim_name The name of the anim
 * @param tile_offset Starting tile index for the animation
 * @param tile_count Number of tiles in the animation
 * @param metatiles_bin The metatile entries to scan
 * @param strategy The strategy for resolving palette when no metatile reference is found
 * @param anim The animation (used for internal_png_palette strategy)
 * @param pals The tileset palettes (used for internal_png_palette strategy)
 * @param extrinsic_transparency The RGBA color representing transparency
 * @param diag The diagnostics interface for emitting warnings and remarks
 * @param pal_printer The palette printer for formatting palette output
 * @pre tile_count must be greater than 0
 * @return The palette index used by all animation tiles
 */
std::size_t find_palette_for_animation_tiles(
    const std::string &anim_name,
    std::size_t tile_offset,
    std::size_t tile_count,
    std::span<const TilemapEntry> metatiles_bin,
    const ConfigValue<AnimPalResolutionStrategy> &strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    const Rgba32 &extrinsic_transparency,
    const UserDiagnostics &diag,
    const PalettePrinter &pal_printer)
{
    std::set<std::size_t> found_pal_indices{};

    // Scan all tiles in the animation range
    for (std::size_t i = 0; i < tile_count; ++i) {
        const std::size_t tile_index = tile_offset + i;
        for (const auto &entry : metatiles_bin) {
            if (entry.tile_index() == tile_index) {
                found_pal_indices.insert(entry.pal_index());
            }
        }
    }

    if (found_pal_indices.empty()) {
        diag.remark(
            "animation-palette-resolution-strategy",
            diag.formatter().format(
                "animation '{}' not referenced in any metatiles", FormatParam{anim_name, Style::bold}));
        diag.remark_note("animation-palette-resolution-strategy", format_config_note(diag.formatter(), strategy));
        switch (strategy) {
        case AnimPalResolutionStrategy::default_pal:
            return 0;

        case AnimPalResolutionStrategy::internal_png_palette: {
            const auto match =
                find_palette_from_animation_frames(anim, pals, extrinsic_transparency, diag, pal_printer);
            if (match.has_value()) {
                return match.value();
            }
            // Warn that we fell back despite the internal_png_palette config
            diag.warning(
                "animation-palette-fallback",
                {diag.formatter().format(
                     "Animation '{}' configured to use internal PNG palette, but no match found",
                     FormatParam{anim_name, Style::bold}),
                 "",
                 "Falling back to default palette 0."});
            return 0;
        }

        case AnimPalResolutionStrategy::full_tileset_scan:
            panic("full_tileset_scan not yet implemented");
        default:
            panic("unhandled AnimPalResolutionStrategy value");
        }
    }

    if (found_pal_indices.size() > 1) {
        /*
         * TODO: ANIM: handle this without panicking, see comment in main decompile_animation function
         */
        std::string pal_list;
        for (const auto &pal_idx : found_pal_indices) {
            if (!pal_list.empty()) {
                pal_list += ", ";
            }
            pal_list += std::to_string(pal_idx);
        }
        panic(
            "animation '" + anim_name + "' tiles in range [" + std::to_string(tile_offset) + ", " +
            std::to_string(tile_offset + (tile_count - 1)) + "] use multiple palette indices: " + pal_list);
    }

    return *found_pal_indices.begin();
}

} // namespace

namespace porytiles2 {

Animation<Rgba32> AnimationDecompiler::decompile_animation(
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    std::span<const TilemapEntry> metatiles_bin,
    const Image<IndexPixel> &tiles_png,
    const Rgba32 &extrinsic_transparency,
    const ConfigValue<AnimPalResolutionStrategy> &strategy) const
{
    Animation<Rgba32> result{anim.name()};
    result.params(anim.params());

    // Get the tile offset from animation params to determine which tile index to look for in metatiles
    const std::size_t tile_offset = anim.params().tile_offset();
    const std::size_t tile_count = anim.params().tile_count();

    /*
     * TODO: ANIM: adapt this code so that it computes a separate pal index for each subtile of the key frame.
     * Technically, advanced users could make animations where different subtiles use different palettes. None of the
     * vanilla game animations work this way, but it's possible and thus a use-case I want to support.
     */
    // Recover the palette index by scanning metatiles for all animation tiles
    const std::size_t pal_index = find_palette_for_animation_tiles(
        anim.name(),
        tile_offset,
        tile_count,
        metatiles_bin,
        strategy,
        anim,
        pals,
        extrinsic_transparency,
        *diag_,
        *pal_printer_);

    const auto &pal = pals.at(pal_index);

    // Extract key frame tiles from tiles.png
    std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
        extract_animation_tiles(tiles_png, tile_offset, tile_count);

    // Check for duplicate tiles within the key frame
    for (std::size_t i = 0; i < key_frame_index_tiles.size(); ++i) {
        for (std::size_t j = i + 1; j < key_frame_index_tiles.size(); ++j) {
            if (key_frame_index_tiles[i] == key_frame_index_tiles[j]) {
                // TODO: ANIM: handle this properly
                std::cerr << std::endl;
                std::cerr << "---------------------------------" << std::endl;
                std::cerr << "|            TODO               |" << std::endl;
                std::cerr << "---------------------------------" << std::endl;
                std::cerr << "Animation '" << anim.name() << "' has duplicate key frame tiles at indices:" << std::endl;
                std::cerr << " - " << std::to_string(i) << std::endl;
                std::cerr << " - " << std::to_string(j) << std::endl;
            }
        }
    }

    // Decompile key frame tiles to Rgba32
    std::vector<PixelTile<Rgba32>> key_frame_rgba_tiles;
    key_frame_rgba_tiles.reserve(key_frame_index_tiles.size());
    for (const auto &index_tile : key_frame_index_tiles) {
        key_frame_rgba_tiles.push_back(color_tile_from_index_tile(index_tile, pal, extrinsic_transparency));
    }

    // Set the key frame on the result
    AnimationFrame key_frame{"key", std::move(key_frame_rgba_tiles)};
    result.key_frame(std::move(key_frame));

    for (const auto &frame : anim.frames_values()) {
        std::vector<PixelTile<Rgba32>> rgba_tiles;
        rgba_tiles.reserve(frame.tiles().size());

        for (const auto &index_tile : frame.tiles()) {
            rgba_tiles.push_back(color_tile_from_index_tile(index_tile, pal, extrinsic_transparency));
        }

        AnimationFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
        result.put_frame(frame.frame_name(), std::move(rgba_frame));
    }

    return result;
}

} // namespace porytiles2
