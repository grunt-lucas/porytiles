#include "porytiles/app/use_cases/inspect_tileset_colors.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/algorithms/color_search.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/services/layer_image_metatileizer.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles;

/// @brief Describes the query for the summary line, e.g. "color '12 34 56' (per-channel tolerance 3)".
std::string describe_query(const TextFormatter &format, const ColorMatcher &matcher)
{
    std::string text = format.format("color '{}'", FormatParam{matcher.target().to_jasc_str(), Style::bold});
    const ColorTolerance &tolerance = matcher.tolerance();
    switch (tolerance.rule()) {
    case ColorTolerance::Rule::per_channel:
        if (tolerance.steps() > 0) {
            text += format.format(" (per-channel tolerance {})", FormatParam{tolerance.steps(), Style::bold});
        }
        break;
    case ColorTolerance::Rule::gba:
        text += format.format(
            " (or any color the GBA displays as '{}')",
            FormatParam{matcher.target().quantize_to_gba().to_jasc_str(), Style::bold});
        break;
    }
    return text;
}

/// @brief The header line for a set of multi-member groups, stating the rule that formed them.
std::string describe_groups(const TextFormatter &format, std::size_t cluster_count, const ColorTolerance &tolerance)
{
    switch (tolerance.rule()) {
    case ColorTolerance::Rule::per_channel:
        return format.format(
            "Found {} group(s) of similar colors (each of R, G, and B within {} of the group's most common color).",
            FormatParam{cluster_count, Style::bold},
            FormatParam{tolerance.steps(), Style::bold});
    case ColorTolerance::Rule::gba:
        return format.format(
            "Found {} group(s) of colors the GBA displays as one color (each of R, G, and B divided by 8 gives the "
            "same 5-bit value).",
            FormatParam{cluster_count, Style::bold});
    }
    return {};
}

/// @brief The label for the colors that ended up in no multi-member group.
std::string describe_singletons(const TextFormatter &format, std::size_t count, const ColorTolerance &tolerance)
{
    switch (tolerance.rule()) {
    case ColorTolerance::Rule::per_channel:
        return format.format("{} color(s) with no similar neighbor:", FormatParam{count, Style::bold});
    case ColorTolerance::Rule::gba:
        return format.format("{} color(s) the GBA displays as a distinct color:", FormatParam{count, Style::bold});
    }
    return {};
}

/// @brief Prefixes every line with two spaces so a block reads as nested under its header.
void append_indented(std::vector<std::string> &lines, const std::vector<std::string> &block)
{
    for (const auto &line : block) {
        lines.push_back(line.empty() ? line : "  " + line);
    }
}

} // namespace

namespace porytiles {

ChainableResult<InspectTilesetColors::LoadedTileset> InspectTilesetColors::load(const std::string &tileset_name) const
{
    assert_or_panic(metadata_provider_->exists(tileset_name), "precondition violated: tileset must exist");
    if (!tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{
            "Tileset '{}' exists but is not Porytiles-managed.", FormatParam{tileset_name, Style::bold}};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset,
        tileset_repo_->load(tileset_name),
        LoadedTileset,
        format_->format("Failed to load tileset '{}'.", FormatParam{tileset_name, Style::bold}));

    PT_UNWRAP_TILESET_CONFIG_PTR(domain_config_, extrinsic_transparency, tileset_name, LoadedTileset);

    const LayerImageMetatileizer<Rgba32> metatileizer{};
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles,
        metatileizer.metatileize(
            tileset->porytiles_component().bottom(),
            tileset->porytiles_component().middle(),
            tileset->porytiles_component().top()),
        LoadedTileset,
        format_->format(
            "Failed to metatileize input layer images for tileset '{}'.", FormatParam{tileset_name, Style::bold}));

    return LoadedTileset{
        .tileset = std::move(tileset),
        .metatiles = std::move(metatiles),
        .extrinsic_transparency = extrinsic_transparency.value()};
}

