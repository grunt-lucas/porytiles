#include "porytiles/utilities/text/ansi_styled_text_formatter.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <string>

namespace {

using namespace porytiles;

/// @brief Converts RGB color to the closest ANSI 256 color code.
///
/// @details
/// Maps an RGB color to the closest color in the ANSI 256 color palette. The function handles two cases:
/// - Grayscale colors (when r == g == b) map to colors 232-255
/// - Other colors map to the 6x6x6 color cube (colors 16-231)
///
/// @param r Red component (0-255)
/// @param g Green component (0-255)
/// @param b Blue component (0-255)
/// @return ANSI 256 color code (16-255)
int rgb_to_ansi256(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    // Handle grayscale (colors 232-255)
    if (r == g && g == b) {
        if (r < 8) {
            return 16;
        }
        if (r > 248) {
            return 231;
        }
        return static_cast<int>(std::lround((r - 8) / 247.0 * 24)) + 232;
    }

    // Convert to 6x6x6 color cube (colors 16-231)
    // Map each RGB component from 0-255 to 0-5
    auto to_6 = [](const std::uint8_t val) -> int {
        if (val < 48) {
            return 0;
        }
        if (val < 115) {
            return 1;
        }
        return (val - 35) / 40;
    };

    const int r_idx = to_6(r);
    const int g_idx = to_6(g);
    const int b_idx = to_6(b);

    return 16 + 36 * r_idx + 6 * g_idx + b_idx;
}

/// @brief Finds the closest plain ANSI color to the given RGB value.
///
/// @details
/// Uses weighted perceptual distance (2*ΔR² + 4*ΔG² + 3*ΔB²) to find the closest match
/// among the 8 standard ANSI colors. The weights reflect human perception sensitivity
/// to different color channels.
///
/// @param r Red component (0-255)
/// @param g Green component (0-255)
/// @param b Blue component (0-255)
/// @return PredefinedColor enum value for the closest plain color
PredefinedColor find_closest_plain_color(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    // Standard ANSI color RGB values
    const std::array<std::pair<PredefinedColor, std::array<int, 3>>, 8> plain_colors = {{
        {PredefinedColor::black, {0, 0, 0}},
        {PredefinedColor::red, {255, 0, 0}},
        {PredefinedColor::green, {0, 255, 0}},
        {PredefinedColor::yellow, {255, 255, 0}},
        {PredefinedColor::blue, {0, 0, 255}},
        {PredefinedColor::magenta, {255, 0, 255}},
        {PredefinedColor::cyan, {0, 255, 255}},
        {PredefinedColor::white, {255, 255, 255}},
    }};

    PredefinedColor closest = PredefinedColor::white;
    double min_distance = std::numeric_limits<double>::max();

    for (const auto &[color, rgb] : plain_colors) {
        const int dr = r - rgb[0];
        const int dg = g - rgb[1];
        const int db = b - rgb[2];

        // Weighted perceptual distance: 2*ΔR² + 4*ΔG² + 3*ΔB²
        const double distance = 2.0 * dr * dr + 4.0 * dg * dg + 3.0 * db * db;

        if (distance < min_distance) {
            min_distance = distance;
            closest = color;
        }
    }

    return closest;
}

// ANSI reset code
const std::string ansi_reset = "\033[0m";

// Mapping from PredefinedColor to ANSI foreground codes
const std::array<std::pair<PredefinedColor, std::string>, 8> fg_color_mappings = {{
    {PredefinedColor::black, "\033[30m"},
    {PredefinedColor::red, "\033[31m"},
    {PredefinedColor::green, "\033[32m"},
    {PredefinedColor::yellow, "\033[33m"},
    {PredefinedColor::blue, "\033[34m"},
    {PredefinedColor::magenta, "\033[35m"},
    {PredefinedColor::cyan, "\033[36m"},
    {PredefinedColor::white, "\033[37m"},
}};

// Mapping from PredefinedColor to ANSI background codes
const std::array<std::pair<PredefinedColor, std::string>, 8> bg_color_mappings = {{
    {PredefinedColor::black, "\033[40m"},
    {PredefinedColor::red, "\033[41m"},
    {PredefinedColor::green, "\033[42m"},
    {PredefinedColor::yellow, "\033[43m"},
    {PredefinedColor::blue, "\033[44m"},
    {PredefinedColor::magenta, "\033[45m"},
    {PredefinedColor::cyan, "\033[46m"},
    {PredefinedColor::white, "\033[47m"},
}};

} // namespace

