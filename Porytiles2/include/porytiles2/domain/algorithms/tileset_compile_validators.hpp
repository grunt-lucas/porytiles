#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/services/palette_printer.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/filesystem_utils.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

/**
 * @file tileset_compile_validators.hpp
 * @brief Validation functions for tileset compile job input.
 *
 * @details
 * This module provides validation functionality for tileset compile job input.
 *
 * @see TilesetCompiler Main consumer of the functions defined here.
 */

namespace porytiles2 {

/**
 * @brief Parameter store for common services used by tileset compile validators.
 *
 * @details
 * This struct aggregates references to all services required by validation functions in this module. It enables a
 * consistent interface across validators without requiring each function to accept many individual parameters.
 *
 * @invariant All references must remain valid for the lifetime of any validation operation using this struct.
 */
struct TilesetCompileValidatorServices {
    const DomainConfig &config;
    const UserDiagnostics &diag;
    const TilePrinter &tile_printer;
    const PalettePrinter &pal_printer;
};

namespace details {

/**
 * @brief Reports a validation error at a specific pixel location within a metatile.
 *
 * @details
 * Emits a formatted error diagnostic that includes the metatile location, layer, subtile position, and the specific
 * pixel coordinates where the error occurred. The diagnostic includes a visual highlight of the problematic pixel
 * within the metatile context.
 *
 * @param services Common services for formatting and diagnostic output.
 * @param metatile The metatile containing the validation error.
 * @param metatile_index Zero-based index of the metatile in the input sequence.
 * @param internal_tile_index Index of the tile within the decomposed metatile (0-11 for triple layer).
 * @param row Row position of the pixel within the tile (0-7).
 * @param col Column position of the pixel within the tile (0-7).
 * @param diagnostic_code Unique identifier for this type of diagnostic.
 * @param error_message Human-readable description of the validation error.
 * @param extrinsic_transparency The transparency color used for rendering the highlight.
 */
inline void report_validation_error_in_metatile(
    const TilesetCompileValidatorServices &services,
    const Metatile<Rgba32> &metatile,
    std::size_t metatile_index,
    std::size_t internal_tile_index,
    std::size_t row,
    std::size_t col,
    const std::string &diagnostic_code,
    const std::string &error_message,
    const Rgba32 &extrinsic_transparency)
{
    auto [layer, subtile] = metatile::from_internal_tile_index(internal_tile_index);
    std::vector errors = {services.diag.formatter().format(
        "{}: {}",
        FormatParam{
            metatile::message_header(services.diag.formatter(), metatile_index, layer, subtile, row, col), Style::bold},
        FormatParam{error_message})};
    std::vector highlight = services.tile_printer.print_metatile_pixel_highlight(
        metatile, layer, subtile, row, col, extrinsic_transparency);
    errors.append_range(highlight);
    services.diag.error(diagnostic_code, errors);
}

/**
 * @brief Reports a validation error at a specific pixel location within an animation frame tile.
 *
 * @details
 * Emits a formatted error diagnostic that includes the animation name, frame name, tile index, and the specific pixel
 * coordinates where the error occurred. The diagnostic includes a visual highlight of the problematic pixel within
 * the tile context.
 *
 * @param services Common services for formatting and diagnostic output.
 * @param tile The pixel tile containing the validation error.
 * @param anim_name Name of the animation containing the error.
 * @param frame_name Name of the frame within the animation.
 * @param internal_tile_index Index of the tile within the frame's tile array.
 * @param row Row position of the pixel within the tile (0-7).
 * @param col Column position of the pixel within the tile (0-7).
 * @param diagnostic_code Unique identifier for this type of diagnostic.
 * @param error_message Human-readable description of the validation error.
 * @param extrinsic_transparency The transparency color used for rendering the highlight.
 */
inline void report_validation_error_in_anim(
    const TilesetCompileValidatorServices &services,
    const PixelTile<Rgba32> &tile,
    const std::string &anim_name,
    const std::string &frame_name,
    std::size_t internal_tile_index,
    std::size_t row,
    std::size_t col,
    const std::string &diagnostic_code,
    const std::string &error_message,
    const Rgba32 &extrinsic_transparency)
{
    std::vector errors = {services.diag.formatter().format(
        "{}: {}",
        FormatParam{
            anim::message_header(services.diag.formatter(), anim_name, frame_name, internal_tile_index, row, col),
            Style::bold},
        FormatParam{error_message})};
    std::vector highlight = services.tile_printer.print_tile_pixel_highlight(tile, row, col, extrinsic_transparency);
    errors.append_range(highlight);
    services.diag.error(diagnostic_code, errors);
}

/**
 * @brief Reports a validation error for an entire animation frame tile.
 *
 * @details
 * Emits a formatted error diagnostic that includes the animation name, frame name, and tile index. Unlike
 * report_validation_error_in_anim which highlights a specific pixel, this function displays the entire tile visual.
 * Use this for tile-level errors like transparency violations or color count violations.
 *
 * @param services Common services for formatting and diagnostic output.
 * @param tile The pixel tile containing the validation error.
 * @param anim_name Name of the animation containing the error.
 * @param frame_name Name of the frame within the animation.
 * @param internal_tile_index Index of the tile within the frame's tile array.
 * @param diagnostic_code Unique identifier for this type of diagnostic.
 * @param error_message Human-readable description of the validation error.
 * @param extrinsic_transparency The transparency color used for rendering.
 */
inline void report_validation_error_in_anim_tile(
    const TilesetCompileValidatorServices &services,
    const PixelTile<Rgba32> &tile,
    const std::string &anim_name,
    const std::string &frame_name,
    std::size_t internal_tile_index,
    const std::string &diagnostic_code,
    const std::string &error_message,
    const Rgba32 &extrinsic_transparency)
{
    std::vector errors = {services.diag.formatter().format(
        "{}: {}",
        FormatParam{
            anim::message_header(services.diag.formatter(), anim_name, frame_name, internal_tile_index), Style::bold},
        FormatParam{error_message})};
    std::vector tile_visual = services.tile_printer.print_tile(tile, extrinsic_transparency);
    errors.append_range(tile_visual);
    services.diag.error(diagnostic_code, errors);
}

/**
 * @brief Emits a diagnostic note displaying the count of each unique color.
 *
 * @details
 * Formats and prints a color count summary as a note diagnostic. This is typically used as a follow-up to a color
 * count violation error to help users understand which colors are present in the problematic tile or metatile.
 *
 * @param tag Diagnostic code to associate with this note.
 * @param services Common services for formatting and diagnostic output.
 * @param color_counts Map of colors to their occurrence counts.
 */
inline void report_color_counts(
    const std::string &tag,
    const TilesetCompileValidatorServices &services,
    const std::map<Rgba32, unsigned int> &color_counts)
{
    std::vector<std::string> color_lines;
    color_lines.emplace_back("color counts:");
    auto counts = services.pal_printer.print_rgba_pal_counts(color_counts);
    color_lines.append_range(counts);
    services.diag.error_note(tag, color_lines);
}

} // namespace details

/**
 * @brief Validates that the metatile count does not exceed the configured limit.
 *
 * @details
 * Checks that the number of input metatiles does not exceed the configured limit for the tileset type. For primary
 * tilesets, this checks against @c num_metatiles_in_primary. For secondary tilesets, this checks against
 * @c num_metatiles_total - @c num_metatiles_in_primary. If validation fails, emits an error diagnostic with the actual
 * count and limit, along with a note showing the relevant configuration source(s).
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param is_secondary Whether this is a secondary tileset (determines which limit to use).
 * @param metatiles The metatiles to validate.
 * @return Empty result on success, or FormattableError describing the violation.
 *
 * @see TilesetCompiler Main consumer of this validation function.
 */
[[nodiscard]] inline ChainableResult<void> validate_metatile_count(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    bool is_secondary,
    const std::vector<Metatile<Rgba32>> &metatiles)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, num_metatiles_in_primary, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, num_metatiles_total, tileset_name, void);

    std::size_t metatile_limit = is_secondary ? (num_metatiles_total.value() - num_metatiles_in_primary.value())
                                              : num_metatiles_in_primary.value();

    if (metatiles.size() > metatile_limit) {
        services.diag.error(
            "metatile-limit-exceeded",
            "Too many metatiles ({}) in Porytiles component for tileset '{}'.",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{tileset_name, Style::bold});

        std::vector<std::string> note_text;
        if (is_secondary) {
            note_text.append_range(build_subtraction_limit_lines(
                services.diag.formatter(),
                "Metatile limit",
                metatile_limit,
                num_metatiles_total,
                num_metatiles_in_primary));
        }
        else {
            note_text.push_back(
                services.diag.formatter().format("Metatile limit is '{}'.", FormatParam{metatile_limit, Style::bold}));
            note_text.emplace_back("");
            note_text.append_range(format_config_note(services.diag.formatter(), num_metatiles_in_primary));
        }
        services.diag.error_note("metatile-limit-exceeded", note_text);

        return FormattableError{
            "Found '{}' metatiles, limit is '{}'.",
            FormatParam{metatiles.size(), Style::bold},
            FormatParam{metatile_limit, Style::bold}};
    }

    return {};
}

