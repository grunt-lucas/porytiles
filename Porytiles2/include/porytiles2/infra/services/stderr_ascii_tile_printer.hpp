#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/tile_constants.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A TilePrinter implementation that prints ASCII art tiles to stderr.
 */
class StderrAsciiTilePrinter final : public TilePrinter {
  public:
    explicit StderrAsciiTilePrinter(gsl::not_null<TextFormatter *> format) : format_{format} {}

    [[nodiscard]] std::vector<std::string>
    print_metatile_highlight(metatile::Subtile subtile, std::size_t row, std::size_t col) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles2
