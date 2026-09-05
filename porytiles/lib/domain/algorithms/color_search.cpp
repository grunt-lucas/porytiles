#include "porytiles/domain/algorithms/color_search.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <expected>
#include <format>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/parse_int.hpp"

namespace {

using namespace porytiles;

/// @brief Collects the row-major pixel indexes of a tile that the matcher accepts.
std::vector<std::size_t> matching_pixel_indexes(const PixelTile<Rgba32> &tile, const ColorMatcher &matcher)
{
    std::vector<std::size_t> indexes{};
    for (std::size_t index = 0; index < tile::size_pix; ++index) {
        if (matcher.matches(tile.at(index))) {
            indexes.push_back(index);
        }
    }
    return indexes;
}

/// @brief Collects the metatile-local coordinates of matching pixels across one layer's four subtiles.
std::set<std::pair<std::size_t, std::size_t>> matching_layer_coords(
    const std::array<PixelTile<Rgba32>, metatile::tiles_per_metatile_layer> &layer_tiles, const ColorMatcher &matcher)
{
    std::set<std::pair<std::size_t, std::size_t>> coords{};
    for (std::size_t subtile_index = 0; subtile_index < layer_tiles.size(); ++subtile_index) {
        const std::size_t row_offset = (subtile_index / metatile::tiles_per_side) * tile::side_length_pix;
        const std::size_t col_offset = (subtile_index % metatile::tiles_per_side) * tile::side_length_pix;
        for (const auto index : matching_pixel_indexes(layer_tiles.at(subtile_index), matcher)) {
            const auto [row, col] = tile::index_to_row_col(index);
            coords.insert({row + row_offset, col + col_offset});
        }
    }
    return coords;
}

void count_tile_colors(const PixelTile<Rgba32> &tile, const Rgba32 &extrinsic_transparency, ColorCountSummary &summary)
{
    for (const auto &pixel : tile.pix()) {
        if (pixel.is_transparent(extrinsic_transparency)) {
            summary.transparent_pixels++;
        }
        else {
            summary.counts[pixel]++;
            summary.opaque_pixels++;
        }
    }
}

/// @brief Visits an animation's frames in the search order: key frame first, then the named frames.
template <typename Visitor>
void for_each_anim_frame(const Animation<Rgba32> &anim, Visitor visit)
{
    if (anim.has_key_frame()) {
        visit(anim.key_frame());
    }
    for (const AnimFrame<Rgba32> &frame : anim.frames() | std::views::values) {
        visit(frame);
    }
}

} // namespace