/**
 * @brief Validates a Porymap palette for correctness according to GBA hardware constraints.
 *
 * @details
 * Performs two checks on an existing Porymap palette:
 *
 * 1. **Slot 0 transparency check (warning)**: Slot 0 should match the configured extrinsic transparency color, as it is
 *    typically reserved for transparent pixels. If slot 0 contains a different color, a warning is emitted (this may be
 *    intentional for .pla blend color usage).
 *
 * 2. **Non-slot-0 transparency check (error)**: Slots 1-15 must not contain the extrinsic transparency color, as this
 *    would cause rendering issues where non-transparent pixels become transparent.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param pal The Porymap palette to validate.
 * @param pal_index The index of this palette (0-12 typically), used for diagnostic messages.
 * @return Empty result on success, or FormattableError if extrinsic transparency appears in non-slot-0 positions.
 */
[[nodiscard]] inline ChainableResult<void> validate_porymap_pal(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const Palette<Rgba32, pal::max_size> &pal,
    std::size_t pal_index)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);

    bool hit_error = false;
    const std::string filename = pal_filename(pal_index);
    std::vector<std::size_t> violating_slots{};

    if (pal.is_wildcard(0)) {
        panic("unexpected wildcard in Porymap palette slot 0");
    }

    // Check 1: Slot 0 should match extrinsic transparency (warning only)
    const Rgba32 slot0_color = pal.slot_zero_color();
    if (!slot0_color.is_extrinsically_transparent(extrinsic_transparency)) {
        std::vector<std::string> warning_lines;
        warning_lines.emplace_back(services.diag.formatter().format(
            "Porymap palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'",
            FormatParam{filename, Style::bold},
            FormatParam{slot0_color.to_jasc_str(), Style::bold},
            FormatParam{extrinsic_transparency.value().to_jasc_str(), Style::bold}));
        warning_lines.emplace_back("Slot 0 is typically reserved for the transparency color.");
        warning_lines.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
        services.diag.warning("porymap-palette-slot-0", warning_lines);

        services.diag.warning_note(
            "porymap-palette-slot-0",
            build_porymap_pal_highlight_lines(
                services.diag.formatter(),
                services.pal_printer,
                "reserved transparency slot",
                pal,
                filename,
                std::vector<std::size_t>{0}));
        services.diag.warning_note(
            "porymap-palette-slot-0", format_config_note(services.diag.formatter(), extrinsic_transparency));
    }

    // Check 2: Non-slot-0 positions cannot contain extrinsic transparency
    for (std::size_t slot = 1; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            panic("unexpected wildcard in Porymap palette");
        }
        const Rgba32 color = pal.at(slot);
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(services.diag.formatter().format(
                "Porymap palette '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in non-slot-0 positions.");
            services.diag.error("porymap-palette-transparency", error_lines);
        }
    }

    if (hit_error) {
        // Print the palette with violating slots highlighted
        services.diag.error_note(
            "porymap-palette-transparency",
            build_porymap_pal_highlight_lines(
                services.diag.formatter(),
                services.pal_printer,
                "slots with invalid extrinsic transparency",
                pal,
                filename,
                violating_slots));
        services.diag.error_note(
            "porymap-palette-transparency", format_config_note(services.diag.formatter(), extrinsic_transparency));

        return FormattableError{"Validation failed for Porymap palette '{}'.", FormatParam{filename, Style::bold}};
    }

    return {};
}

/**
 * @brief Validates a user-specified Porytiles override palette for correctness.
 *
 * @details
 * Performs two checks on a Porytiles override palette:
 *
 * 1. **Slot 0 transparency check (warning)**: If slot 0 is not a wildcard, it should match the configured extrinsic
 *    transparency color. A warning is emitted if it doesn't (this may be intentional for .pla blend color usage).
 *
 * 2. **Non-slot-0 transparency check (error)**: Non-wildcard slots 1-15 must not contain the extrinsic transparency
 *    color, as this would cause rendering issues.
 *
 * Unlike Porymap palettes, Porytiles palettes may contain wildcards ('*' entries) that are filled in during the
 * palette assignment process. Wildcard slots are skipped during validation.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param pal The Porytiles override palette to validate.
 * @param pal_index The index of this palette (0-12 typically), used for diagnostic messages.
 * @return Empty result on success, or FormattableError if extrinsic transparency appears in non-slot-0 positions.
 */
