#pragma once

#include <algorithm>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/packing/services/palette_packer.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/config/config_value.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

constexpr std::string_view diagnostic_separator = "--------";

/// @brief Format a ConfigValue into diagnostic note lines.
///
/// @details
/// Shared helper that constructs the standard format for displaying configuration values in note diagnostics. The
/// output includes a header line showing the config name and value, followed by a blank line, then the full
/// prettified configuration context.
///
/// @tparam T The underlying type of the ConfigValue
/// @param format The TextFormatter for styling
/// @param config The ConfigValue to format
/// @return Vector of formatted lines ready for note output
template <typename T>
[[nodiscard]] std::vector<std::string> format_config_note(const TextFormatter &format, const ConfigValue<T> &config)
{
    std::vector<std::string> lines{};
    lines.push_back(format.format(
        "'{}' is '{}' due to configuration:",
        FormatParam{config.canonical_name(), Style::bold},
        FormatParam{config.value(), Style::bold}));
    lines.emplace_back("");
    lines.append_range(config.prettify(format));
    return lines;
}

/// @brief Format a ConfigValue into diagnostic note lines with a separator.
///
/// @details
/// This method is the same as format_config_note, but it includes a separator section above the config note. This is
/// useful for cases where the caller is already printing some other information and wants some visual separation
/// between that info and the config printout.
///
/// @tparam T The underlying type of the ConfigValue
/// @param format The TextFormatter for styling
/// @param config The ConfigValue to format
/// @return Vector of formatted lines ready for note output
template <typename T>
[[nodiscard]] std::vector<std::string>
format_config_note_with_separator(const TextFormatter &format, const ConfigValue<T> &config)
{
    std::vector<std::string> lines{};
    lines.emplace_back("");
    lines.emplace_back(diagnostic_separator);
    lines.emplace_back("");
    lines.append_range(format_config_note(format, config));
    return lines;
}

/// @brief Builds note lines explaining the global color count limit for primary tileset compilation.
///
/// @details
/// This function creates and returns standardized note lines explaining how the global color count limit is calculated.
/// It is used in error messages when the user exceeds the maximum number of unique colors allowed in a tileset.
///
/// The generated note includes:
/// - A statement of the limit value
/// - The formula for calculating the limit (num_pals_in_primary * nontransparent_colors_per_pal)
/// - The prettified configuration value showing the source of the num_pals setting
///
/// @param format The TextFormatter for styling
/// @param color_count_limit The calculated color count limit value
/// @param num_pals The configuration value for the number of palettes in primary
/// @return Vector of formatted lines describing the color count limit configuration
[[nodiscard]] inline std::vector<std::string> build_global_color_limit_lines(
    const TextFormatter &format, std::size_t color_count_limit, const ConfigValue<std::size_t> &num_pals)
{
    std::vector<std::string> lines;
    lines.push_back(format.format("Global unique color limit is '{}'.", FormatParam{color_count_limit, Style::bold}));
    lines.emplace_back("");
    lines.emplace_back("Color limit definition:");
    lines.push_back(format.format(
        "{} * {}:",
        FormatParam{num_pals.canonical_name(), Style::bold | Style::yellow},
        FormatParam{"nontransparent_colors_per_pal", Style::bold}));
    lines.push_back(format.format(
        "{} * {} = {}",
        FormatParam{num_pals.value(), Style::bold | Style::yellow},
        FormatParam{(pal::max_size - 1), Style::bold},
        FormatParam{color_count_limit, Style::bold}));
    lines.emplace_back("");
    lines.append_range(format_config_note(format, num_pals));
    return lines;
}

