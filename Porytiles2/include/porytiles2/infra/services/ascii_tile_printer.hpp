#pragma once

#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
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

    [[nodiscard]] std::vector<std::string> print_metatile(
        const Metatile<Rgba32> &metatile,
        metatile::Layer layer,
        metatile::Subtile subtile,
        const Rgba32 &extrinsic_transparency) const override;

    [[nodiscard]] std::vector<std::string> print_metatile_pixel_highlight(
        const Metatile<Rgba32> &metatile,
        metatile::Layer layer,
        metatile::Subtile subtile,
        std::size_t row,
        std::size_t col,
        const Rgba32 &extrinsic_transparency) const override;

    [[nodiscard]] std::vector<std::string> print_metatile_pixel_highlights(
        const Metatile<Rgba32> &metatile,
        metatile::Layer layer,
        metatile::Subtile subtile,
        const std::vector<std::size_t> &indexes,
        const Rgba32 &extrinsic_transparency) const override;

    [[nodiscard]] std::vector<std::string>
    print_tile(const PixelTile<Rgba32> &tile, const Rgba32 &extrinsic_transparency) const override;

    [[nodiscard]] std::vector<std::string> print_tile_pixel_highlight(
        const PixelTile<Rgba32> &tile,
        std::size_t row,
        std::size_t col,
        const Rgba32 &extrinsic_transparency) const override;

    [[nodiscard]] std::vector<std::string> print_tile_pixel_highlights(
        const PixelTile<Rgba32> &tile,
        const std::vector<std::size_t> &indexes,
        const Rgba32 &extrinsic_transparency) const override;

  private:
    TextFormatter *format_;
};

} // namespace porytiles2