[[nodiscard]] inline ChainableResult<void> validate_porytiles_pal(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const Palette<Rgba32, pal::max_size> &pal,
    std::size_t pal_index)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);

    bool hit_error = false;
    const std::string filename = pal_filename(pal_index);
    std::vector<std::size_t> violating_slots;

    // Check 1: Slot 0 should match extrinsic transparency (warning only)
    if (!pal.is_wildcard(0)) {
        const Rgba32 slot0_color = pal.slot_zero_color();
        if (!slot0_color.is_extrinsically_transparent(extrinsic_transparency)) {
            // Build the warning text
            std::vector<std::string> warning_text;
            warning_text.emplace_back(services.diag.formatter().format(
                "Porytiles palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot0_color.to_jasc_str(), Style::bold},
                FormatParam{extrinsic_transparency.value().to_jasc_str(), Style::bold}));
            warning_text.emplace_back("Slot 0 is typically reserved for the transparency color.");
            warning_text.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
            services.diag.warning("porytiles-palette-slot-0", warning_text);

            // Print the palette and config notes.
            services.diag.warning_note(
                "porytiles-palette-slot-0",
                build_porytiles_pal_highlight_lines(
                    services.diag.formatter(),
                    services.pal_printer,
                    "reserved transparency slot",
                    pal,
                    filename,
                    std::vector<std::size_t>{0}));
            services.diag.warning_note(
                "porytiles-palette-slot-0", format_config_note(services.diag.formatter(), extrinsic_transparency));
        }
    }

    // Check 2: Non-slot-0 positions cannot contain extrinsic transparency
    for (std::size_t slot = 1; slot < pal.size(); ++slot) {
        if (pal.is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = pal.at(slot);
        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(services.diag.formatter().format(
                "Porytiles palette '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{filename, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in non-slot-0 positions.");
            services.diag.error("porytiles-palette-transparency", error_lines);
        }
    }

    if (hit_error) {
        // Print the palette and config notes
        services.diag.error_note(
            "porytiles-palette-transparency",
            build_porytiles_pal_highlight_lines(
                services.diag.formatter(),
                services.pal_printer,
                "slots with invalid extrinsic transparency",
                pal,
                filename,
                violating_slots));
        services.diag.error_note(
            "porytiles-palette-transparency", format_config_note(services.diag.formatter(), extrinsic_transparency));

        return FormattableError{"Validation failed for Porytiles palette '{}'.", FormatParam{filename, Style::bold}};
    }

    return {};
}

/**
 * @brief Validates a user-specified palette hint for correctness.
 *
 * @details
 * Performs three checks on a palette hint:
 *
 * 1. **Size check (error)**: Palette hints must have at most 15 colors (pal::max_size - 1), since slot 0 is reserved
 *    for transparency and hints don't include the transparency slot.
 *
 * 2. **Extrinsic transparency check (error)**: Palette hints must not contain the extrinsic transparency color at any
 *    position, as hints are meant to specify opaque colors that should be grouped together.
 *
 * 3. **Duplicate color check (error)**: Palette hints must not contain duplicate colors, as each color can only appear
 *    once in a palette.
 *
 * Palette hints are user-provided color groupings that guide the palette assignment algorithm to place certain colors
 * together in the same palette. Unlike full palettes, hints don't specify an index and may not include all 15 slots.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param hint The palette hint to validate.
 * @return Empty result on success, or FormattableError if any validation check fails.
 */
[[nodiscard]] inline ChainableResult<void> validate_pal_hint(
    const TilesetCompileValidatorServices &services, const std::string &tileset_name, const PaletteHint &hint)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);

    bool hit_any_error = false;
    const std::string &hint_name = hint.name();

    if (hint.pal().size() >= pal::max_size) {
        services.diag.error(
            "palette-hint-size-violation",
            services.diag.formatter().format(
                "palette hint '{}' has size '{}', max allowed size is '{}'",
                FormatParam{hint_name, Style::bold},
                FormatParam{hint.pal().size(), Style::bold},
                FormatParam{pal::max_size - 1, Style::bold}));
        services.diag.error_note(
            "palette-hint-size-violation",
            build_pal_hint_highlight_lines(
                services.diag.formatter(),
                services.pal_printer,
                "invalid extra slots start here",
                hint,
                hint_name,
                std::vector{pal::max_size}));
        return FormattableError{"Validation failed for palette hint '{}'.", FormatParam{hint_name, Style::bold}};
    }

    std::set<Rgba32> seen_colors{};
    std::vector<std::size_t> violating_slots{};

    // Check 1: Extrinsic transparency not allowed in hints
    for (std::size_t slot = 0; slot < hint.pal().size(); ++slot) {
        if (hint.pal().is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = hint.pal().at(slot);

        if (color.is_extrinsically_transparent(extrinsic_transparency)) {
            hit_any_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(services.diag.formatter().format(
                "palette hint '{}' slot '{}' contains extrinsic transparency color '{}'",
                FormatParam{hint_name, Style::bold},
                FormatParam{slot, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold}));
            error_lines.emplace_back("Extrinsic transparency is not allowed in palette hints.");
            services.diag.error("palette-hint-transparency", error_lines);
        }
    }
    if (!violating_slots.empty()) {
        services.diag.error_note(
            "palette-hint-transparency",
            build_pal_hint_highlight_lines(
                services.diag.formatter(),
                services.pal_printer,
                "slots with invalid extrinsic transparency",
                hint,
                hint_name,
                violating_slots));
        services.diag.error_note(
            "palette-hint-transparency", format_config_note(services.diag.formatter(), extrinsic_transparency));
    }
    violating_slots.clear();

    // Check 2: No duplicate colors
    for (std::size_t slot = 0; slot < hint.pal().size(); ++slot) {
        if (hint.pal().is_wildcard(slot)) {
            continue;
        }
        const Rgba32 color = hint.pal().at(slot);

        if (seen_colors.contains(color)) {
            hit_any_error = true;
            violating_slots.push_back(slot);
            std::vector<std::string> error_lines;
            error_lines.emplace_back(services.diag.formatter().format(
                "palette hint '{}' contains duplicate color '{}' at slot '{}'",
                FormatParam{hint_name, Style::bold},
                FormatParam{color.to_jasc_str(), Style::bold},
                FormatParam{slot, Style::bold}));
            error_lines.emplace_back("Duplicate colors are not allowed in palette hints.");
            services.diag.error("palette-hint-duplicate-color", error_lines);
        }
        seen_colors.insert(color);
    }
    if (!violating_slots.empty()) {
        services.diag.error_note(
            "palette-hint-duplicate-color",
            build_pal_hint_highlight_lines(
                services.diag.formatter(),
                services.pal_printer,
                "slots with invalid duplicate colors",
                hint,
                hint_name,
                violating_slots));
    }

    if (hit_any_error) {
        return FormattableError{"Validation failed for palette hint '{}'.", FormatParam{hint_name, Style::bold}};
    }
    return {};
}

