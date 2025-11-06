#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A collection of printer functions for the Palette type.
 */
class PalettePrinter {
  public:
    virtual ~PalettePrinter() = default;

    [[nodiscard]] virtual std::vector<std::string> print_rgba_palette(const Palette<Rgba32> &pal) const = 0;
};

} // namespace porytiles2
