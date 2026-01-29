#include "porytiles2/domain/services/animation_decompiler.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>
#include <set>
#include <string>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/algorithms/tile_converters.hpp"
#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_value.hpp"

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

    // PNG palette must have exactly 16 colors to match GBA palette format
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

    // Check for extrinsic transparency in non-slot-0 positions
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

    // Slot-by-slot matching (skip slot 0 which is transparency)
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

ChainableResult<std::size_t> find_pal_for_anim_tiles(
    const std::string &anim_name,
    std::size_t tile_offset,
    std::size_t tile_count,
    std::span<const TilemapEntry> metatiles_bin,
    const ConfigValue<AnimPalResolutionStrategy> &strategy,
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    const ConfigValue<Rgba32> &extrinsic_transparency,
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
            {diag.formatter().format(
                 "Animation '{}' not referenced in metatiles.", FormatParam{anim_name, Style::bold}),
             "Falling back to palette resolution strategy."});
        diag.remark_note("animation-palette-resolution-strategy", format_config_note(diag.formatter(), strategy));

        switch (strategy) {
        case AnimPalResolutionStrategy::error: {
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag.formatter().format(
                "Animation '{}' tile index range [{},{}] is not referenced in metatiles.",
                FormatParam{anim_name, Style::bold},
                FormatParam{tile_offset, Style::bold},
                FormatParam{tile_offset + tile_count - 1, Style::bold}));
            std::ranges::copy(
                format_config_note_with_separator(diag.formatter(), strategy), std::back_inserter(err_msg));
            return FormattableError{err_msg};
        }

        case AnimPalResolutionStrategy::default_pal:
            return 0;

        case AnimPalResolutionStrategy::internal_png_pal: {
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag.formatter().format(
                "Palette resolution strategy '{}' failed.",
                FormatParam{to_string(AnimPalResolutionStrategy::internal_png_pal), Style::bold}));
            std::ranges::copy(
                format_config_note_with_separator(diag.formatter(), strategy), std::back_inserter(err_msg));
            PT_TRY_ASSIGN_CHAIN_ERR(
                match,
                internal_png_pal_strategy(anim, pals, extrinsic_transparency, diag, pal_printer),
                err_msg,
                std::size_t);
            return match;
        }

        case AnimPalResolutionStrategy::full_tileset_scan:
            panic("full_tileset_scan not yet implemented");

        default:
            panic("unhandled AnimPalResolutionStrategy value");
        }
    }

    if (found_pal_indices.size() > 1) {
        /*
         * TODO: ANIM: adapt this code so that it computes a separate pal index for each subtile of the key frame.
         * Technically, advanced users could make animations where different subtiles use different palettes. None of
         * the vanilla game animations work this way, but it's possible and thus a use-case I want to support.
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

ChainableResult<Animation<Rgba32>> AnimationDecompiler::decompile_animation(
    const Animation<IndexPixel> &anim,
    const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals,
    std::span<const TilemapEntry> metatiles_bin,
    const Image<IndexPixel> &tiles_png,
    const ConfigValue<Rgba32> &extrinsic_transparency,
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
    PT_TRY_ASSIGN_CHAIN_ERR(
        pal_index,
        find_pal_for_anim_tiles(
            anim.name(),
            tile_offset,
            tile_count,
            metatiles_bin,
            strategy,
            anim,
            pals,
            extrinsic_transparency,
            *diag_,
            *pal_printer_),
        diag_->formatter().format("Failed to find palette for animation '{}'.", FormatParam{anim.name(), Style::bold}),
        Animation<Rgba32>);

    const auto &pal = pals.at(pal_index);

    // Extract key frame tiles from tiles.png
    std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
        extract_tiles_from_image(tiles_png, tile_offset, tile_count);

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
        key_frame_rgba_tiles.push_back(color_tile_from_index_tile(index_tile, pal, extrinsic_transparency.value()));
    }

    // Set the key frame on the result
    AnimationFrame key_frame{"key", std::move(key_frame_rgba_tiles)};
    result.key_frame(std::move(key_frame));

    for (const auto &frame : anim.frames_values()) {
        std::vector<PixelTile<Rgba32>> rgba_tiles;
        rgba_tiles.reserve(frame.tiles().size());

        for (const auto &index_tile : frame.tiles()) {
            rgba_tiles.push_back(color_tile_from_index_tile(index_tile, pal, extrinsic_transparency.value()));
        }

        AnimationFrame rgba_frame{frame.frame_name(), std::move(rgba_tiles)};
        result.put_frame(frame.frame_name(), std::move(rgba_frame));
    }

    return result;
}

} // namespace porytiles2