/**
 * @brief Validates that all pixel alpha channels in provided input have valid values.
 *
 * @details
 * In Porytiles, only alpha values of 0 (fully transparent) or 255 (fully opaque) are valid. Any intermediate alpha
 * value indicates partial transparency, which is not supported by the GBA hardware. This function iterates through all
 * pixels in both metatiles and animation frames, reporting each invalid alpha channel with a visual highlight of the
 * problematic pixel.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param metatiles The metatiles to validate for alpha channel correctness.
 * @param anims The animations to validate for alpha channel correctness.
 * @return Empty result on success, or FormattableError if any pixel has an invalid alpha value.
 */
[[nodiscard]] inline ChainableResult<void> validate_alpha_channels(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const std::vector<Metatile<Rgba32>> &metatiles,
    const std::map<std::string, Animation<Rgba32>> &anims)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);

    bool hit_error = false;

    // Validate metatiles
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto decomposed_metatile = metatile.decompose();

        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < decomposed_metatile.size();
             ++internal_tile_index) {
            const auto &tile = decomposed_metatile[internal_tile_index];

            // Iterate over each pixel in the current internal tile
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                        hit_error = true;
                        std::string error_message = services.diag.formatter().format(
                            "invalid alpha channel: {}", FormatParam{std::to_string(pixel.alpha()), Style::bold});
                        details::report_validation_error_in_metatile(
                            services,
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "alpha-channel-violation",
                            error_message,
                            extrinsic_transparency);
                    }
                }
            }
        }
        metatile_index++;
    }

    // Lambda to avoid duplicating logic
    auto validate_anim_frame = [&services, &hit_error, &extrinsic_transparency](
                                   const std::string &anim_name, const AnimFrame<Rgba32> &frame) -> void {
        const auto &frame_tiles = frame.tiles();

        // Loop over each internal frame tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < frame_tiles.size(); ++internal_tile_index) {
            const auto &tile = frame_tiles.at(internal_tile_index);

            // loop over tile pixels, row-major
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);

                    // validate
                    if (pixel.alpha() != Rgba32::alpha_opaque && pixel.alpha() != Rgba32::alpha_transparent) {
                        hit_error = true;
                        std::string error_message = services.diag.formatter().format(
                            "invalid alpha channel: {}", FormatParam{std::to_string(pixel.alpha()), Style::bold});
                        details::report_validation_error_in_anim(
                            services,
                            tile,
                            anim_name,
                            frame.frame_name(),
                            internal_tile_index,
                            row,
                            col,
                            "alpha-channel-violation",
                            error_message,
                            extrinsic_transparency);
                    }
                }
            }
        }
    };

    // Validate animations
    for (const auto &[anim_name, anim] : anims) {
        if (anim.has_key_frame()) {
            const auto &key_frame = anim.key_frame();
            validate_anim_frame(anim_name, key_frame);
        }
        for (const AnimFrame<Rgba32> &frame : anim.frames() | std::views::values) {
            validate_anim_frame(anim_name, frame);
        }
    }

    if (hit_error) {
        return FormattableError{"Found input pixel(s) with invalid alpha channel value(s)."};
    }

    return {};
}

/**
 * @brief Validates that metatiles conform to the configured layer mode.
 *
 * @details
 * Checks each metatile to ensure its implied layer mode matches the configured layer mode. The implied layer mode is
 * determined by examining whether all three layers (bottom, middle, top) contain non-transparent content at any given
 * subtile position. If a metatile requires triple-layer rendering but the configuration specifies dual-layer mode, an
 * error is reported with visual highlights showing which subtile positions have content on all three layers.
 *
 * This validation is skipped entirely if the configured layer mode is already triple, since triple-layer mode can
 * accommodate any metatile configuration.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param metatiles The metatiles to validate for layer mode compliance.
 * @return Empty result on success, or FormattableError if any metatile requires triple-layer mode in a dual-layer
 *         configuration.
 *
 * @note If validation fails, a note is emitted with instructions for enabling triple-layer mode.
 */
[[nodiscard]] inline ChainableResult<void> validate_layer_mode(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const std::vector<Metatile<Rgba32>> &metatiles)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, num_tiles_per_metatile, tileset_name, void);
    auto configured_layer_mode = layer_mode_from_val(num_tiles_per_metatile);

    // If layer mode is triple, just return
    if (configured_layer_mode == LayerMode::triple) {
        return {};
    }

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
            if (implied_mode == LayerMode::triple && configured_layer_mode == LayerMode::dual) {
                hit_error = true;
                std::vector errors = {services.diag.formatter().format(
                    "{}: {}",
                    FormatParam{
                        metatile::message_header(services.diag.formatter(), metatile_index, subtile), Style::bold},
                    FormatParam{"non-transparent content on all three layers"})};
                std::vector bottom_highlight = services.tile_printer.print_metatile_tile_highlight(
                    metatile, metatile::Layer::bottom, subtile, extrinsic_transparency);
                std::vector middle_highlight = services.tile_printer.print_metatile_tile_highlight(
                    metatile, metatile::Layer::middle, subtile, extrinsic_transparency);
                std::vector top_highlight = services.tile_printer.print_metatile_tile_highlight(
                    metatile, metatile::Layer::top, subtile, extrinsic_transparency);
                errors.append_range(bottom_highlight);
                errors.append_range(middle_highlight);
                errors.append_range(top_highlight);
                services.diag.error("layer-mode-violation", errors);
            }
        }

        metatile_index++;
    }

    if (hit_error) {
        std::vector<std::string> note_text;
        note_text.push_back(
            services.diag.formatter().format("Implied layer mode is '{}'.", FormatParam{LayerMode::dual, Style::bold}));
        note_text.emplace_back("");
        note_text.append_range(format_config_note(services.diag.formatter(), num_tiles_per_metatile));
        note_text.emplace_back("");
        note_text.emplace_back("Consider enabling triple-layer metatiles.");
        note_text.push_back(services.diag.formatter().format(
            "To enable layer mode '{}' for your project:", FormatParam{LayerMode::triple, Style::bold}));
        note_text.push_back(services.diag.formatter().format(
            "   - set '{}' = '{}'",
            FormatParam{num_tiles_per_metatile.canonical_name(), Style::bold},
            FormatParam{metatile::entries_per_metatile_triple, Style::bold}));
        note_text.push_back(services.diag.formatter().format(
            "   - follow the steps here: {}",
            FormatParam{"https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles", Style::underline}));

        // Emit note
        services.diag.error_note("layer-mode-violation", note_text);

        return FormattableError{"Found metatile(s) with mismatched implied layer mode."};
    }

    return {};
}

