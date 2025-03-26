#include "diagnostics/diagnostics.hpp"

#include <ranges>
#include <sstream>
#include <unistd.h>

namespace {

using namespace porytiles;

void push_to_ss_n_times(std::stringstream &ss, std::string_view s, std::size_t n) {
    for (std::size_t i = 0; i < n; i++) {
        ss << s;
    }
}

constexpr std::size_t DIAG_MARGIN_SIZE = 7;

std::vector<std::string> build_tile_pixel_highlight(bool is_a_tty, diag_level in_flight_level, const RGBATile &tile,
                                                    std::size_t row, std::size_t col) {
    std::vector<std::string> highlight{};
    std::stringstream ss{};
    const fmt::terminal_color level_color = color_for_level(in_flight_level);

    if (tile.type == TileType::LAYERED) {
        for (std::size_t i = 0; i < 16; i++) {
            for (std::size_t j = 0; j < 16; j++) {
                // First cell of each row is margin followed by a bar: "       |"
                if (j == 0) {
                    push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
                    ss << "|";
                }

                /*
                 * General case. Decide if we are drawing the highlighted tile
                 * and pixel. If not, draw a "-".
                 */
                auto styled_x = styled(" X ", is_a_tty ? (fg(level_color) | fmt::emphasis::bold) : fmt::text_style{});
                auto styled_star = styled(" * ", is_a_tty ? fmt::emphasis::bold : fmt::text_style{});
                if (tile.subtile == Subtile::NORTHWEST && i < 8 && j < 8) {
                    if (row == i && col == j) {
                        ss << format(fmt::runtime("{}"), styled_x);
                    } else {
                        ss << format(fmt::runtime("{}"), styled_star);
                    }
                } else if (tile.subtile == Subtile::NORTHEAST && i < 8 && j >= 8) {
                    if (row == i && col == j - 8) {
                        ss << format(fmt::runtime("{}"), styled_x);
                    } else {
                        ss << format(fmt::runtime("{}"), styled_star);
                    }
                } else if (tile.subtile == Subtile::SOUTHWEST && i >= 8 && j < 8) {
                    if (row == i - 8 && col == j) {
                        ss << format(fmt::runtime("{}"), styled_x);
                    } else {
                        ss << format(fmt::runtime("{}"), styled_star);
                    }
                } else if (tile.subtile == Subtile::SOUTHEAST && i >= 8 && j >= 8) {
                    if (row == i - 8 && col == j - 8) {
                        ss << format(fmt::runtime("{}"), styled_x);
                    } else {
                        ss << format(fmt::runtime("{}"), styled_star);
                    }
                } else {
                    ss << " - ";
                }

                // If we're at the midpoint cell, add an extra space.
                if (j == 7) {
                    ss << " ";
                }

                // Reset once this row is exhausted
                if (j == 15) {
                    highlight.push_back(ss.str());
                    ss.clear();
                    ss.str(std::string{});
                }
            }

            // Insert a spacer line between top and bottom tiles
            if (i == 7) {
                push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
                ss << "|";
                highlight.push_back(ss.str());
                ss.clear();
                ss.str(std::string{});
            }
        }
    }
    return highlight;
}

} // namespace

