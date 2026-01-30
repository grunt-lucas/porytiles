#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

class ColorPalettePrinter : public PalettePrinter {
  public:
    explicit ColorPalettePrinter(gsl::not_null<TextFormatter *> format) : format_{format} {}

    [[nodiscard]] std::vector<std::string> print_rgba_pal(const Palette<Rgba32, pal::max_size> &pal) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_pal(const Palette<Rgba32> &pal) const override;

    [[nodiscard]] std::vector<std::string>
    print_rgba_pal_with_highlights(const Palette<Rgba32> &pal, const std::vector<std::size_t> &slots) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_pal_with_highlights(
        const Palette<Rgba32, pal::max_size> &pal, const std::vector<std::size_t> &slots) const override;

    [[nodiscard]] std::vector<std::string>
    print_pal_hint_with_highlights(const PaletteHint &hint, const std::vector<std::size_t> &slots) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_palette_covered_missing(
        const Palette<Rgba32, pal::max_size> &pal,
        std::set<Rgba32> covered_colors,
        std::set<Rgba32> missing_colors) const override;

    [[nodiscard]] std::vector<std::string>
    print_rgba_counts(const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles2
