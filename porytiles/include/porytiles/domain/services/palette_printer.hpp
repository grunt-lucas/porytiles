#pragma once

#include <string>
#include <vector>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/packing/models/palette_hint.hpp"
#include "porytiles/utilities/count_map_to_list.hpp"

namespace porytiles {

/// @brief A collection of printer functions for the Palette and related types.
class PalettePrinter {
  public:
    virtual ~PalettePrinter() = default;

    [[nodiscard]] virtual std::vector<std::string>
    print_rgba_palette(const Palette<Rgba32, palette::max_size> &palette) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_rgba_palette(const Palette<Rgba32> &palette) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_rgba_palette_with_highlights(const Palette<Rgba32> &palette, const std::vector<std::size_t> &slots) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_rgba_palette_with_highlights(
        const Palette<Rgba32, palette::max_size> &palette, const std::vector<std::size_t> &slots) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_palette_hint_with_highlights(const PaletteHint &hint, const std::vector<std::size_t> &slots) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_rgba_palette_covered_missing(
        const Palette<Rgba32, palette::max_size> &palette,
        std::set<Rgba32> covered_colors,
        std::set<Rgba32> missing_colors) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_rgba_counts(const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts) const = 0;

    /// @brief Prints color counts with each color's share of a pixel total.
    ///
    /// @param colors_counts The (color, count) pairs to print, in the order they should appear
    /// @param total_pixels The denominator for the percentage column, must be at least the sum of the counts
    /// @return The rendered lines, one per color
    [[nodiscard]] virtual std::vector<std::string> print_rgba_counts(
        const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts, std::size_t total_pixels) const = 0;

    [[nodiscard]] std::vector<std::string>
    print_rgba_palette_counts(const std::map<Rgba32, unsigned int> &color_counts) const
    {
        const auto sorted_colors = counts_to_descending_list(color_counts);
        return print_rgba_counts(sorted_colors);
    }
};

} // namespace porytiles
