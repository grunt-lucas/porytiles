#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/count_map_to_list.hpp"

namespace porytiles2 {

/**
 * @brief A collection of printer functions for the Palette and related types.
 */
class PalettePrinter {
  public:
    virtual ~PalettePrinter() = default;

    [[nodiscard]] virtual std::vector<std::string>
    print_rgba_palette(const Palette<Rgba32, pal::max_size> &pal) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_rgba_pal_with_highlights(const Palette<Rgba32> &pal, const std::vector<std::size_t> &slots) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_rgba_pal_with_highlights(
        const Palette<Rgba32, pal::max_size> &pal, const std::vector<std::size_t> &slots) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_pal_hint_with_highlights(const PaletteHint &hint, const std::vector<std::size_t> &slots) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_rgba_palette_covered_missing(
        const Palette<Rgba32, pal::max_size> &pal,
        std::set<Rgba32> covered_colors,
        std::set<Rgba32> missing_colors) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_rgba_counts(const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts) const = 0;

    [[nodiscard]] std::vector<std::string>
    print_rgba_pal_counts(const std::map<Rgba32, unsigned int> &color_counts) const
    {
        const auto sorted_colors = counts_to_descending_list(color_counts);
        return print_rgba_counts(sorted_colors);
    }
};

} // namespace porytiles2