namespace porytiles {

std::string AnsiStyledTextFormatter::style(const std::string &text, Style styles) const
{
    std::string prefix;

    // Handle foreground color
    if (styles.has_fg_color()) {
        if (styles.is_fg_rgb()) {
            const RgbColor rgb = styles.fg_rgb();

            switch (mode_) {
            case AnsiColorMode::plain: {
                // Find closest plain color and use its standard ANSI code
                const PredefinedColor closest = find_closest_plain_color(rgb.r, rgb.g, rgb.b);
                for (const auto &[color, ansi_code] : fg_color_mappings) {
                    if (color == closest) {
                        prefix += ansi_code;
                        break;
                    }
                }
                break;
            }
            case AnsiColorMode::colors_256: {
                // Convert to ANSI-256 and emit \033[38;5;<code>m
                const int color_code = rgb_to_ansi256(rgb.r, rgb.g, rgb.b);
                prefix += "\033[38;5;" + std::to_string(color_code) + "m";
                break;
            }
            case AnsiColorMode::colors_24_bit: {
                // Emit 24-bit RGB ANSI code \033[38;2;<r>;<g>;<b>m
                prefix += "\033[38;2;" + std::to_string(rgb.r) + ";" + std::to_string(rgb.g) + ";" +
                          std::to_string(rgb.b) + "m";
                break;
            }
            }
        }
        else {
            // Handle predefined foreground colors using standard ANSI codes
            const PredefinedColor fg = styles.fg_predefined();
            for (const auto &[color, ansi_code] : fg_color_mappings) {
                if (color == fg) {
                    prefix += ansi_code;
                    break;
                }
            }
        }
    }

    // Handle background color
    if (styles.has_bg_color()) {
        if (styles.is_bg_rgb()) {
            const RgbColor rgb = styles.bg_rgb();

            switch (mode_) {
            case AnsiColorMode::plain: {
                // Find closest plain color and use its standard ANSI code
                const PredefinedColor closest = find_closest_plain_color(rgb.r, rgb.g, rgb.b);
                for (const auto &[color, ansi_code] : bg_color_mappings) {
                    if (color == closest) {
                        prefix += ansi_code;
                        break;
                    }
                }
                break;
            }
            case AnsiColorMode::colors_256: {
                // Convert to ANSI-256 and emit \033[48;5;<code>m
                const int color_code = rgb_to_ansi256(rgb.r, rgb.g, rgb.b);
                prefix += "\033[48;5;" + std::to_string(color_code) + "m";
                break;
            }
            case AnsiColorMode::colors_24_bit: {
                // Emit 24-bit RGB ANSI code \033[48;2;<r>;<g>;<b>m
                prefix += "\033[48;2;" + std::to_string(rgb.r) + ";" + std::to_string(rgb.g) + ";" +
                          std::to_string(rgb.b) + "m";
                break;
            }
            }
        }
        else {
            // Handle predefined background colors using standard ANSI codes
            const PredefinedColor bg = styles.bg_predefined();
            for (const auto &[color, ansi_code] : bg_color_mappings) {
                if (color == bg) {
                    prefix += ansi_code;
                    break;
                }
            }
        }
    }

    // Apply formatting flags (bold, italic) - these work independently of color mode
    if (styles.has_bold()) {
        prefix += "\033[1m";
    }
    if (styles.has_faint()) {
        prefix += "\033[2m";
    }
    if (styles.has_italic()) {
        prefix += "\033[3m";
    }
    if (styles.has_underline()) {
        prefix += "\033[4m";
    }
    if (styles.has_blink()) {
        prefix += "\033[5m";
    }

    // Only add prefix and reset if we actually have styling to apply
    if (prefix.empty()) {
        return text;
    }

    return prefix + text + ansi_reset;
}

} // namespace porytiles