namespace porytiles {

bool rgb_within_tolerance(const Rgba32 &a, const Rgba32 &b, std::uint8_t tolerance)
{
    const auto channel_within = [tolerance](std::uint8_t x, std::uint8_t y) {
        return std::abs(static_cast<int>(x) - static_cast<int>(y)) <= static_cast<int>(tolerance);
    };
    return channel_within(a.red(), b.red()) && channel_within(a.green(), b.green()) &&
           channel_within(a.blue(), b.blue());
}

bool rgb_gba_equivalent(const Rgba32 &a, const Rgba32 &b)
{
    return a.quantize_to_gba().equals_ignoring_alpha(b.quantize_to_gba());
}

bool ColorTolerance::similar(const Rgba32 &a, const Rgba32 &b) const
{
    switch (rule_) {
    case Rule::per_channel:
        return rgb_within_tolerance(a, b, steps_);
    case Rule::gba:
        return rgb_gba_equivalent(a, b);
    }
    return false;
}

std::expected<ColorTolerance, std::string> parse_color_tolerance(std::string_view text)
{
    if (text == "gba") {
        return ColorTolerance::gba();
    }
    const auto steps = parse_int<int>(text, 10);
    if (!steps.has_value() || steps.value() < 0 || steps.value() > 255) {
        return std::unexpected{std::format("'{}' must be an integer in 0-255 or 'gba'", text)};
    }
    return ColorTolerance::per_channel(static_cast<std::uint8_t>(steps.value()));
}

bool ColorMatcher::matches(const Rgba32 &pixel) const
{
    if (pixel.is_intrinsically_transparent()) {
        return false;
    }
    return tolerance_.similar(pixel, target_);
}

std::vector<MetatileColorMatch>
find_color_in_metatiles(const std::vector<Metatile<Rgba32>> &metatiles, const ColorMatcher &matcher)
{
    std::vector<MetatileColorMatch> matches{};
    for (std::size_t metatile_index = 0; metatile_index < metatiles.size(); ++metatile_index) {
        const auto &metatile = metatiles.at(metatile_index);
        const std::array layers{
            std::pair{metatile::Layer::bottom, &metatile.bottom()},
            std::pair{metatile::Layer::middle, &metatile.middle()},
            std::pair{metatile::Layer::top, &metatile.top()},
        };
        for (const auto &[layer, layer_tiles] : layers) {
            auto coords = matching_layer_coords(*layer_tiles, matcher);
            if (!coords.empty()) {
                matches.push_back({metatile_index, layer, std::move(coords)});
            }
        }
    }
    return matches;
}

std::vector<AnimTileColorMatch>
find_color_in_anims(const std::map<std::string, Animation<Rgba32>> &anims, const ColorMatcher &matcher)
{
    std::vector<AnimTileColorMatch> matches{};
    for (const auto &[anim_name, anim] : anims) {
        for_each_anim_frame(anim, [&](const AnimFrame<Rgba32> &frame) {
            for (std::size_t tile_index = 0; tile_index < frame.tile_count(); ++tile_index) {
                auto indexes = matching_pixel_indexes(frame.tile_at(tile_index), matcher);
                if (!indexes.empty()) {
                    matches.push_back({anim_name, frame.frame_name(), tile_index, std::move(indexes)});
                }
            }
        });
    }
    return matches;
}

ColorCountSummary count_tileset_colors(
    const std::vector<Metatile<Rgba32>> &metatiles,
    const std::map<std::string, Animation<Rgba32>> &anims,
    const Rgba32 &extrinsic_transparency)
{
    ColorCountSummary summary{.counts = {}, .opaque_pixels = 0, .transparent_pixels = 0};
    for (const auto &metatile : metatiles) {
        for (const auto &tile : metatile.decompose()) {
            count_tile_colors(tile, extrinsic_transparency, summary);
        }
    }
    for (const Animation<Rgba32> &anim : anims | std::views::values) {
        for_each_anim_frame(anim, [&](const AnimFrame<Rgba32> &frame) {
            for (const auto &tile : frame.tiles()) {
                count_tile_colors(tile, extrinsic_transparency, summary);
            }
        });
    }
    return summary;
}

std::vector<std::pair<Rgba32, unsigned int>> sort_color_counts_descending(const std::map<Rgba32, unsigned int> &counts)
{
    // The map already iterates in ascending color order, so a stable sort on count alone yields the color tie-break.
    std::vector<std::pair<Rgba32, unsigned int>> sorted{counts.begin(), counts.end()};
    std::ranges::stable_sort(sorted, [](const auto &a, const auto &b) { return a.second > b.second; });
    return sorted;
}

std::vector<ColorGroup>
group_similar_colors(const std::vector<std::pair<Rgba32, unsigned int>> &sorted_counts, const ColorTolerance &tolerance)
{
    std::vector<ColorGroup> groups{};
    for (const auto &entry : sorted_counts) {
        const auto &[color, count] = entry;
        auto home = std::ranges::find_if(
            groups, [&](const ColorGroup &group) { return tolerance.similar(color, group.anchor); });
        if (home == groups.end()) {
            groups.push_back({.anchor = color, .members = {entry}, .total_pixels = count});
        }
        else {
            home->members.push_back(entry);
            home->total_pixels += count;
        }
    }
    // Groups were created in descending anchor-count order, so a stable sort on the total keeps the anchor ordering
    // as the tie-breaker.
    std::ranges::stable_sort(
        groups, [](const ColorGroup &a, const ColorGroup &b) { return a.total_pixels > b.total_pixels; });
    return groups;
}

} // namespace porytiles
