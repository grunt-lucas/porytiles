#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

class ColorPalettePrinter : public PalettePrinter {
  public:
    explicit ColorPalettePrinter(gsl::not_null<TextFormatter *> format) : format_{format} {}

    [[nodiscard]] std::vector<std::string> print_rgba_palette(const Palette<Rgba32> &pal) const override;

    [[nodiscard]] std::vector<std::string>
    print_rgba_counts(const std::vector<std::pair<Rgba32, unsigned int>> &colors_counts) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles2
