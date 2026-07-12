#pragma once

#include "gsl/pointers"

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/packing/models/palette_hint.hpp"
#include "porytiles/domain/services/palette_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

class ColorPalettePrinter : public PalettePrinter {
  public:
    explicit ColorPalettePrinter(gsl::not_null<TextFormatter *> format) : format_{format} {}

    [[nodiscard]] std::vector<std::string>
    print_rgba_palette(const Palette<Rgba32, palette::max_size> &palette) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_palette(const Palette<Rgba32> &palette) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_palette_with_highlights(
        const Palette<Rgba32> &palette, const std::vector<std::size_t> &slots) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_palette_with_highlights(
        const Palette<Rgba32, palette::max_size> &palette, const std::vector<std::size_t> &slots) const override;

    [[nodiscard]] std::vector<std::string>
    print_palette_hint_with_highlights(const PaletteHint &hint, const std::vector<std::size_t> &slots) const override;

    [[nodiscard]] std::vector<std::string> print_rgba_palette_covered_missing(
        const Palette<Rgba32, palette::max_size> &palette,
        std::set<Rgba32> covered_colors,
        std::set<Rgba32> missing_colors) const override;

    [[nodiscard]] std::vector<std::string>
    print_rgba_counts(const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles
