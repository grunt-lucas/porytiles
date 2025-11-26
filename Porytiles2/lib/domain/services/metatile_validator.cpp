#include "porytiles2/domain/services/metatile_validator.hpp"

#include <map>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/count_map_to_list.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles2;

constexpr auto metatile_limit_exceeded = "metatile-limit-exceeded";
constexpr auto alpha_channel_violation = "alpha-channel-violation";
constexpr auto tile_color_count_violation = "tile-color-count-violation";
constexpr auto global_color_count_violation = "global-color-count-violation";
constexpr auto layer_mode_violation = "layer-mode-violation";

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
    const TilePrinter *tile_printer,
    const Rgba32 &extrinsic_transparency)
{
    auto [layer, subtile] = metatile::from_internal_tile_index(internal_tile_index);
    std::vector errors = {format->format(
        "{}: {}",
        FormatParam{
            porytiles2::metatile::message_header(*format, metatile_index, layer, subtile, row, col), Style::bold},
        FormatParam{error_message})};
    errors.emplace_back("");
    std::vector highlight =
        tile_printer->print_metatile_pixel_highlight(metatile, layer, subtile, row, col, extrinsic_transparency);
    std::ranges::copy(highlight, std::back_inserter(errors));
    diag->err(diagnostic_code, errors);
}

void report_color_counts(
    const std::map<Rgba32, unsigned int> &color_counts,
    const TextFormatter *format,
    const UserDiagnostics *diag,
    const std::string &tag,
    const PalettePrinter *pal_printer)
{
    std::vector<std::string> color_lines;
    color_lines.emplace_back("color counts:");
    auto counts = pal_printer->print_rgba_palette_counts(color_counts);
    std::ranges::copy(counts, std::back_inserter(color_lines));
    diag->note(tag, color_lines);
}

} // namespace

namespace porytiles2 {

ChainableResult<void> MetatileValidator::validate_alpha_channels(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_scope_, void);

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
                            alpha_channel_violation,
                            error_message,
                            format_,
                            diag_,
                            tile_printer_,
                            extrinsic_transparency);
                    }
                }
            }
        }
        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{"{}: found invalid alpha channels", FormatParam{alpha_channel_violation, Style::bold}};
    }

    return {};
}

ChainableResult<void> MetatileValidator::validate_tile_color_count(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_scope_, void);

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
                            tile_color_count_violation,
                            error_message,
                            format_,
                            diag_,
                            tile_printer_,
                            extrinsic_transparency);
                        report_color_counts(color_counts, format_, diag_, tile_color_count_violation, pal_printer_);
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
            "{}: found tile(s) with more than {} unique non-transparent pixels",
            FormatParam{tile_color_count_violation, Style::bold},
            FormatParam{pal::max_size - 1}};
    }

    return {};
}

ChainableResult<void> MetatileValidator::validate_global_color_count(
    const std::vector<Metatile<Rgba32>> &metatiles, std::size_t count_limit) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_scope_, void);

    std::map<Rgba32, unsigned int> color_counts;
    for (const auto &metatile : metatiles) {
        for (const auto tiles = metatile.decompose(); const auto &tile : tiles) {
            auto tile_colors = tile.unique_nontransparent_colors(extrinsic_transparency.value());
            for (const auto &color : tile_colors) {
                color_counts[color]++;
            }
        }
    }
    if (color_counts.size() > count_limit) {
        diag_->err(
            global_color_count_violation,
            format_->format(
                "global color count violation: found '{}' unique colors, limit is '{}'",
                FormatParam{color_counts.size(), Style::bold},
                FormatParam{count_limit, Style::bold}));
        report_color_counts(color_counts, format_, diag_, global_color_count_violation, pal_printer_);
        return FormattableError{
            "{}: found '{}' unique colors, limit is '{}'",
            FormatParam{global_color_count_violation, Style::bold},
            FormatParam{color_counts.size(), Style::bold},
            FormatParam{count_limit, Style::bold}};
    }
    return {};
}

ChainableResult<void>
MetatileValidator::generate_precision_loss_warnings(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    // TODO: implement
    (void)metatiles.size();
    return {};
}