/**
 * @brief Validates that each individual tile does not exceed the per-tile color limit.
 *
 * @details
 * Each 8x8 tile on GBA hardware can reference at most 16 palette slots, with slot 0 reserved for transparency. This
 * means each tile can have at most 15 unique non-transparent colors. This function iterates through all tiles in both
 * metatiles and animation frames, counting unique colors per tile. When a tile exceeds the limit, an error is reported
 * showing the pixel that triggered the violation, along with a note listing all colors found in that tile.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param metatiles The metatiles to validate for per-tile color count compliance.
 * @param anims The animations to validate for per-tile color count compliance.
 * @return Empty result on success, or FormattableError if any tile exceeds the color limit.
 *
 * @note Both extrinsic transparency and intrinsic transparency (alpha=0) are excluded from color counting.
 */
[[nodiscard]] inline ChainableResult<void> validate_tile_color_count(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const std::vector<Metatile<Rgba32>> &metatiles,
    const std::map<std::string, Animation<Rgba32>> &anims)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);

    bool hit_error = false;
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto decomposed_metatile = metatile.decompose();

        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < decomposed_metatile.size();
             ++internal_tile_index) {
            const auto &tile = decomposed_metatile[internal_tile_index];
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
                        std::string error_message = services.diag.formatter().format(
                            "found {}th unique tile color: {}",
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold});
                        details::report_validation_error_in_metatile(
                            services,
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "tile-color-count-violation",
                            error_message,
                            extrinsic_transparency);
                        details::report_color_counts("tile-color-count-violation", services, color_counts);
                        goto next_tile;
                    }
                }
            }
        next_tile:;
        }
        metatile_index++;
    }

    auto validate_anim_frame = [&services, &hit_error, &extrinsic_transparency](
                                   const std::string &anim_name, const AnimFrame<Rgba32> &frame) -> void {
        const auto &frame_tiles = frame.tiles();
        // Iterate over each internal tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < frame_tiles.size(); ++internal_tile_index) {
            const auto &tile = frame_tiles[internal_tile_index];
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
                        std::string error_message = services.diag.formatter().format(
                            "found {}th unique frame tile color: {}",
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold});
                        details::report_validation_error_in_anim(
                            services,
                            tile,
                            anim_name,
                            frame.frame_name(),
                            internal_tile_index,
                            row,
                            col,
                            "tile-color-count-violation",
                            error_message,
                            extrinsic_transparency);
                        details::report_color_counts("tile-color-count-violation", services, color_counts);
                        goto next_tile;
                    }
                }
            }
        next_tile:;
        }
    };

    // Validate animations
    for (const auto &[anim_name, anim] : anims) {
        if (anim.has_key_frame()) {
            const auto &key_frame = anim.key_frame();
            validate_anim_frame(anim_name, key_frame);
        }
        for (const AnimFrame<Rgba32> &frame : anim.frames() | std::views::values) {
            validate_anim_frame(anim_name, frame);
        }
    }

    if (hit_error) {
        return FormattableError{
            "Found tile(s) with more than {} unique non-transparent pixels.", FormatParam{pal::max_size - 1}};
    }

    return {};
}

/**
 * @brief Validates that the total unique color count across all input does not exceed the global limit.
 *
 * @details
 * The global color limit is determined by the number of available palettes times 15 colors per palette (slot 0 is
 * reserved for transparency). This function counts all unique colors across metatiles, animations, Porytiles override
 * palettes, and palette hints. When the global limit is exceeded, an error is reported showing the pixel that triggered
 * the violation, along with a complete color count summary.
 *
 * Additionally, this function warns about unused colors in Porytiles override palettes and palette hints, helping users
 * identify manual color specifications that don't correspond to any colors in the actual input assets.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param is_secondary Whether this is a secondary tileset (determines which palette limit to use).
 * @param metatiles The metatiles to include in global color counting.
 * @param anims The animations to include in global color counting.
 * @param porytiles_pals User-specified Porytiles override palettes (may contain wildcards).
 * @param hints User-specified palette hints for color grouping.
 * @return Empty result on success, or FormattableError if the global color limit is exceeded.
 *
 * @note Colors in Porytiles palettes and hints that don't appear in input assets trigger warnings.
 */
