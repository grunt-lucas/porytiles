#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A TilePrinter implementation that generates ASCII art tiles with formatting based on the provided
 * TextFormatter.
 */
class AsciiTilePrinter final : public TilePrinter {
  public:
    explicit AsciiTilePrinter(gsl::not_null<TextFormatter *> format) : format_{format} {}

    [[nodiscard]] std::vector<std::string>
    print_metatile_highlight(metatile::Subtile subtile, std::size_t row, std::size_t col, Style color) const override;

    [[nodiscard]] std::vector<std::string> print_metatile_highlights(
        metatile::Subtile subtile, const std::vector<std::size_t> &indexes, Style color) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles2