namespace porytiles {

std::string level_to_str(diag_level level) {
    switch (level) {
    case diag_level::ignored:
        return "ignored";
    case diag_level::note:
        return "note";
    case diag_level::remark:
        return "remark";
    case diag_level::warning:
        return "warning";
    case diag_level::error:
        return "error";
    case diag_level::fatal:
        return "fatal error";
    default:
        panic("level_to_str: unknown diag_level");
    }
}

fmt::terminal_color color_for_level(diag_level level) {
    switch (level) {
    case diag_level::ignored:
        return fmt::terminal_color::white;
    case diag_level::note:
        return fmt::terminal_color::cyan;
    case diag_level::remark:
        return fmt::terminal_color::green;
    case diag_level::warning:
        return fmt::terminal_color::magenta;
    case diag_level::error:
    case diag_level::fatal:
        return fmt::terminal_color::red;
    default:
        panic("color_for_level: unknown diag_level");
    }
}

void ignore_consumer::consume(const in_flight_diag &diag) {
    consumed_count_++;
}

bool ignore_consumer::is_a_tty() const {
    return false;
}

in_flight_diag ignore_consumer::consumed_at(std::size_t i) const {
    panic("ignore_consumer::consumed_at: not implemented");
}

std::uint64_t ignore_consumer::consumed_count() const {
    return consumed_count_;
}

void stderr_consumer::consume(const in_flight_diag &diag) {
    consumed_count_++;
    std::fputs(diag.msg().c_str(), stderr);
}

bool stderr_consumer::is_a_tty() const {
    return isatty(fileno(stderr));
}

in_flight_diag stderr_consumer::consumed_at(std::size_t i) const {
    panic("stderr_consumer::consumed_at: not implemented");
}

std::uint64_t stderr_consumer::consumed_count() const {
    return consumed_count_;
}

void vector_consumer::consume(const in_flight_diag &diag) {
    diags_.emplace_back(diag);
}

bool vector_consumer::is_a_tty() const {
    return false;
}

in_flight_diag vector_consumer::consumed_at(std::size_t i) const {
    try {
        return diags_.at(i);
    } catch (const std::out_of_range &) {
        panic(fmt::format("vector_consumer::at: index {} out of range for size {}", i, diags_.size()));
    }
}

std::uint64_t vector_consumer::consumed_count() const {
    return diags_.size();
}

std::vector<std::string> w_color_precision_loss_dynamic_msg_builder(bool is_a_tty, diag_level in_flight_level,
                                                                    const std::vector<std::any> &args) {
    if (args.size() != 5) {
        const auto loc = std::source_location::current();
        panic(fmt::format("{}: found {} args but expected 5", loc.function_name(), args.size()));
    }

    const RGBATile *tile;
    const char *color;
    const char *mode;
    std::size_t row;
    std::size_t col;
    try {
        tile = &std::any_cast<const RGBATile &>(args[0]);
        color = std::any_cast<const char *>(args[1]);
        mode = std::any_cast<const char *>(args[2]);
        row = std::any_cast<std::size_t>(args[3]);
        col = std::any_cast<std::size_t>(args[4]);
    } catch (const std::bad_any_cast &) {
        const auto loc = std::source_location::current();
        panic(fmt::format("{}: bad_any_cast", loc.function_name()));
    }

    constexpr auto msg_templ = "{} {}: collapsed to duplicate BGR: '{}' at col '{}', row '{}'";
    std::vector<std::string> msg{};
    if (is_a_tty) {
        msg.push_back(fmt::format(
            msg_templ, styled(mode, fmt::emphasis::bold), styled(tile->prettify().c_str(), fmt::emphasis::bold),
            styled(color, fmt::emphasis::bold), styled(col, fmt::emphasis::bold), styled(row, fmt::emphasis::bold)));
    } else {
        msg.push_back(fmt::format(msg_templ, mode, tile->prettify().c_str(), color, col, row));
    }
    auto highlight = build_tile_pixel_highlight(is_a_tty, in_flight_level, *tile, row, col);
    msg.insert(std::end(msg), std::begin(highlight), std::end(highlight));

    return msg;
}

std::vector<std::string> w_color_precision_loss_note_dynamic_msg_builder(bool is_a_tty, diag_level in_flight_level,
                                                                         const std::vector<std::any> &args) {
    if (args.size() != 4) {
        const auto loc = std::source_location::current();
        panic(fmt::format("{}: found {} args but expected 4", loc.function_name(), args.size()));
    }

    const RGBATile *tile;
    const char *color;
    std::size_t row;
    std::size_t col;
    try {
        tile = &std::any_cast<const RGBATile &>(args[0]);
        color = std::any_cast<const char *>(args[1]);
        row = std::any_cast<std::size_t>(args[2]);
        col = std::any_cast<std::size_t>(args[3]);
    } catch (const std::bad_any_cast &) {
        const auto loc = std::source_location::current();
        panic(fmt::format("{}: bad_any_cast", loc.function_name()));
    }

    /*
     * FIXME : this template is incomplete, we want to show the mode since it's
     * possible to have precision loss across a primary-secondary boundary
     */
    constexpr auto msg_templ = "{}: previously saw: '{}' at col '{}', row '{}'";
    std::vector<std::string> msg{};
    if (is_a_tty) {
        msg.push_back(fmt::format(msg_templ, styled(tile->prettify().c_str(), fmt::emphasis::bold),
                                  styled(color, fmt::emphasis::bold),

                                  styled(col, fmt::emphasis::bold), styled(row, fmt::emphasis::bold)));
    } else {
        msg.push_back(fmt::format(msg_templ, tile->prettify().c_str(), color, col, row));
    }
    auto highlight = build_tile_pixel_highlight(is_a_tty, in_flight_level, *tile, row, col);
    msg.insert(std::end(msg), std::begin(highlight), std::end(highlight));

    return msg;
}

// @formatter:off
// clang-format off
static const diag_templ W_COLOR_PRECISION_LOSS_TEMPL{
    W_COLOR_PRECISION_LOSS,
    false,
    diag_level::warning,
    w_color_precision_loss_dynamic_msg_builder,
    {
        diag_templ{
            "color-precision-loss-previously-seen-note",
            false,
            diag_level::note,
            w_color_precision_loss_note_dynamic_msg_builder,
            {}
        }
    }
};

static const diag_templ W_KEY_FRAME_NO_MATCHING_TILE_TEMPL{
    W_KEY_FRAME_NO_MATCHING_TILE,
    false,
    diag_level::warning,
    "animation '{}' key frame tile '{}' was not present in any metatile entries",
    {}
};

static const diag_templ W_KEY_FRAME_MISSING_COLORS_TEMPL{
    W_KEY_FRAME_MISSING_COLORS,
    false,
    diag_level::warning,
    "animation '{}' key frame tile '{}' missing essential colors",
    {}
};

static const diag_templ W_USED_TRUE_COLOR_MODE_TEMPL{W_USED_TRUE_COLOR_MODE, true, diag_level::warning, "'true-color' mode requires Porymap minimum version 5.2.0", {}};

static const diag_templ W_ATTRIBUTE_FORMAT_MISMATCH_TEMPL{W_ATTRIBUTE_FORMAT_MISMATCH, false, diag_level::warning, "{}: too {} attribute columns for base game '{}'", {}};

static const diag_templ W_MISSING_ATTRIBUTES_CSV_TEMPL{W_MISSING_ATTRIBUTES_CSV, false, diag_level::warning, "{}: attributes file did not exist", {}};

static const diag_templ W_UNUSED_ATTRIBUTE_TEMPL{W_MISSING_ATTRIBUTES_CSV, false, diag_level::warning, "found attribute for nonexistent metatile ID {}", {}};

static const diag_templ W_TRANSPARENCY_COLLAPSE_TEMPL{W_TRANSPARENCY_COLLAPSE, false, diag_level::warning, "color '{}' at {} '{}' subtile pixel col {}, row {} collapsed to transparent under BGR conversion", {}};

static const diag_templ W_UNUSED_MANUAL_PAL_COLOR_TEMPL{W_UNUSED_MANUAL_PAL_COLOR, false, diag_level::warning, "{}: '{}' was not used in layers or anims", {}};

static const diag_templ E_GENERIC_TEMPL{E_GENERIC, true, diag_level::error, "{}", {}};

static const diag_templ E_FATAL_GENERIC_TEMPL{E_FATAL_GENERIC, true, diag_level::fatal, "{}", {}};

static const std::unordered_map<const char *, diag_templ> DIAG_TEMPLS{
    {W_COLOR_PRECISION_LOSS, W_COLOR_PRECISION_LOSS_TEMPL},
    {W_KEY_FRAME_NO_MATCHING_TILE, W_KEY_FRAME_NO_MATCHING_TILE_TEMPL},
    {W_KEY_FRAME_MISSING_COLORS, W_KEY_FRAME_MISSING_COLORS_TEMPL},
    {W_USED_TRUE_COLOR_MODE, W_USED_TRUE_COLOR_MODE_TEMPL},
    {W_ATTRIBUTE_FORMAT_MISMATCH,W_ATTRIBUTE_FORMAT_MISMATCH_TEMPL},
    {W_MISSING_ATTRIBUTES_CSV,W_MISSING_ATTRIBUTES_CSV_TEMPL},
    {W_UNUSED_ATTRIBUTE,W_UNUSED_ATTRIBUTE_TEMPL},
    {W_TRANSPARENCY_COLLAPSE,W_TRANSPARENCY_COLLAPSE_TEMPL},
    {W_UNUSED_MANUAL_PAL_COLOR, W_UNUSED_MANUAL_PAL_COLOR_TEMPL},

    {E_GENERIC, E_GENERIC_TEMPL},
    {E_FATAL_GENERIC,E_FATAL_GENERIC_TEMPL}
};
// @formatter:on
// clang-format on

diag_templ diag_templ_for(const std::string_view diag) {
    if (!DIAG_TEMPLS.contains(diag.data())) {
        panic(fmt::format("diag_template_for: unknown diagnostic: {}", diag));
    }
    return DIAG_TEMPLS.at(diag.data());
}

auto all_diag_templs() {
    return std::views::keys(DIAG_TEMPLS);
}

} // namespace porytiles