ChainableResult<void>
MetatileValidator::validate_layer_mode(const std::vector<Metatile<Rgba32>> &metatiles, LayerMode mode) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, extrinsic_transparency, tileset_scope_, void);

    bool hit_error = false;
    std::size_t metatile_index = 0;

    for (const auto &metatile : metatiles) {
        // Determine the implied layer mode for this metatile
        // Check each of the 4 tile positions (northwest, northeast, southwest, southeast)
        bool found_triple_layer_region = false;

        for (std::size_t subtile_idx = 0; subtile_idx < metatile::tiles_per_metatile_layer; ++subtile_idx) {
            const auto &bottom_tile = metatile.bottom(subtile_idx);
            const auto &middle_tile = metatile.middle(subtile_idx);
            const auto &top_tile = metatile.top(subtile_idx);
            const auto subtile = metatile::subtile_from_index(subtile_idx);

            // Check if each layer has at least one non-transparent pixel in this tile position
            const bool bottom_has_opaque = !bottom_tile.is_transparent(extrinsic_transparency.value());
            const bool middle_has_opaque = !middle_tile.is_transparent(extrinsic_transparency.value());
            const bool top_has_opaque = !top_tile.is_transparent(extrinsic_transparency.value());

            // If all three layers have opaque pixels in this tile position, this is a triple-layer region
            if (bottom_has_opaque && middle_has_opaque && top_has_opaque) {
                found_triple_layer_region = true;
            }

            const LayerMode implied_mode = found_triple_layer_region ? LayerMode::triple : LayerMode::dual;

            // Error condition if implied mode is triple for a dual-layer compilation
            if (implied_mode == LayerMode::triple && mode == LayerMode::dual) {
                hit_error = true;
                std::vector errors = {format_->format(
                    "{}: {}",
                    FormatParam{metatile::message_header(*format_, metatile_index, subtile), Style::bold},
                    FormatParam{"non-transparent content on all three layers"})};
                errors.emplace_back("");
                // TODO: create and use print_metatile_tile_highlight
                std::vector bottom_highlight = tile_printer_->print_tile(bottom_tile, extrinsic_transparency);
                std::vector middle_highlight = tile_printer_->print_tile(middle_tile, extrinsic_transparency);
                std::vector top_highlight = tile_printer_->print_tile(top_tile, extrinsic_transparency);
                std::ranges::copy(bottom_highlight, std::back_inserter(errors));
                errors.emplace_back("");
                std::ranges::copy(middle_highlight, std::back_inserter(errors));
                errors.emplace_back("");
                std::ranges::copy(top_highlight, std::back_inserter(errors));
                diag_->err(layer_mode_violation, errors);
            }
        }

        metatile_index++;
    }

    if (hit_error) {
        return FormattableError{
            "{}: found metatile(s) with mismatched implied layer mode", FormatParam{layer_mode_violation, Style::bold}};
    }

    return {};
}

ChainableResult<void> MetatileValidator::validate_primary(const std::vector<Metatile<Rgba32>> &metatiles) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_pals_primary, tileset_scope_, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_metatiles_primary, tileset_scope_, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, num_tiles_per_metatile, tileset_scope_, void);

    auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile);

    std::vector<std::string> error_messages;

    // Run metatile count validation
    if (metatiles.size() > num_metatiles_primary) {
        diag_->err(
            metatile_limit_exceeded,
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
        diag_->note(metatile_limit_exceeded, note_text);

        // Push back error message to fatal chain
        error_messages.push_back(format_->format(
            "{}: found '{}', limit is '{}'",
            FormatParam{metatile_limit_exceeded, Style::bold},
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{num_metatiles_primary, Style::bold}));
    }

    // Run alpha channel validation
    const auto alpha_result = validate_alpha_channels(metatiles);
    if (!alpha_result.has_value()) {
        error_messages.push_back(alpha_result.error().join(*format_));
    }

    // Run tile color count validation
    const auto tile_color_result = validate_tile_color_count(metatiles);
    if (!tile_color_result.has_value()) {
        error_messages.push_back(tile_color_result.error().join(*format_));
    }

    // Run global color count validation
    std::size_t color_count_limit = num_pals_primary.value() * (pal::max_size - 1);
    const auto global_color_result = validate_global_color_count(metatiles, color_count_limit);
    if (!global_color_result.has_value()) {
        // Construct note text
        std::vector<std::string> note_text;
        note_text.push_back(format_->format(
            "unique color count limit is '{}' due to configuration", FormatParam{color_count_limit, Style::bold}));
        note_text.emplace_back("");
        note_text.emplace_back("Color limit definition:");
        note_text.push_back(format_->format(
            "{} * {}:",
            FormatParam{num_pals_primary.name(), Style::bold | Style::yellow},
            FormatParam{"nontransparent_colors_per_pal", Style::bold}));
        note_text.push_back(format_->format(
            "{} * {} = {}",
            FormatParam{num_pals_primary.value(), Style::bold | Style::yellow},
            FormatParam{(pal::max_size - 1), Style::bold},
            FormatParam{color_count_limit, Style::bold}));
        note_text.emplace_back("");
        std::ranges::copy(num_pals_primary.prettify(*format_), std::back_inserter(note_text));

        // Emit note
        diag_->note(global_color_count_violation, note_text);

        // Append fatal message
        error_messages.push_back(global_color_result.error().join(*format_));
    }

    // Run layer mode validation if we're dual-layer
    if (configured_layer_mode == LayerMode::dual) {
        const auto layer_mode_result = validate_layer_mode(metatiles, configured_layer_mode);
        if (!layer_mode_result.has_value()) {
            // Construct note text
            std::vector<std::string> note_text;
            note_text.push_back(format_->format(
                "implied layer mode is '{}' due to configuration", FormatParam{LayerMode::dual, Style::bold}));
            note_text.emplace_back("");
            std::ranges::copy(num_tiles_per_metatile.prettify(*format_), std::back_inserter(note_text));
            note_text.emplace_back("");
            note_text.emplace_back("Consider enabling triple-layer metatiles.");
            note_text.push_back(format_->format(
                "To enable layer mode '{}' for your project:", FormatParam{LayerMode::triple, Style::bold}));
            note_text.push_back(format_->format(
                "   - set '{}' = '{}'",
                FormatParam{num_tiles_per_metatile.name(), Style::bold},
                FormatParam{metatile::entries_per_metatile_triple, Style::bold}));
            note_text.push_back(format_->format(
                "   - follow the steps here: {}",
                FormatParam{"https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles", Style::underline}));

            // Emit note
            diag_->note(layer_mode_violation, note_text);

            // Append fatal message
            error_messages.push_back(layer_mode_result.error().join(*format_));
        }
    }

    // Run precision loss warning generation
    const auto precision_result = generate_precision_loss_warnings(metatiles);
    if (!precision_result.has_value()) {
        error_messages.push_back(precision_result.error().join(*format_));
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