[[nodiscard]] ChainableResult<void> inline validate_global_color_count(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    bool is_secondary,
    const std::vector<Metatile<Rgba32>> &metatiles,
    const std::map<std::string, Animation<Rgba32>> &anims,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &porytiles_pals,
    const std::vector<PaletteHint> &hints)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, num_pals_in_primary, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, num_pals_total, tileset_name, void);

    ConfigValue<std::size_t> num_pals_cfg = is_secondary ? num_pals_total : num_pals_in_primary;
    std::size_t count_max = num_pals_cfg.value() * (pal::max_size - 1);

    std::map<Rgba32, unsigned int> color_counts{};
    bool hit_first_violation = false;

    // Count colors in the input metatiles
    std::size_t metatile_index = 0;
    for (const auto &metatile : metatiles) {
        const auto decomposed_metatile = metatile.decompose();
        for (std::size_t internal_tile_index = 0; internal_tile_index < decomposed_metatile.size();
             ++internal_tile_index) {
            const auto &tile = decomposed_metatile[internal_tile_index];
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);
                    if (!pixel.is_transparent(extrinsic_transparency)) {
                        color_counts[pixel]++;
                    }

                    /*
                     * Print the first violation only, but we'll keep counting across the rest of the metatiles so we
                     * can give a final tally.
                     */
                    if (color_counts.size() > count_max && !hit_first_violation) {
                        hit_first_violation = true;
                        std::string error_message = services.diag.formatter().format(
                            "found {}th globally unique color: {}",
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold});
                        details::report_validation_error_in_metatile(
                            services,
                            metatile,
                            metatile_index,
                            internal_tile_index,
                            row,
                            col,
                            "global-color-count-violation",
                            error_message,
                            extrinsic_transparency);
                    }
                }
            }
        }
        metatile_index++;
    }

    // Count colors in the input anims
    auto validate_anim_frame = [&services, &hit_first_violation, &color_counts, &count_max, &extrinsic_transparency](
                                   const std::string &anim_name, const AnimFrame<Rgba32> &frame) -> void {
        const auto &frame_tiles = frame.tiles();
        // Loop over each internal frame tile
        for (std::size_t internal_tile_index = 0; internal_tile_index < frame_tiles.size(); ++internal_tile_index) {
            const auto &tile = frame_tiles.at(internal_tile_index);

            // loop over tile pixels, row-major
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const auto &pixel = tile.at(row, col);

                    if (!pixel.is_transparent(extrinsic_transparency)) {
                        color_counts[pixel]++;
                    }

                    /*
                     * Print the first violation only, but we'll keep counting across the rest of the anims so we can
                     * give a final tally.
                     */
                    if (color_counts.size() > count_max && !hit_first_violation) {
                        hit_first_violation = true;
                        std::string error_message = services.diag.formatter().format(
                            "found {}th globally unique color: {}",
                            FormatParam{pal::max_size},
                            FormatParam{pixel.to_jasc_str(), Style::bold});
                        details::report_validation_error_in_anim(
                            services,
                            tile,
                            anim_name,
                            frame.frame_name(),
                            internal_tile_index,
                            row,
                            col,
                            "global-color-count-violation",
                            error_message,
                            extrinsic_transparency);
                    }
                }
            }
        }
    };

    for (const auto &[anim_name, anim] : anims) {
        if (anim.has_key_frame()) {
            const auto &key_frame = anim.key_frame();
            validate_anim_frame(anim_name, key_frame);
        }
        for (const AnimFrame<Rgba32> &frame : anim.frames() | std::views::values) {
            validate_anim_frame(anim_name, frame);
        }
    }

    /*
     * Finally, check Porytiles override pals and hints. We check these last, since we've now seen all colors in the
     * actual input assets. This allows us to warn the user if they've specified manual colors that never actually
     * appear in the input assets.
     */
    std::set<Rgba32> unused_manual_colors{};
    for (std::size_t pal_index = 0; pal_index < num_pals_cfg; ++pal_index) {
        const std::string filename = pal_filename(pal_index);

        if (porytiles_pals.at(pal_index).has_value()) {
            const auto &pal = porytiles_pals.at(pal_index).value();
            std::vector<std::size_t> unused_slots{};

            for (std::size_t slot_index = 0; slot_index < pal.size(); ++slot_index) {
                if (pal.is_wildcard(slot_index)) {
                    continue;
                }

                const auto &pixel = pal.at(slot_index);
                /*
                 * Warn user for nontransparent pal colors that satisfy either:
                 *
                 * 1. The pal color is not present in color_counts, i.e. it wasn't seen in any input assets.
                 * 2. It's present in our unused_manual_colors set, which we update as we iterate over the pals.
                 *
                 * Condition 2) is vital because it allows us to detect and report all instances of an unused color,
                 * even if it was duplicated multiple times in the manual palettes.
                 */
                if ((!color_counts.contains(pixel) || unused_manual_colors.contains(pixel)) &&
                    !pixel.is_transparent(extrinsic_transparency)) {
                    services.diag.warning(
                        "unused-manual-color",
                        services.diag.formatter().format(
                            "color '{}' in slot '{}' of Porytiles palette '{}' is unused",
                            FormatParam{pixel, Style::bold},
                            FormatParam{slot_index, Style::bold},
                            FormatParam{filename, Style::bold}));
                    unused_slots.push_back(slot_index);
                    unused_manual_colors.insert(pixel);
                }

                // Increment count for color.
                if (!pixel.is_transparent(extrinsic_transparency)) {
                    color_counts[pixel]++;
                }

                /*
                 * Print the first violation only, but we'll keep counting across the rest of the pals so we can
                 * give a final tally.
                 */
                if (color_counts.size() > count_max && !hit_first_violation) {
                    hit_first_violation = true;
                    std::string error_message = services.diag.formatter().format(
                        "in Porytiles palette '{}': found {}th globally unique color: {}",
                        FormatParam{filename, Style::bold},
                        FormatParam{pal::max_size},
                        FormatParam{pixel.to_jasc_str(), Style::bold});
                    services.diag.error("global-color-count-violation", error_message);
                    std::vector<std::size_t> violating_slot{};
                    violating_slot.push_back(slot_index);
                    services.diag.error_note(
                        "global-color-count-violation",
                        build_porytiles_pal_highlight_lines(
                            services.diag.formatter(),
                            services.pal_printer,
                            "violating slot",
                            pal,
                            filename,
                            violating_slot));
                }
            } // END: loop over all slots in override pal

            // Print note highlighting all unused slots in this pal
            if (!unused_slots.empty()) {
                services.diag.warning_note(
                    "unused-manual-color",
                    build_porytiles_pal_highlight_lines(
                        services.diag.formatter(),
                        services.pal_printer,
                        "slots with unused colors",
                        pal,
                        filename,
                        unused_slots));
            }
            unused_slots.clear();
        }
    } // END: loop over all override pals

    for (const auto &hint : hints) {
        std::vector<std::size_t> unused_slots{};

        for (std::size_t slot_index = 0; slot_index < hint.pal().size(); ++slot_index) {
            const auto &pixel = hint.pal().at(slot_index);

            /*
             * Warn user for nontransparent pal colors that satisfy either:
             *
             * 1. The pal color is not present in color_counts, i.e. it wasn't seen in any input assets.
             * 2. It's present in our unused_manual_colors set, which we update as we iterate over the pals.
             *
             * Condition 2) is vital because it allows us to detect and report all instances of an unused color,
             * even if it was duplicated multiple times in the manual palettes.
             */
            if ((!color_counts.contains(pixel) || unused_manual_colors.contains(pixel)) &&
                !pixel.is_transparent(extrinsic_transparency)) {
                services.diag.warning(
                    "unused-manual-color",
                    services.diag.formatter().format(
                        "color '{}' in slot '{}' of palette hint '{}' is unused",
                        FormatParam{pixel, Style::bold},
                        FormatParam{slot_index, Style::bold},
                        FormatParam{hint.name(), Style::bold}));
                unused_slots.push_back(slot_index);
                unused_manual_colors.insert(pixel);
            }

            // Increment count for color.
            if (!pixel.is_transparent(extrinsic_transparency)) {
                color_counts[pixel]++;
            }

            /*
             * Print the first violation only, but we'll keep counting across the rest of the hints so we can
             * give a final tally.
             */
            if (color_counts.size() > count_max && !hit_first_violation) {
                hit_first_violation = true;
                std::string error_message = services.diag.formatter().format(
                    "in palette hint '{}': found {}th globally unique color: {}",
                    FormatParam{hint.name(), Style::bold},
                    FormatParam{pal::max_size},
                    FormatParam{pixel.to_jasc_str(), Style::bold});
                services.diag.error("global-color-count-violation", error_message);
                std::vector<std::size_t> violating_slot{};
                violating_slot.push_back(slot_index);
                services.diag.error_note(
                    "global-color-count-violation",
                    build_pal_hint_highlight_lines(
                        services.diag.formatter(),
                        services.pal_printer,
                        "violating slot",
                        hint,
                        hint.name(),
                        violating_slot));
            }
        } // END: loop over all slots in pal hint

        // Print note highlighting all unused slots in this pal
        if (!unused_slots.empty()) {
            services.diag.warning_note(
                "unused-manual-color",
                build_pal_hint_highlight_lines(
                    services.diag.formatter(),
                    services.pal_printer,
                    "slots with unused colors",
                    hint,
                    hint.name(),
                    unused_slots));
        }
        unused_slots.clear();
    } // END: loop over all pal hints

    if (color_counts.size() > count_max) {
        services.diag.error(
            "global-color-count-violation",
            services.diag.formatter().format(
                "global color count violation: found '{}' unique colors, limit is '{}'",
                FormatParam{color_counts.size(), Style::bold},
                FormatParam{count_max, Style::bold}));
        details::report_color_counts("global-color-count-violation", services, color_counts);
        services.diag.error_note(
            "global-color-count-violation",
            build_global_color_limit_lines(services.diag.formatter(), count_max, num_pals_cfg));
        return FormattableError{
            "Found '{}' unique colors globally, limit is '{}'.",
            FormatParam{color_counts.size(), Style::bold},
            FormatParam{count_max, Style::bold}};
    }

    return {};
}

