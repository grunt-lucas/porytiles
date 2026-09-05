#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

namespace porytiles {

/// @brief Checks whether two colors are within a per-channel tolerance of each other, ignoring alpha.
///
/// @details
/// The distance is the largest absolute difference across the red, green, and blue channels. A tolerance of 0 means an
/// exact RGB match.
///
/// @param a The first color
/// @param b The second color
/// @param tolerance The largest per-channel difference that still counts as a match
/// @return True if every channel of @p a is within @p tolerance of the same channel of @p b
[[nodiscard]] bool rgb_within_tolerance(const Rgba32 &a, const Rgba32 &b, std::uint8_t tolerance);

/// @brief Checks whether two colors become the same color after gbagfx downsampling, ignoring alpha.
///
/// @details
/// The GBA stores 5 bits per channel, and gbagfx converts an 8-bit channel using integer division (by 8). Two colors
/// are equivalent when every channel converts to the same 5-bit value. Equivalence effectively partitions the color
/// space into 8x8x8 buckets, one per hardware color.
///
/// @param a The first color
/// @param b The second color
/// @return True if @p a and @p b downsample to the same GBA color
[[nodiscard]] bool rgb_gba_equivalent(const Rgba32 &a, const Rgba32 &b);

/// @brief The rule that decides when two colors count as the same, shared by color search and color grouping.
///
/// @details
/// Two rules exist. The per-channel rule accepts colors whose R, G, and B each differ by at most a fixed number of
/// steps, with 0 steps meaning an exact RGB match. The GBA rule accepts colors that downsample to the same GBA 15-bit
/// color, so an accepted pair is indistinguishable in the game.
class ColorTolerance {
  public:
    enum class Rule : std::uint8_t { per_channel, gba };

    [[nodiscard]] static constexpr ColorTolerance exact()
    {
        return ColorTolerance{Rule::per_channel, 0};
    }

    [[nodiscard]] static constexpr ColorTolerance per_channel(std::uint8_t steps)
    {
        return ColorTolerance{Rule::per_channel, steps};
    }

    [[nodiscard]] static constexpr ColorTolerance gba()
    {
        return ColorTolerance{Rule::gba, 0};
    }

    bool operator==(const ColorTolerance &other) const = default;

    /// @brief Applies the rule to two colors.
    [[nodiscard]] bool similar(const Rgba32 &a, const Rgba32 &b) const;

    [[nodiscard]] Rule rule() const
    {
        return rule_;
    }

    /// @brief The per-channel step count. Always 0 under the GBA rule.
    [[nodiscard]] std::uint8_t steps() const
    {
        return steps_;
    }

    /// @brief True for the per-channel rule with 0 steps, i.e. an exact RGB match.
    [[nodiscard]] bool is_exact() const
    {
        return rule_ == Rule::per_channel && steps_ == 0;
    }

  private:
    constexpr ColorTolerance(Rule rule, std::uint8_t steps) : rule_{rule}, steps_{steps} {}

    Rule rule_;
    std::uint8_t steps_;
};

/// @brief Parses the user-facing tolerance syntax.
///
/// @details
/// Accepts a decimal integer in [0, 255] for the per-channel rule, or the word "gba" for the GBA rule.
///
/// @param text The tolerance string
/// @return The tolerance, or a lowercase error fragment (no leading capital, no trailing period) for the caller to
///         wrap with its own context
[[nodiscard]] std::expected<ColorTolerance, std::string> parse_color_tolerance(std::string_view text);

/// @brief A pixel predicate for a target color and tolerance rule.
///
/// @details
/// Intrinsically transparent pixels (alpha 0) don't match. Since their RGB channels don't carry a visible color,
/// counting them would flood a search with every "matching" transparent pixel in the tileset. The extrinsic
/// transparency color is deliberately not excluded. The user can still search for it to locate transparent pixels.
class ColorMatcher {
  public:
    ColorMatcher(const Rgba32 &target, const ColorTolerance &tolerance) : target_{target}, tolerance_{tolerance} {}

    [[nodiscard]] bool matches(const Rgba32 &pixel) const;

    [[nodiscard]] const Rgba32 &target() const
    {
        return target_;
    }

    [[nodiscard]] const ColorTolerance &tolerance() const
    {
        return tolerance_;
    }

