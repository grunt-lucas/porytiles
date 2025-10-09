#include "porytiles2/domain/services/tile_validator.hpp"

#include <unordered_set>

#include "porytiles2/domain/models/rgba_tile.hpp"
#include "porytiles2/domain/models/tile_constants.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> TileValidator::validate_alpha_channels(const std::vector<RgbaTile> &tiles) const
{
    bool hit_error = false;
    std::size_t tile_index = 0;
    for (const auto &tile : tiles) {
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const auto &pixel = tile.at(row, col);
                if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                    hit_error = true;
                    auto [metatile_index, layer, subtile] = metatile::compute_metatile(tile_index);
                    // TODO: create a standard utility to format metatile string
                    std::vector errors = {format_->format(
                        "|metatile {}|{}|{}|{},{}|: invalid alpha channel: {}",
                        FormatParam{metatile_index, Style::bold},
                        FormatParam{to_string(layer), Style::bold},
                        FormatParam{to_string(subtile), Style::bold},
                        FormatParam{std::to_string(row), Style::bold},
                        FormatParam{std::to_string(col), Style::bold},
                        FormatParam{std::to_string(pixel.alpha()), Style::bold})};
                    diag_->err(errors);
                }
            }
        }
        tile_index++;
    }

    if (hit_error) {
        return FormattableError{"alpha channel validation failed"};
    }

    return {};
}

ChainableResult<void> TileValidator::validate_unique_color_count(const std::vector<RgbaTile> &tiles) const
{
    return {};
}

ChainableResult<void> TileValidator::generate_precision_loss_warnings(const std::vector<RgbaTile> &tiles) const
{
    return {};
}

} // namespace porytiles2
