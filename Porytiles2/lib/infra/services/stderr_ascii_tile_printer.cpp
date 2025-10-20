#include "porytiles2/infra/services/stderr_ascii_tile_printer.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

void push_to_stream(std::stringstream &ss, const std::string_view s, const std::size_t n)
{
    for (std::size_t i = 0; i < n; i++) {
        ss << s;
    }
}

void reset_stream(std::stringstream &ss)
{
    ss.clear();
    ss.str(std::string{});
}

} // namespace

namespace porytiles2 {

std::vector<std::string> StderrAsciiTilePrinter::print_metatile_highlight(
    metatile::Subtile subtile, std::size_t row, std::size_t col, Style color) const
{
    std::vector<std::string> highlight{};
    std::stringstream ss{};

    for (std::size_t i = 0; i < metatile::side_length_pix; i++) {
        for (std::size_t j = 0; j < metatile::side_length_pix; j++) {
            // General case. Decide if we are drawing the highlighted tile
            // and pixel. If not, draw a "-".

            auto styled_x = format_->format(" {} ", FormatParam{"X", Style::bold | color});
            auto styled_star = format_->format(" {} ", FormatParam{"*", Style::bold});
            if (subtile == metatile::Subtile::northwest && i < 8 && j < 8) {
                if (row == i && col == j) {
                    ss << format_->format("{}", FormatParam{styled_x});
                }
                else {
                    ss << format_->format("{}", FormatParam{styled_star});
                }
            }
            else if (subtile == metatile::Subtile::northeast && i < 8 && j >= 8) {
                if (row == i && col == j - 8) {
                    ss << format_->format("{}", FormatParam{styled_x});
                }
                else {
                    ss << format_->format("{}", FormatParam{styled_star});
                }
            }
            else if (subtile == metatile::Subtile::southwest && i >= 8 && j < 8) {
                if (row == i - 8 && col == j) {
                    ss << format_->format("{}", FormatParam{styled_x});
                }
                else {
                    ss << format_->format("{}", FormatParam{styled_star});
                }
            }
            else if (subtile == metatile::Subtile::southeast && i >= 8 && j >= 8) {
                if (row == i - 8 && col == j - 8) {
                    ss << format_->format("{}", FormatParam{styled_x});
                }
                else {
                    ss << format_->format("{}", FormatParam{styled_star});
                }
            }
            else {
                ss << " - ";
            }

            // If we're at the midpoint cell, add an extra space.
            if (j == 7) {
                ss << " ";
            }

            // Reset once this row is exhausted
            if (j == 15) {
                highlight.push_back(ss.str());
                reset_stream(ss);
            }
        }

        // Insert a spacer line between top and bottom tiles
        if (i == 7) {
            highlight.push_back(ss.str());
            reset_stream(ss);
        }
    }

    return highlight;
}

} // namespace porytiles2