/**
 * @brief Validates that no colors will suffer unacceptable precision loss during GBA color conversion.
 *
 * @details
 * GBA hardware uses 15-bit color (5 bits per channel) rather than 24-bit color (8 bits per channel). When converting
 * from 32-bit RGBA to GBA format, colors may be quantized in ways that cause visually different colors to become
 * identical or similar colors to diverge. This validation checks for such precision loss scenarios and warns users
 * about potential visual artifacts in the final output.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param metatiles The metatiles to check for precision loss.
 * @param anims The animations to check for precision loss.
 * @param porytiles_pals User-specified Porytiles override palettes to check.
 * @param hints User-specified palette hints to check.
 * @param porymap_pals Optional existing Porymap palettes (relevant for pal:patch and pal:locked modes).
 * @return Empty result on success, or FormattableError if critical precision loss is detected.
 *
 * @todo Implementation pending. Should check Porymap palettes in pal:patch and pal:locked modes.
 */
[[nodiscard]] inline ChainableResult<void> validate_precision_loss(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const std::vector<Metatile<Rgba32>> &metatiles,
    const std::map<std::string, Animation<Rgba32>> &anims,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &porytiles_pals,
    const std::vector<PaletteHint> &hints,
    const std::optional<std::array<Palette<Rgba32, pal::max_size>, pal::num_pals>> &porymap_pals)
{
    // TODO : impl
    return {};
}

/**
 * @brief Validates animation frames for correctness according to tileset compilation constraints.
 *
 * @details
 * Performs three validation checks on animation frames:
 *
 * 1. **Transparent key frame check**: Key frame tiles cannot be fully transparent, as this would make them
 *    indistinguishable from the actual transparent tile (tile 0) at runtime, preventing Porytiles from correctly
 *    indexing into animations from the layer sheet. All violations are collected before returning an error. This check
 *    only applies to animations with a key frame (key.png); animations without a key frame are skipped.
 *
 * 2. **Duplicate key frame check (error)**: The same tile content cannot appear as a key frame in multiple positions
 *    across all animations. Each key frame tile must be unique so the game engine can properly identify which animation
 *    a tile belongs to.
 *
 * 3. **Composite frame color count check (error)**: Each animation's composite frame (which aggregates all colors from
 *    all frames at each tile position) cannot exceed 15 unique colors per tile. This is because the palette index is
 *    fixed for the entire animation lifecycle, but the tile data changes dynamically.
 *
 * @param services Common services parameter store.
 * @param tileset_name The name of the tileset being validated (used for config lookup).
 * @param anims The animations to validate.
 * @return Empty result on success, or FormattableError if any validation check fails.
 */