ChainableResult<std::vector<std::string>>
InspectTilesetColors::find_color(const std::string &tileset_name, const FindColorOptions &options) const
{
    PT_TRY_ASSIGN_PASS_ERR(loaded, load(tileset_name), std::vector<std::string>);
    const auto &data = loaded;

    const ColorMatcher matcher{options.color, options.tolerance};
    const auto metatile_matches = find_color_in_metatiles(data.metatiles, matcher);
    const auto anim_matches = find_color_in_anims(data.tileset->porytiles_component().anims(), matcher);

    std::size_t matching_pixels = 0;
    for (const auto &match : metatile_matches) {
        matching_pixels += match.pixel_coords.size();
    }
    for (const auto &match : anim_matches) {
        matching_pixels += match.pixel_indexes.size();
    }

    std::vector<std::string> lines{};
    const std::string query = describe_query(*format_, matcher);
    if (matching_pixels == 0) {
        lines.push_back(format_->format(
            "No pixels of {} found in tileset '{}'.", FormatParam{query}, FormatParam{tileset_name, Style::bold}));
        return lines;
    }

    const std::size_t total_matches = metatile_matches.size() + anim_matches.size();
    lines.push_back(format_->format(
        "Found {} pixel(s) of {} in tileset '{}': {} metatile layer(s), {} animation tile(s).",
        FormatParam{matching_pixels, Style::bold},
        FormatParam{query},
        FormatParam{tileset_name, Style::bold},
        FormatParam{metatile_matches.size(), Style::bold},
        FormatParam{anim_matches.size(), Style::bold}));

    const std::size_t shown = options.limit.has_value() ? std::min(*options.limit, total_matches) : total_matches;
    if (shown < total_matches) {
        lines.push_back(format_->format(
            "Showing the first {} of {} match(es). Pass '{}' to show every match.",
            FormatParam{shown, Style::bold},
            FormatParam{total_matches, Style::bold},
            FormatParam{"--limit all", Style::bold}));
    }

    std::size_t rendered = 0;
    for (const auto &match : metatile_matches) {
        if (rendered == shown) {
            break;
        }
        lines.emplace_back();
        lines.push_back(format_->format(
            "{}: {} matching pixel(s)",
            FormatParam{metatile::message_header(*format_, match.metatile_index, match.layer), Style::bold},
            FormatParam{match.pixel_coords.size(), Style::bold}));
        lines.append_range(tile_printer_->print_metatile_pixel_highlights(
            data.metatiles.at(match.metatile_index), match.layer, match.pixel_coords, data.extrinsic_transparency));
        rendered++;
    }
    for (const auto &match : anim_matches) {
        if (rendered == shown) {
            break;
        }
        const auto &anim = data.tileset->porytiles_component().anim_for_name(match.anim_name);
        const auto &frame = (anim.has_key_frame() && anim.key_frame().frame_name() == match.frame_name)
                                ? anim.key_frame()
                                : anim.frames().at(match.frame_name);
        lines.emplace_back();
        lines.push_back(format_->format(
            "{}: {} matching pixel(s)",
            FormatParam{
                anim::message_header(*format_, match.anim_name, match.frame_name, match.tile_index), Style::bold},
            FormatParam{match.pixel_indexes.size(), Style::bold}));
        lines.append_range(tile_printer_->print_tile_pixel_highlights(
            frame.tile_at(match.tile_index), match.pixel_indexes, data.extrinsic_transparency));
        rendered++;
    }

    return lines;
}

