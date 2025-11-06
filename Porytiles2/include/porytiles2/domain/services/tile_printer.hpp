#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A collection of printer functions for various tile types.
 */
class TilePrinter {
  public:
    virtual ~TilePrinter() = default;

    [[nodiscard]] virtual std::vector<std::string>
    print_metatile_highlight(metatile::Subtile subtile, std::size_t row, std::size_t col, Style color) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_metatile_highlights(
        metatile::Subtile subtile, const std::vector<std::size_t> &indexes, Style color) const = 0;
};

} // namespace porytiles2