[[nodiscard]] inline ChainableResult<void> validate_anim_frames(
    const TilesetCompileValidatorServices &services,
    const std::string &tileset_name,
    const std::map<std::string, Animation<Rgba32>> &anims)
{
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, extrinsic_transparency, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, global_frame_linking, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_REF(services.config, per_anim_overrides, tileset_name, void);

    bool hit_error = false;

    /*
     * Validation 0: Warn when animations with automatic frame linking are missing a key frame.
     *
     * In automatic mode, key.png is used to determine which tiles in tiles.png are animation tiles. Without it,
     * the first regular frame is used as a representative and animation tiles will not be linked to any metatiles.
     * This is allowed so users can incrementally build animation assets before committing to linking.
     */
    for (const auto &[anim_name, anim] : anims) {
        bool has_per_anim_override = per_anim_overrides.value().contains(anim_name);
        const ConfigValue<FrameLinking> effective_linking =
            (has_per_anim_override && per_anim_overrides.value().at(anim_name).linking.has_value())
                ? per_anim_overrides.derive(per_anim_overrides.value().at(anim_name).linking)
                : global_frame_linking;

        if (effective_linking == FrameLinking::automatic && !anim.has_key_frame()) {
            std::vector<std::string> warning_lines;
            warning_lines.emplace_back(services.diag.formatter().format(
                "Animation '{}' has frame_linking '{}' but no key frame (key.png) was found.",
                FormatParam{anim_name, Style::bold},
                FormatParam{"automatic", Style::bold}));
            warning_lines.emplace_back("Animation tiles will not be linked to any metatiles.");
            services.diag.warning("missing-key-frame", warning_lines);

            if (has_per_anim_override) {
                std::vector<std::string> note_lines;
                note_lines.push_back(services.diag.formatter().format(
                    "Per-animation override for '{}' sets frame_linking to '{}'.",
                    FormatParam{anim_name, Style::bold},
                    FormatParam{"automatic", Style::bold}));
                note_lines.emplace_back("");
                note_lines.append_range(format_config_note(services.diag.formatter(), per_anim_overrides));
                services.diag.warning_note("missing-key-frame", note_lines);
            }
            else {
                services.diag.warning_note(
                    "missing-key-frame", format_config_note(services.diag.formatter(), global_frame_linking));
            }
        }
    }

    /*
     * Validation 1: Ensure no animation key frame tile is fully transparent.
     *
     * Key frames are used to index into animations from the layer sheet. If a key frame tile were transparent,
     * Porytiles couldn't distinguish it from the default transparent tile (tile 0). This restriction only applies to
     * actual key frames (key.png), not to representative frames used as fallbacks when no key frame is present.
     *
     * We collect all violations before failing so the user can see all problematic tiles at once.
     */
    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_key_frame()) {
            continue;
        }

        const auto &key_frame = anim.key_frame();
        const auto &tiles = key_frame.tiles();

        for (std::size_t tile_idx = 0; tile_idx < tiles.size(); ++tile_idx) {
            const auto &tile = tiles.at(tile_idx);

            if (tile.is_transparent(extrinsic_transparency.value())) {
                hit_error = true;
                std::string error_message = "key frame tile is fully transparent";
                details::report_validation_error_in_anim_tile(
                    services,
                    tile,
                    anim_name,
                    key_frame.frame_name(),
                    tile_idx,
                    "transparent-key-frame",
                    error_message,
                    extrinsic_transparency);

                services.diag.error_note(
                    "transparent-key-frame",
                    std::vector<std::string>{
                        "Key frame tiles cannot be fully transparent because they would be",
                        "indistinguishable from the actual transparent tile on the layer PNGs."});
            }
        }
    }

    if (hit_error) {
        return FormattableError{"Found animation(s) with transparent key frame tile(s)."};
    }

    /*
     * Validation 2: Ensure there are no duplicate key frame tiles across all animations.
     *
     * Each key frame tile must be unique so that Porytiles can properly identify which animation a tile belongs to. We
     * collect all key frame tiles into a map and report any that appear in multiple locations.
     */
    std::map<PixelTile<Rgba32>, std::vector<std::pair<std::string, std::size_t>>> key_frame_occurrences;

    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_key_frame()) {
            continue;
        }

        const auto &key_frame = anim.key_frame();
        const auto &tiles = key_frame.tiles();

        for (std::size_t tile_idx = 0; tile_idx < tiles.size(); ++tile_idx) {
            const auto &tile = tiles.at(tile_idx);
            key_frame_occurrences[tile].emplace_back(anim_name, tile_idx);
        }
    }

    for (const auto &[tile, occurrences] : key_frame_occurrences) {
        if (occurrences.size() > 1) {
            hit_error = true;

            // Report error for first occurrence
            const auto &[first_anim, first_idx] = occurrences.at(0);
            std::string error_message = "duplicate key frame tile";
            details::report_validation_error_in_anim_tile(
                services,
                tile,
                first_anim,
                "key",
                first_idx,
                "duplicate-key-frame",
                error_message,
                extrinsic_transparency);

            // Build note showing all locations where this tile appears
            std::vector<std::string> note_lines;
            note_lines.emplace_back("This tile appears as a key frame in multiple locations:");
            for (const auto &[anim_name, tile_idx] : occurrences) {
                note_lines.push_back(services.diag.formatter().format(
                    "   - anim '{}' subtile {}", FormatParam{anim_name, Style::bold}, FormatParam{tile_idx}));
            }
            note_lines.emplace_back("");
            note_lines.emplace_back("Each key frame tile must be unique so that Porytiles can");
            note_lines.emplace_back("identify which animation a tile belongs to at runtime.");
            services.diag.error_note("duplicate-key-frame", note_lines);
        }
    }

    /*
     * Validation 3: Verify composite frame color counts.
     *
     * Each animation's composite frame aggregates all colors from all frames at each tile position. Since the palette
     * index is fixed for the entire animation lifecycle, each composite tile cannot exceed 15 unique colors.
     *
     * We iterate pixel by pixel across all frames for each tile position, tracking actual color counts. When a
     * violation is detected, we show the specific subtile and highlight the pixel that caused the overflow.
     */
    for (const auto &[anim_name, anim] : anims) {
        if (!anim.has_key_frame()) {
            continue;
        }

        const auto &key_frame = anim.key_frame();
        const std::size_t tile_count = key_frame.tile_count();

        for (std::size_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
            // Track color counts across all frames for this tile position
            std::map<Rgba32, unsigned int> color_counts;

            // Track violation details (frame name, tile, row, col) for later reporting
            bool found_violation = false;
            std::string violation_frame_name;
            const PixelTile<Rgba32> *violation_tile = nullptr;
            std::size_t violation_row = 0;
            std::size_t violation_col = 0;
            Rgba32 violation_pixel;

            // Helper lambda to process a tile's pixels
            auto process_tile_pixels = [&](const PixelTile<Rgba32> &tile, const std::string &frame_name) {
                for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                    for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                        const auto &pixel = tile.at(row, col);
                        if (!pixel.is_transparent(extrinsic_transparency)) {
                            color_counts[pixel]++;
                        }

                        // Record the first violation but continue counting for accurate totals
                        if (color_counts.size() > pal::max_size - 1 && !found_violation) {
                            found_violation = true;
                            violation_frame_name = frame_name;
                            violation_tile = &tile;
                            violation_row = row;
                            violation_col = col;
                            violation_pixel = pixel;
                        }
                    }
                }
            };

            // Process key frame pixels
            process_tile_pixels(key_frame.tile_at(tile_idx), key_frame.frame_name());

            // Process regular frame pixels
            for (const auto &[frame_name, frame] : anim.frames()) {
                process_tile_pixels(frame.tile_at(tile_idx), frame_name);
            }

            // Report violation after processing all frames (so we have complete color counts)
            if (found_violation) {
                hit_error = true;

                std::string error_message = services.diag.formatter().format(
                    "found {}th unique composite frame tile color: {}",
                    FormatParam{pal::max_size},
                    FormatParam{violation_pixel.to_jasc_str(), Style::bold});
                details::report_validation_error_in_anim(
                    services,
                    *violation_tile,
                    anim_name,
                    violation_frame_name,
                    tile_idx,
                    violation_row,
                    violation_col,
                    "composite-color-count-violation",
                    error_message,
                    extrinsic_transparency);

                std::vector<std::string> note_lines;
                note_lines.emplace_back("The composite frame aggregates all colors across all animation frames");
                note_lines.emplace_back("for each tile position. Since the palette index is fixed for the");
                note_lines.emplace_back("entire animation lifecycle, each composite tile cannot exceed 15 colors.");
                services.diag.error_note("composite-color-count-violation", note_lines);

                // Print all frame tiles at this position to show what's contributing colors
                std::vector<std::string> frame_tiles_note;
                frame_tiles_note.emplace_back("Tiles at this position across all frames:");

                // Print key frame tile
                frame_tiles_note.emplace_back("");
                frame_tiles_note.push_back(services.diag.formatter().format(
                    "Frame '{}' subtile {}:", FormatParam{key_frame.frame_name(), Style::bold}, FormatParam{tile_idx}));
                auto key_tile_visual =
                    services.tile_printer.print_tile(key_frame.tile_at(tile_idx), extrinsic_transparency);
                frame_tiles_note.append_range(key_tile_visual);

                // Print regular frame tiles
                for (const auto &[frame_name, frame] : anim.frames()) {
                    frame_tiles_note.emplace_back("");
                    frame_tiles_note.push_back(services.diag.formatter().format(
                        "Frame '{}' subtile {}:", FormatParam{frame_name, Style::bold}, FormatParam{tile_idx}));
                    auto frame_tile_visual =
                        services.tile_printer.print_tile(frame.tile_at(tile_idx), extrinsic_transparency);
                    frame_tiles_note.append_range(frame_tile_visual);
                }

                services.diag.error_note("composite-color-count-violation", frame_tiles_note);

                details::report_color_counts("composite-color-count-violation", services, color_counts);
            }
        }
    }

    if (hit_error) {
        return FormattableError{"Animation frame validation failed."};
    }

    return {};
}

} // namespace porytiles2
