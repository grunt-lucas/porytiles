#include "diagnostics/diagnostics.hpp"

#include <iostream>
#include <unistd.h>

namespace porytiles {

// FIXME : map with string_view key is not good style: https://olafurw.com/2022-12-03-a-view-of-a-map/
static const std::unordered_map<std::string_view, diag_templ> DIAG_TEMPLS{
    {W_COLOR_PRECISION_LOSS, W_COLOR_PRECISION_LOSS_TEMPL},
    {W_KEY_FRAME_NO_MATCHING_TILE, W_KEY_FRAME_NO_MATCHING_TILE_TEMPL},
    {W_KEY_FRAME_MISSING_COLORS, W_KEY_FRAME_MISSING_COLORS_TEMPL},
    {W_USED_TRUE_COLOR_MODE, W_USED_TRUE_COLOR_MODE_TEMPL},

    {E_GENERIC, E_GENERIC_TEMPL}};

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
        panic("level_to_str: unknown diag_level");
    }
}

std::string build_tile_pixel_highlight(bool is_a_tty, diag_level in_flight_level, const RGBATile &tile, std::size_t row,
                                       std::size_t col) {
    const fmt::terminal_color level_color = color_for_level(in_flight_level);

    std::stringstream ss{};
    if (tile.type == TileType::LAYERED) {
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                if (j == 0) {
                    ss << "  |";
                }
                if (is_a_tty && tile.subtile == Subtile::NORTHWEST && i <= 7 && j <= 7) {
                    if (row == i && col == j) {
                        ss << fmt::format("{}", styled(" X ", fg(level_color) | fmt::emphasis::bold));
                    } else {
                        ss << fmt::format("{}", styled(" * ", fmt::emphasis::bold));
                    }
                } else if (is_a_tty && tile.subtile == Subtile::NORTHEAST && i <= 7 && j > 7) {
                    if (row == i && col == j - 7) {
                        ss << fmt::format("{}", styled(" X ", fg(level_color) | fmt::emphasis::bold));
                    } else {
                        ss << fmt::format("{}", styled(" * ", fmt::emphasis::bold));
                    }
                } else if (is_a_tty && tile.subtile == Subtile::SOUTHWEST && i > 7 && j <= 7) {
                    if (row == i - 7 && col == j) {
                        ss << fmt::format("{}", styled(" X ", fg(level_color) | fmt::emphasis::bold));
                    } else {
                        ss << fmt::format("{}", styled(" * ", fmt::emphasis::bold));
                    }
                } else if (is_a_tty && tile.subtile == Subtile::SOUTHEAST && i > 7 && j > 7) {
                    if (row == i - 7 && col == j - 7) {
                        ss << fmt::format("{}", styled(" X ", fg(level_color) | fmt::emphasis::bold));
                    } else {
                        ss << fmt::format("{}", styled(" * ", fmt::emphasis::bold));
                    }
                } else {
                    ss << " - ";
                }
                if (j == 7) {
                    ss << " ";
                }
                if (j == 15) {
                    ss << std::endl;
                }
            }
            if (i == 7) {
                ss << "  |" << std::endl;
            }
        }
    }
    return ss.str();
}

diag_templ diag_templ_for(const std::string_view diag) {
    if (!DIAG_TEMPLS.contains(diag)) {
        panic(fmt::format("diag_template_for: unknown diagnostic: {}", diag));
    }
    return DIAG_TEMPLS.at(diag);
}

void ignore_consumer::consume(const in_flight_diag &diag) {
    consumed_count_++;
}

bool ignore_consumer::is_a_tty() const {
    return false;
}

in_flight_diag ignore_consumer::consumed(std::size_t i) const {
    panic("ignore_consumer::consumed: not implemented");
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

in_flight_diag stderr_consumer::consumed(std::size_t i) const {
    panic("stderr_consumer::consumed: not implemented");
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

in_flight_diag vector_consumer::consumed(std::size_t i) const {
    try {
        return diags_.at(i);
    } catch (const std::out_of_range &) {
        panic(fmt::format("vector_consumer::at: index {} out of range for size {}", i, diags_.size()));
    }
}

std::uint64_t vector_consumer::consumed_count() const {
    return diags_.size();
}

} // namespace porytiles
