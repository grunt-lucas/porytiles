#include "porytiles2/domain/services/metatile_validator.hpp"

#include <map>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/utilities/count_map_to_list.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles2;

void report_validation_error(
    const Metatile<Rgba32> &metatile,
    std::size_t metatile_index,
    std::size_t internal_tile_index,
    std::size_t row,
    std::size_t col,
    const std::string &diagnostic_code,
    const std::string &error_message,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const TilePrinter *tile_printer)
{
    auto [layer, subtile] = metatile::from_internal_tile_index(internal_tile_index);
    std::vector errors = {format->format(
        "{}: {}",
        FormatParam{porytiles2::metatile::message_header(metatile_index, layer, subtile, row, col, *format)},
        FormatParam{error_message})};
    errors.emplace_back("");
    std::vector highlight = tile_printer->print_metatile_highlight(metatile, layer, subtile, row, col);
    std::ranges::copy(highlight, std::back_inserter(errors));
    diag->err(diagnostic_code, errors);
}

void report_color_counts(
    const std::map<Rgba32, unsigned int> &color_counts,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const PalettePrinter *pal_printer)
{
    std::vector<std::string> color_lines;
    color_lines.emplace_back("color counts:");
    auto counts = pal_printer->print_rgba_palette_counts(color_counts);
    std::ranges::copy(counts, std::back_inserter(color_lines));
    diag->note("tile-color-count-violation", color_lines);
}

} // namespace

namespace porytiles2 {

ChainableResult<void> MetatileValidator::validate_alpha_channels(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    bool hit_error = false;
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto tiles = metatile.decompose();

        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];

            // Iterate over each pixel in the current internal tile
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                        hit_error = true;
                        std::string error_message = format_->format(
                            "invalid alpha channel: {}", FormatParam{std::to_string(pixel.alpha()), Style::bold});
                        report_validation_error(
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "alpha-channel-violation",
                            error_message,
                            format_,
                            diag_,
                            tile_printer_);
                    }
                }
            }
        }
        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{"alpha channel violation: found invalid alpha channels"};
    }

    return {};
}

ChainableResult<void> MetatileValidator::validate_tile_color_count(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    PT_UNWRAP_TILESET_CONFIG(config_, extrinsic_transparency, tileset_scope_, void);

    bool hit_error = false;
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto tiles = metatile.decompose();

        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];
            std::map<Rgba32, unsigned int> color_counts;

            // Iterate over each pixel in the current internal tile
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_transparent && pixel != extrinsic_transparency.value()) {
                        color_counts[pixel]++;
                    }

                    if (color_counts.size() > pal::max_size - 1) {
                        hit_error = true;
                        std::string error_message = format_->format(
                            "found {}th unique color: {}",
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold});
                        report_validation_error(
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "tile-color-count-violation",
                            error_message,
                            format_,
                            diag_,
                            tile_printer_);
                        report_color_counts(color_counts, format_, diag_, pal_printer_);
                        goto next_tile;
                    }
                }
            }
        next_tile:;
        }
        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{
            "tile color count violation: found tile(s) with more than {} unique non-transparent pixels",
            FormatParam{pal::max_size - 1}};
    }

    return {};
}

ChainableResult<void> MetatileValidator::validate_global_color_count(
    const std::vector<Metatile<Rgba32>> &metatiles, std::size_t count_limit) const
{
    PT_UNWRAP_TILESET_CONFIG(config_, extrinsic_transparency, tileset_scope_, void);

    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto tiles = metatile.decompose();
        for (std::size_t internal_tile_index = 0; internal_tile_index < tiles.size(); ++internal_tile_index) {
            const auto &tile = tiles[internal_tile_index];
            // TODO: implement
            std::ignore = tile.unique_nontransparent_colors(extrinsic_transparency.value());
        }
    }
    /*
     * TODO: It would be nice to give users very detailed information about their global color count when they go over.
     * Example, we could print out a list of colors with their pixel counts, the first location of colors that went over
     * the limit, etc. This will really help users narrow down issues when they exceed color count.
     */
    std::ignore = count_limit;
    return {};
}

ChainableResult<void>
MetatileValidator::generate_precision_loss_warnings(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    // TODO: implement
    (void)metatiles.size();
    return {};
}

ChainableResult<void> MetatileValidator::validate_primary(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    PT_UNWRAP_TILESET_CONFIG(config_, num_pals_primary, tileset_scope_, void);
    PT_UNWRAP_TILESET_CONFIG(config_, num_metatiles_primary, tileset_scope_, void);

    std::vector<std::string> error_messages;

    // Run metatile count validation
    if (metatiles.size() > num_metatiles_primary) {
        diag_->err(
            "metatile-limit-exceeded",
            format_->format(
                "too many metatiles ({}) in Porytiles component for tileset '{}'",
                FormatParam{metatiles.size(), Style::bold},
                FormatParam{tileset_scope_, Style::bold}));

        // Construct note text
        std::vector<std::string> note_text;
        note_text.push_back(format_->format(
            "metatile limit is '{}' due to configuration", FormatParam{num_metatiles_primary, Style::bold}));
        note_text.emplace_back("");
        std::ranges::copy(num_metatiles_primary.prettify(*format_), std::back_inserter(note_text));

        // Emit note
        diag_->note("metatile-limit-exceeded", note_text);

        // Push back error message to fatal chain
        error_messages.push_back(format_->format(
            "too many input metatiles: found '{}', limit is '{}'",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{num_metatiles_primary, Style::bold}));
    }

    // Run alpha channel validation
    auto alpha_result = validate_alpha_channels(metatiles);
    if (!alpha_result.has_value()) {
        error_messages.push_back(alpha_result.error().join(*format_));
    }

    // Run tile color count validation
    auto tile_color_result = validate_tile_color_count(metatiles);
    if (!tile_color_result.has_value()) {
        error_messages.push_back(tile_color_result.error().join(*format_));
    }

    // Run precision loss warning generation
    auto precision_result = generate_precision_loss_warnings(metatiles);
    if (!precision_result.has_value()) {
        error_messages.push_back(precision_result.error().join(*format_));
    }

    // Run global color count validation
    std::size_t color_count_limit = num_pals_primary.value() * (pal::max_size - 1);
    auto global_color_result = validate_global_color_count(metatiles, color_count_limit);
    if (!global_color_result.has_value()) {
        error_messages.push_back(global_color_result.error().join(*format_));
    }

    // If any validation failed, return all error messages
    if (!error_messages.empty()) {
        std::vector<std::string> combined_message{};
        combined_message.emplace_back("validation failed with the following errors:");
        for (const auto &msg : error_messages) {
            combined_message.emplace_back("  - " + msg);
        }
        return FormattableError{combined_message};
    }

    return {};
}

} // namespace porytiles2
