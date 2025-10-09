#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/tile_constants.hpp"

namespace porytiles2 {

/**
 * @brief A collection of printer functions for various tile types.
 */
class TilePrinter {
  public:
    virtual ~TilePrinter() = default;

    [[nodiscard]] virtual std::vector<std::string>
    print_metatile_highlight(metatile::Subtile subtile, std::size_t row, std::size_t col) const = 0;
};

} // namespace porytiles2
