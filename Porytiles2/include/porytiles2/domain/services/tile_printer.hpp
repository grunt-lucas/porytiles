#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief A collection of printer functions for various tile types.
 */
class TilePrinter {
  public:
    virtual ~TilePrinter() = default;

    [[nodiscard]] virtual std::vector<std::string> print_metatile_highlight(
        const Metatile<Rgba32> &metatile,
        metatile::Layer layer,
        metatile::Subtile subtile,
        std::size_t row,
        std::size_t col) const = 0;

    [[nodiscard]] virtual std::vector<std::string> print_metatile_highlights(
        const Metatile<Rgba32> &metatile,
        metatile::Layer layer,
        metatile::Subtile subtile,
        const std::vector<std::size_t> &indexes) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_tile_highlight(const PixelTile<Rgba32> &tile, std::size_t row, std::size_t col) const = 0;

    [[nodiscard]] virtual std::vector<std::string>
    print_tile_highlights(const PixelTile<Rgba32> &tile, const std::vector<std::size_t> &indexes) const = 0;
};

} // namespace porytiles2