/// @brief Builds note lines explaining a derived limit computed as total minus primary.
///
/// @details
/// Used in secondary tileset diagnostics where a limit is derived by subtracting a primary config value from a total
/// config value (e.g. metatile limit = num_metatiles_total - num_metatiles_in_primary). The output shows the computed
/// limit, the subtraction formula with both config canonical names and values, and prettified config notes for both
/// source config values separated by a visual divider.
///
/// @param format The TextFormatter for styling
/// @param label A human-readable label for the limit (e.g. "Metatile limit")
/// @param computed_limit The derived limit value (total - primary)
/// @param total_cfg The ConfigValue for the total count
/// @param primary_cfg The ConfigValue for the primary count
/// @return Vector of formatted lines describing the subtraction-based limit
[[nodiscard]] inline std::vector<std::string> build_subtraction_limit_lines(
    const TextFormatter &format,
    std::string_view label,
    std::size_t computed_limit,
    const ConfigValue<std::size_t> &total_cfg,
    const ConfigValue<std::size_t> &primary_cfg)
{
    std::vector<std::string> lines;
    lines.push_back(
        format.format("{} is '{}'.", FormatParam{label, Style::bold}, FormatParam{computed_limit, Style::bold}));
    lines.emplace_back("");
    lines.push_back(format.format("{} definition:", FormatParam{label, Style::bold}));
    lines.push_back(format.format(
        "{} - {}:",
        FormatParam{total_cfg.canonical_name(), Style::bold | Style::yellow},
        FormatParam{primary_cfg.canonical_name(), Style::bold | Style::yellow}));
    lines.push_back(format.format(
        "{} - {} = {}",
        FormatParam{total_cfg.value(), Style::bold},
        FormatParam{primary_cfg.value(), Style::bold},
        FormatParam{computed_limit, Style::bold}));
    lines.emplace_back("");
    lines.append_range(format_config_note(format, total_cfg));
    lines.append_range(format_config_note_with_separator(format, primary_cfg));
    return lines;
}