ChainableResult<std::vector<std::string>>
InspectTilesetColors::dump_colors(const std::string &tileset_name, const DumpColorsOptions &options) const
{
    PT_TRY_ASSIGN_PASS_ERR(loaded, load(tileset_name), std::vector<std::string>);
    const auto &data = loaded;

    PT_TRY_ASSIGN_CHAIN_ERR(
        is_secondary,
        metadata_provider_->is_secondary(tileset_name),
        std::vector<std::string>,
        format_->format("Failed to determine tileset kind for '{}'.", FormatParam{tileset_name, Style::bold}));
    PT_UNWRAP_TILESET_CONFIG_PTR(domain_config_, num_palettes_in_primary, tileset_name, std::vector<std::string>);
    PT_UNWRAP_TILESET_CONFIG_PTR(domain_config_, num_palettes_total, tileset_name, std::vector<std::string>);

    // Same budget formula as the compiler's global color count check, so this number is the one compile enforces.
    const ConfigValue<std::size_t> num_palettes_cfg = is_secondary ? num_palettes_total : num_palettes_in_primary;
    constexpr std::size_t usable_colors_per_palette = palette::max_size - 1;
    const std::size_t budget = num_palettes_cfg.value() * usable_colors_per_palette;

    const auto summary =
        count_tileset_colors(data.metatiles, data.tileset->porytiles_component().anims(), data.extrinsic_transparency);
    const auto sorted = sort_color_counts_descending(summary.counts);

    std::vector<std::string> lines{};
    lines.push_back(format_->format(
        "Tileset '{}' has {} unique color(s) across {} opaque pixel(s) ({} transparent pixel(s) skipped).",
        FormatParam{tileset_name, Style::bold},
        FormatParam{summary.counts.size(), Style::bold},
        FormatParam{summary.opaque_pixels, Style::bold},
        FormatParam{summary.transparent_pixels, Style::bold}));
    lines.push_back(format_->format(
        "Color budget is {} color(s) ('{}' {} * {} usable colors per palette): {}/{}, {} used.",
        FormatParam{budget, Style::bold},
        FormatParam{num_palettes_cfg.canonical_name(), Style::bold},
        FormatParam{num_palettes_cfg.value(), Style::bold},
        FormatParam{usable_colors_per_palette, Style::bold},
        FormatParam{summary.counts.size(), Style::bold},
        FormatParam{budget, Style::bold},
        FormatParam{format_percentage(summary.counts.size(), budget), Style::bold}));

    if (sorted.empty()) {
        return lines;
    }

    if (!options.group) {
        lines.emplace_back();
        lines.append_range(palette_printer_->print_rgba_counts(sorted, summary.opaque_pixels));
        return lines;
    }

    const ColorTolerance tolerance = options.tolerance.value_or(DumpColorsOptions::default_group_tolerance);
    const auto groups = group_similar_colors(sorted, tolerance);

    // Only a group with company is interesting: it is where a near-duplicate hides. Singleton groups print as a flat
    // list afterwards so the dump stays complete without one header per color.
    std::vector<const ColorGroup *> clusters{};
    std::vector<std::pair<Rgba32, unsigned int>> singletons{};
    for (const auto &group : groups) {
        if (group.members.size() > 1) {
            clusters.push_back(&group);
        }
        else {
            singletons.push_back(group.members.front());
        }
    }

    lines.push_back(describe_groups(*format_, clusters.size(), tolerance));
    std::size_t group_number = 1;
    for (const auto *group : clusters) {
        lines.emplace_back();
        std::string header = format_->format(
            "group {}: {} color(s), {} pixel(s) ({})",
            FormatParam{group_number, Style::bold},
            FormatParam{group->members.size(), Style::bold},
            FormatParam{group->total_pixels, Style::bold},
            FormatParam{format_percentage(group->total_pixels, summary.opaque_pixels), Style::bold});
        if (tolerance.rule() == ColorTolerance::Rule::gba) {
            // Every member quantizes to the same color, so the anchor's quantized value is the group's.
            header += format_->format(
                ", displayed by the GBA as '{}'",
                FormatParam{group->anchor.quantize_to_gba().to_jasc_str(), Style::bold});
        }
        lines.push_back(std::move(header));
        append_indented(lines, palette_printer_->print_rgba_counts(group->members, summary.opaque_pixels));
        group_number++;
    }

    if (!singletons.empty()) {
        lines.emplace_back();
        lines.push_back(describe_singletons(*format_, singletons.size(), tolerance));
        append_indented(lines, palette_printer_->print_rgba_counts(singletons, summary.opaque_pixels));
    }

    return lines;
}

} // namespace porytiles