  private:
    Rgba32 target_;
    ColorTolerance tolerance_;
};

/// @brief One layer of one metatile that contains matching pixels.
struct MetatileColorMatch {
    std::size_t metatile_index;
    metatile::Layer layer;
    /// @brief Metatile-local (row, col) pairs of the matching pixels, each in [0, 16).
    std::set<std::pair<std::size_t, std::size_t>> pixel_coords;
};

/// @brief One animation frame tile that contains matching pixels.
struct AnimTileColorMatch {
    std::string anim_name;
    std::string frame_name;
    std::size_t tile_index;
    /// @brief Row-major pixel indexes of the matching pixels within the 8x8 tile, each in [0, 64).
    std::vector<std::size_t> pixel_indexes;
};

/// @brief Finds every metatile layer containing a pixel the matcher accepts.
///
/// @details
/// Results are in metatile order (index order followed by bottom/middle/top layer order), so the output is stable
/// across runs. A metatile with matches on two layers produces two entries.
///
/// @param metatiles The metatiles to search, indexed by position
/// @param matcher The pixel predicate
/// @return The matching layers with their pixel coordinates
[[nodiscard]] std::vector<MetatileColorMatch>
find_color_in_metatiles(const std::vector<Metatile<Rgba32>> &metatiles, const ColorMatcher &matcher);

/// @brief Finds every animation frame tile containing a pixel the matcher accepts.
///
/// @details
/// Animations are visited in alphabetical order. Within an animation the key frame (when present) comes first, then the
/// remaining frames in alphabetical order, then tiles in frame order.
///
/// @param anims The animations to search
/// @param matcher The pixel predicate
/// @return The matching tiles with their pixel indexes
[[nodiscard]] std::vector<AnimTileColorMatch>
find_color_in_anims(const std::map<std::string, Animation<Rgba32>> &anims, const ColorMatcher &matcher);

/// @brief The tally of every color in a tileset's Porytiles-side pixels.
struct ColorCountSummary {
    /// @brief Pixel count per color. Keyed by the full Rgba32 including alpha, matching the configured global color
    /// count
    std::map<Rgba32, unsigned int> counts;
    /// @brief Total pixels that were counted, i.e. the sum of @c counts.
    std::size_t opaque_pixels;
    /// @brief Pixels skipped because they are transparent, either alpha 0 or the extrinsic transparency color.
    std::size_t transparent_pixels;
};

/// @brief Counts every non-transparent pixel color across metatiles and animation frames.
///
/// @details
/// A pixel is skipped when it is intrinsically transparent (alpha 0) or matches the extrinsic transparency color.
/// Animation key frames and regular frames both count.
///
/// @param metatiles The tileset's metatiles
/// @param anims The tileset's animations
/// @param extrinsic_transparency The configured extrinsic transparency color
/// @return The per-color counts and pixel totals
[[nodiscard]] ColorCountSummary count_tileset_colors(
    const std::vector<Metatile<Rgba32>> &metatiles,
    const std::map<std::string, Animation<Rgba32>> &anims,
    const Rgba32 &extrinsic_transparency);

/// @brief Orders color counts by descending count, breaking ties by ascending color.
///
/// @param counts The per-color counts
/// @return The (color, count) pairs in display order
[[nodiscard]] std::vector<std::pair<Rgba32, unsigned int>>
sort_color_counts_descending(const std::map<Rgba32, unsigned int> &counts);

/// @brief A cluster of similar colors.
struct ColorGroup {
    /// @brief The group's most common color, which every member is similar to under the grouping rule.
    Rgba32 anchor;
    /// @brief The members in descending count order; the first member is the anchor.
    std::vector<std::pair<Rgba32, unsigned int>> members;
    /// @brief The sum of the members' counts.
    std::size_t total_pixels;
};

/// @brief Clusters similar colors around their most common representative.
///
/// @details
/// Uses a greedy anchor clustering approach. Colors are visited in the given (descending count) order. Each color joins
/// the first existing group whose anchor it is most similar to under @p tolerance, or starts a new group with itself as
/// the anchor. Anchoring on the most common colors (because we're iterating in descending count order) keeps a
/// per-channel group from chaining across unrelated colors the way transitive clustering would. Under the GBA rule
/// similarity is already transitive, so the groups turn out to be the downsampled buckets and each group is exactly one
/// BGR15 color. Groups are returned in descending order of total pixel count.
///
/// @param sorted_counts The (color, count) pairs in descending count order, as produced by sort_color_counts_descending
/// @param tolerance The rule that makes two colors similar
/// @return The groups
[[nodiscard]] std::vector<ColorGroup> group_similar_colors(
    const std::vector<std::pair<Rgba32, unsigned int>> &sorted_counts, const ColorTolerance &tolerance);

} // namespace porytiles