/// @brief Builds note lines displaying a Porymap palette with highlighted violating slots.
///
/// @details
/// This function generates and returns formatted note lines showing a Porymap palette with specific color slots
/// highlighted to indicate violations. It is used to provide visual feedback when palette-related errors occur during
/// tileset compilation, specifically for palettes originating from Porymap assets.
///
/// The generated note includes:
/// - A header identifying the Porymap palette by label and custom message
/// - The palette rendered with violating slots visually highlighted
///
/// @tparam N The size of the palette (number of color slots)
/// @param format The TextFormatter for styling
/// @param pal_printer The palette printer for rendering palette visualizations
/// @param message A custom message describing the issue with the palette
/// @param pal The palette to display
/// @param pal_label The human-readable label identifying this palette
/// @param violating_slots The indices of slots that should be highlighted as violations
/// @return Vector of formatted lines describing the palette with highlights
template <std::size_t N>
[[nodiscard]] std::vector<std::string> build_porymap_pal_highlight_lines(
    const TextFormatter &format,
    const PalettePrinter &pal_printer,
    const std::string &message,
    const Palette<Rgba32, N> &pal,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> lines;
    lines.emplace_back(
        format.format("Porymap palette '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    lines.emplace_back();
    lines.append_range(pal_printer.print_rgba_pal_with_highlights(pal, violating_slots));
    return lines;
}

/// @brief Builds note lines displaying a Porytiles palette with highlighted violating slots.
///
/// @details
/// This function generates and returns formatted note lines showing a Porytiles palette with specific color slots
/// highlighted to indicate violations. It is used to provide visual feedback when palette-related errors occur during
/// tileset compilation.
///
/// The generated note includes:
/// - A header identifying the Porytiles palette by label and custom message
/// - The palette rendered with violating slots visually highlighted
///
/// @tparam N The size of the palette (number of color slots)
/// @param format The TextFormatter for styling
/// @param pal_printer The palette printer for rendering palette visualizations
/// @param message A custom message describing the issue with the palette
/// @param pal The palette to display
/// @param pal_label The human-readable label identifying this palette
/// @param violating_slots The indices of slots that should be highlighted as violations
/// @return Vector of formatted lines describing the palette with highlights
template <std::size_t N>
[[nodiscard]] std::vector<std::string> build_porytiles_pal_highlight_lines(
    const TextFormatter &format,
    const PalettePrinter &pal_printer,
    const std::string &message,
    const Palette<Rgba32, N> &pal,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> lines;
    lines.emplace_back(
        format.format("Porytiles palette '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    lines.emplace_back();
    lines.append_range(pal_printer.print_rgba_pal_with_highlights(pal, violating_slots));
    return lines;
}

/// @brief Builds note lines displaying a palette hint with highlighted violating slots.
///
/// @details
/// This function generates and returns formatted note lines showing a palette hint with specific color slots
/// highlighted to indicate violations. Palette hints are user-provided color specifications that guide the palette
/// assignment algorithm, and this function helps diagnose issues when hints contain invalid or conflicting colors.
///
/// The generated note includes:
/// - A header identifying the palette hint by label and custom message
/// - The palette hint rendered with violating slots visually highlighted
///
/// @param format The TextFormatter for styling
/// @param pal_printer The palette printer for rendering palette visualizations
/// @param message A custom message describing the issue with the palette hint
/// @param hint The palette hint to display
/// @param pal_label The human-readable label identifying this palette hint
/// @param violating_slots The indices of slots that should be highlighted as violations
/// @return Vector of formatted lines describing the palette hint with highlights
[[nodiscard]] inline std::vector<std::string> build_pal_hint_highlight_lines(
    const TextFormatter &format,
    const PalettePrinter &pal_printer,
    const std::string &message,
    const PaletteHint &hint,
    const std::string &pal_label,
    const std::vector<std::size_t> &violating_slots)
{
    std::vector<std::string> lines;
    lines.emplace_back(
        format.format("palette hint '{}': {}:", FormatParam{pal_label, Style::bold}, FormatParam{message}));
    lines.emplace_back();
    lines.append_range(pal_printer.print_pal_hint_with_highlights(hint, violating_slots));
    return lines;
}

/// @brief Builds note lines showing ASCII art for each color version tile in a sharing group.
///
/// @details
/// For each color version tile index, emits a "Color version N (metatile header):" line followed by the ASCII art
/// representation of that tile. Used in Phase 1 and Phase 2 tile sharing diagnostics.
///
/// @param format The TextFormatter for styling
/// @param tile_printer The TilePrinter for rendering tile ASCII art
/// @param pixel_tiles The full collection of pixel tiles (indexed by tile index)
/// @param extrinsic_transparency The extrinsic transparency color
/// @param color_version_tile_indices Tile indices for each distinct color version
/// @return Vector of formatted lines showing each color version tile
[[nodiscard]] inline std::vector<std::string> build_tile_sharing_color_version_tile_lines(
    const TextFormatter &format,
    const TilePrinter &tile_printer,
    const std::vector<PixelTile<Rgba32>> &pixel_tiles,
    const Rgba32 &extrinsic_transparency,
    const std::vector<std::size_t> &color_version_tile_indices)
{
    std::vector<std::string> lines;
    for (std::size_t v = 0; v < color_version_tile_indices.size(); ++v) {
        const auto tile_index = color_version_tile_indices.at(v);
        auto [mt_index, layer, subtile] = metatile::from_tile_index(tile_index);
        lines.emplace_back(format.format(
            "Color version {} (secondary {}):",
            FormatParam{v + 1, Style::bold},
            FormatParam{metatile::message_header(format, mt_index, layer, subtile), Style::bold}));
        lines.append_range(tile_printer.print_tile(pixel_tiles.at(tile_index), extrinsic_transparency));
    }
    return lines;
}

/// @brief Builds note lines showing ASCII art for each primary tile color version in a sharing group.
///
/// @details
/// For each primary tile color version entry, emits a "Color version N (primary metatile ..., palette 'XX.pal'):" line
/// followed by the ASCII art representation of that tile. The primary metatile coordinate identifies the first slot in
/// the paired primary's triple-layerized metatiles where the underlying (tile, palette) pair was first seen. The
/// version numbering continues after the secondary color versions via @p version_offset.
///
/// @param format The TextFormatter for styling
/// @param tile_printer The TilePrinter for rendering tile ASCII art
/// @param primary_tiles The primary tiles collection (each entry is a PackingParams::PrimaryTileRef)
/// @param extrinsic_transparency The extrinsic transparency color
/// @param primary_color_version_entries Pairs of (tile_index, pal_index) for each distinct primary color version
/// @param version_offset The number of secondary color versions already displayed (for numbering continuation)
/// @return Vector of formatted lines showing each primary color version tile
[[nodiscard]] inline std::vector<std::string> build_primary_tile_color_version_lines(
    const TextFormatter &format,
    const TilePrinter &tile_printer,
    const std::vector<PackingParams::PrimaryTileRef> &primary_tiles,
    const Rgba32 &extrinsic_transparency,
    const std::vector<std::pair<std::size_t, std::size_t>> &primary_color_version_entries,
    std::size_t version_offset)
{
    std::vector<std::string> lines;
    for (std::size_t v = 0; v < primary_color_version_entries.size(); ++v) {
        const auto &[tile_index, pal_index] = primary_color_version_entries.at(v);
        const auto &primary_ref = primary_tiles.at(tile_index);
        lines.emplace_back(format.format(
            "Color version {} (primary {}, palette '{}'):",
            FormatParam{version_offset + v + 1, Style::bold},
            FormatParam{
                metatile::message_header(format, primary_ref.metatile_index, primary_ref.layer, primary_ref.subtile),
                Style::bold},
            FormatParam{pal_filename(pal_index), Style::bold}));
        lines.append_range(tile_printer.print_tile(primary_ref.tile, extrinsic_transparency));
    }
    return lines;
}

/// @brief Builds truncated tile reference lines, listing up to 8 entries comma-joined with ellipsis.
///
/// @details
/// Formats tile indices as metatile message headers joined into a single comma-separated line, up to a maximum of 8
/// displayed entries, letting terminal auto-wrap break the line as needed. If there are more than 8, appends an
/// "... and N more." line as its own element. The caller is responsible for providing any header line. This function
/// only emits the tile reference listing.
///
/// @param format The TextFormatter for styling
/// @param tile_indices The tile indices to display
/// @return Vector of formatted lines listing the tile references
[[nodiscard]] inline std::vector<std::string>
build_truncated_tile_ref_lines(const TextFormatter &format, const std::vector<std::size_t> &tile_indices)
{
    constexpr std::size_t max_displayed_refs = 8;

    std::vector<std::string> lines;
    const std::size_t display_count = std::min(tile_indices.size(), max_displayed_refs);
    std::string joined;
    for (std::size_t i = 0; i < display_count; i++) {
        if (i > 0) {
            joined += ", ";
        }
        auto [mt_index, layer, subtile] = metatile::from_tile_index(tile_indices.at(i));
        joined += "secondary " + metatile::message_header(format, mt_index, layer, subtile);
    }
    if (!joined.empty()) {
        lines.emplace_back(joined);
    }
    if (tile_indices.size() > max_displayed_refs) {
        lines.emplace_back(
            format.format("... and {} more.", FormatParam{tile_indices.size() - max_displayed_refs, Style::bold}));
    }
    return lines;
}

/// @brief Builds per-palette tile reference lines with a header and truncated listing for each palette.
///
/// @details
/// For each palette in the ordered map, emits a "Palette 'XX.pal': N tilemap entries:" header followed by a truncated
/// tile reference listing via @c build_truncated_tile_ref_lines(). Uses @c std::map for deterministic ascending
/// iteration order.
///
/// @param format The TextFormatter for styling
/// @param members_by_pal Map from palette index to the tile indices assigned to that palette
/// @return Vector of formatted lines with per-palette groupings
[[nodiscard]] inline std::vector<std::string> build_per_palette_tile_ref_lines(
    const TextFormatter &format, const std::map<std::size_t, std::vector<std::size_t>> &members_by_pal)
{
    std::vector<std::string> lines;
    for (const auto &[pal_index, tile_indices] : members_by_pal) {
        lines.emplace_back(format.format(
            "Palette '{}': '{}' tilemap entries:",
            FormatParam{pal_filename(pal_index), Style::bold},
            FormatParam{tile_indices.size(), Style::bold}));
        lines.append_range(build_truncated_tile_ref_lines(format, tile_indices));
    }
    return lines;
}

/// @brief Builds note lines showing one representative tile (ASCII art) per palette.
///
/// @details
/// For each palette in the ordered map, renders the first tile in that palette's member list as ASCII art with a
/// "Representative shape for palette 'XX.pal' (metatile header):" header. Used in Phase 3 tile sharing diagnostics.
///
/// @param format The TextFormatter for styling
/// @param tile_printer The TilePrinter for rendering tile ASCII art
/// @param pixel_tiles The full collection of pixel tiles (indexed by tile index)
/// @param extrinsic_transparency The extrinsic transparency color
/// @param members_by_pal Map from palette index to the tile indices assigned to that palette
/// @return Vector of formatted lines showing one representative tile per palette
[[nodiscard]] inline std::vector<std::string> build_representative_tile_per_palette_lines(
    const TextFormatter &format,
    const TilePrinter &tile_printer,
    const std::vector<PixelTile<Rgba32>> &pixel_tiles,
    const Rgba32 &extrinsic_transparency,
    const std::map<std::size_t, std::vector<std::size_t>> &members_by_pal)
{
    std::vector<std::string> lines;
    for (const auto &[pal_index, tile_indices] : members_by_pal) {
        const auto representative_tile_index = tile_indices.front();
        auto [mt_index, layer, subtile] = metatile::from_tile_index(representative_tile_index);
        lines.emplace_back(format.format(
            "Representative shape for palette '{}' (secondary {}):",
            FormatParam{pal_filename(pal_index), Style::bold},
            FormatParam{metatile::message_header(format, mt_index, layer, subtile), Style::bold}));
        lines.append_range(tile_printer.print_tile(pixel_tiles.at(representative_tile_index), extrinsic_transparency));
    }
    return lines;
}

/// @brief Builds note lines showing one representative primary tile (ASCII art) per palette.
///
/// @details
/// For each palette in the ordered map, renders the first primary tile in that palette's member list as ASCII art with
/// a "Representative shape for palette 'XX.pal' (primary metatile ...):" header. The primary metatile coordinate
/// identifies the first slot in the paired primary's triple-layerized metatiles where the underlying (tile, palette)
/// pair was first seen. Used in Phase 3 tile sharing diagnostics for cross-tileset members.
///
/// @param format The TextFormatter for styling
/// @param tile_printer The TilePrinter for rendering tile ASCII art
/// @param primary_tiles The primary tiles collection (each entry is a PackingParams::PrimaryTileRef)
/// @param extrinsic_transparency The extrinsic transparency color
/// @param primary_members_by_pal Map from palette index to the primary tile indices assigned to that palette
/// @return Vector of formatted lines showing one representative primary tile per palette
[[nodiscard]] inline std::vector<std::string> build_primary_representative_tile_per_palette_lines(
    const TextFormatter &format,
    const TilePrinter &tile_printer,
    const std::vector<PackingParams::PrimaryTileRef> &primary_tiles,
    const Rgba32 &extrinsic_transparency,
    const std::map<std::size_t, std::vector<std::size_t>> &primary_members_by_pal)
{
    std::vector<std::string> lines;
    for (const auto &[pal_index, tile_indices] : primary_members_by_pal) {
        const auto representative_tile_index = tile_indices.front();
        const auto &primary_ref = primary_tiles.at(representative_tile_index);
        lines.emplace_back(format.format(
            "Representative shape for palette '{}' (primary {}):",
            FormatParam{pal_filename(pal_index), Style::bold},
            FormatParam{
                metatile::message_header(format, primary_ref.metatile_index, primary_ref.layer, primary_ref.subtile),
                Style::bold}));
        lines.append_range(tile_printer.print_tile(primary_ref.tile, extrinsic_transparency));
    }
    return lines;
}

} // namespace porytiles
