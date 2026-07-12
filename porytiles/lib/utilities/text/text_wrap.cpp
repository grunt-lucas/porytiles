#include "porytiles/utilities/text/text_wrap.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace porytiles;

const std::string ansi_reset = "\033[0m";

/// @brief One visible glyph plus the ANSI escape sequences that immediately precede it.
struct Cell {
    std::string prefix; ///< ANSI escape sequences emitted just before the glyph (may be empty)
    std::string glyph;  ///< A single visible unit: one UTF-8 code point, counted as one column
    bool is_space;      ///< True when the glyph is an ASCII space, marking a break opportunity
};

/// @brief Advances past a CSI escape sequence starting at @p pos, returning its raw text.
std::string consume_escape(const std::string &line, std::size_t &pos)
{
    const std::size_t start = pos;
    pos += 2; // skip the ESC and '['
    // CSI parameters/intermediates run until a final byte in the 0x40-0x7E range.
    while (pos < line.size() &&
           (static_cast<unsigned char>(line[pos]) < 0x40 || static_cast<unsigned char>(line[pos]) > 0x7E)) {
        ++pos;
    }
    if (pos < line.size()) {
        ++pos; // include the final byte
    }
    return line.substr(start, pos - start);
}

/// @brief Advances past one UTF-8 code point starting at @p pos, returning its raw bytes.
std::string consume_codepoint(const std::string &line, std::size_t &pos)
{
    const std::size_t start = pos;
    const auto lead = static_cast<unsigned char>(line[pos]);
    std::size_t len = 1;
    if ((lead & 0x80U) != 0) {
        if ((lead & 0xE0U) == 0xC0U) {
            len = 2;
        }
        else if ((lead & 0xF0U) == 0xE0U) {
            len = 3;
        }
        else if ((lead & 0xF8U) == 0xF0U) {
            len = 4;
        }
    }
    len = std::min(len, line.size() - pos);
    pos += len;
    return line.substr(start, len);
}

/// @brief Splits a line into visible glyph cells, capturing any trailing escapes with no following glyph.
std::vector<Cell> tokenize(const std::string &line, std::string &trailing_out)
{
    std::vector<Cell> cells;
    std::string pending_prefix;
    std::size_t pos = 0;
    while (pos < line.size()) {
        if (static_cast<unsigned char>(line[pos]) == 0x1B && pos + 1 < line.size() && line[pos + 1] == '[') {
            pending_prefix += consume_escape(line, pos);
        }
        else {
            std::string glyph = consume_codepoint(line, pos);
            const bool is_space = glyph == " ";
            cells.push_back(Cell{std::move(pending_prefix), std::move(glyph), is_space});
            pending_prefix.clear();
        }
    }
    trailing_out = pending_prefix;
    return cells;
}

/// @brief Folds an escape-code prefix into the active SGR state, clearing it on a full reset.
void apply_sgr(std::string &active, const std::string &prefix)
{
    std::size_t pos = 0;
    while (pos < prefix.size()) {
        const std::size_t start = pos;
        std::string seq = consume_escape(prefix, pos);
        if (seq == ansi_reset || seq == "\033[m") {
            active.clear();
        }
        else {
            active += seq;
        }
    }
}

} // namespace

namespace porytiles {

std::vector<std::string> wrap_ansi_line(const std::string &line, const std::size_t width)
{
    if (width == 0) {
        return {line};
    }

    std::string trailing;
    const std::vector<Cell> cells = tokenize(line, trailing);
    if (cells.empty()) {
        return {line};
    }

    // active_entering[i] is the SGR state in effect just before cell i's own prefix, i.e. the styling a wrapped
    // continuation line must re-open when it starts at cell i. active_entering[cells.size()] is the state after the
    // final cell, used to decide whether a line needs a trailing reset.
    std::vector<std::string> active_entering(cells.size() + 1);
    for (std::size_t i = 0; i < cells.size(); ++i) {
        active_entering[i + 1] = active_entering[i];
        apply_sgr(active_entering[i + 1], cells[i].prefix);
    }

    // First pass: choose the [start, end) cell ranges for each physical line, breaking at spaces where possible.
    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    std::size_t i = 0;
    while (i < cells.size()) {
        const std::size_t line_start = i;
        std::size_t visible = 0;
        std::size_t last_space = cells.size(); // sentinel: no break opportunity seen yet
        std::size_t j = i;
        while (j < cells.size() && visible < width) {
            if (cells[j].is_space) {
                last_space = j;
            }
            ++visible;
            ++j;
        }
        if (j == cells.size()) {
            ranges.emplace_back(line_start, cells.size());
            break;
        }
        if (cells[j].is_space) {
            // The break lands exactly on a space: end the line here and swallow the run of spaces.
            ranges.emplace_back(line_start, j);
            while (j < cells.size() && cells[j].is_space) {
                ++j;
            }
            i = j;
        }
        else if (last_space != cells.size() && last_space > line_start) {
            // Break at the last space that fit on the line, swallowing that single space.
            ranges.emplace_back(line_start, last_space);
            i = last_space + 1;
        }
        else {
            // No usable space boundary (an over-long word): hard-break at the column limit.
            ranges.emplace_back(line_start, j);
            i = j;
        }
    }

    // Second pass: render each range, re-opening inherited style and closing any style left open.
    std::vector<std::string> result;
    result.reserve(ranges.size());
    for (std::size_t r = 0; r < ranges.size(); ++r) {
        const auto [start, end] = ranges[r];
        std::string rendered = active_entering[start];
        for (std::size_t k = start; k < end; ++k) {
            rendered += cells[k].prefix;
            rendered += cells[k].glyph;
        }
        const bool is_last = r + 1 == ranges.size();
        if (is_last) {
            rendered += trailing;
        }
        std::string active_at_end = active_entering[end];
        if (is_last) {
            apply_sgr(active_at_end, trailing);
        }
        if (!active_at_end.empty()) {
            rendered += ansi_reset;
        }
        result.push_back(std::move(rendered));
    }
    return result;
}

} // namespace porytiles
