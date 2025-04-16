#include "diagnostics/diagnostics.hpp"

#include <any>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <unistd.h>
#include <unordered_set>

#include "./diagnostics//diagnostic_engine.hpp"

namespace {

using namespace porytiles;

constexpr std::size_t DIAG_MARGIN_SIZE = 7;

void assert_arg_size(std::size_t expected, std::size_t actual, const char *func_name) {
    if (actual != expected) {
        panic(fmt::format("{}: found {} args but expected {}", func_name, actual, expected));
    }
}

template <typename T>
T any_cast_or_panic(const std::any &a, const std::source_location &loc) {
    try {
        return std::any_cast<T>(a);
    } catch (std::bad_any_cast &) {
        panic(fmt::format("bad any cast: {}:{}", loc.file_name(), loc.line()));
    }
}

template <typename T>
const T &any_cast_or_panic(const std::any *a, const std::source_location &loc) {
    auto any_unwrapped = any_cast<T>(a);
    if (any_unwrapped == nullptr) {
        panic(fmt::format("bad any cast: {}:{}", loc.file_name(), loc.line()));
    }
    return *any_unwrapped;
}

void push_to_ss_n_times(std::stringstream &ss, const std::string_view s, const std::size_t n) {
    for (std::size_t i = 0; i < n; i++) {
        ss << s;
    }
}

void reset_ss(std::stringstream &ss) {
    ss.clear();
    ss.str(std::string{});
}

std::vector<std::string> build_tile_pixel_highlight(const DiagEngine &eng, const DiagLevel in_flight_level,
                                                    const RGBATile &tile, const std::size_t row,
                                                    const std::size_t col) {
    std::vector<std::string> highlight{};
    std::stringstream ss{};
    const fmt::terminal_color level_color = color_for_level(in_flight_level);

    // TODO : std::variant here, see note below
    // Eventually we can remove this outer check by introducing better metadata
    // handling in RGBTile. Specifically, metadata can be a std::variant that
    // changes based on the TileType. Then, we can use the visitor pattern to
    // create different visit implementations for the different TileTypes.
    if (tile.type == TileType::LAYERED) {
        for (std::size_t i = 0; i < 16; i++) {
            for (std::size_t j = 0; j < 16; j++) {
                // First cell of each row is margin followed by a bar: "       |"
                if (j == 0) {
                    push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
                    ss << "|";
                }

                // General case. Decide if we are drawing the highlighted tile
                // and pixel. If not, draw a "-".

                auto styled_x = eng.style(" X ", fg(level_color) | fmt::emphasis::bold);
                auto styled_star = eng.style(" * ", fmt::emphasis::bold);
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
                    reset_ss(ss);
                }
            }

            // Insert a spacer line between top and bottom tiles
            if (i == 7) {
                push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
                ss << "|";
                highlight.push_back(ss.str());
                reset_ss(ss);
            }
        }
    }
    return highlight;
}

} // namespace

namespace porytiles {
std::string level_to_str(DiagLevel level) {
    switch (level) {
    case DiagLevel::Ignored:
        return "ignored";
    case DiagLevel::Note:
        return "note";
    case DiagLevel::Remark:
        return "remark";
    case DiagLevel::Warning:
        return "warning";
    case DiagLevel::Error:
        return "error";
    case DiagLevel::Fatal:
        return "fatal error";
    default:
        panic("level_to_str: unknown diag_level");
    }
}

fmt::terminal_color color_for_level(DiagLevel level) {
    switch (level) {
    case DiagLevel::Ignored:
        return fmt::terminal_color::white;
    case DiagLevel::Note:
        return fmt::terminal_color::cyan;
    case DiagLevel::Remark:
        return fmt::terminal_color::green;
    case DiagLevel::Warning:
        return fmt::terminal_color::magenta;
    case DiagLevel::Error:
    case DiagLevel::Fatal:
        return fmt::terminal_color::red;
    default:
        panic("color_for_level: unknown diag_level");
    }
}

int level_priority(DiagLevel level) {
    switch (level) {
    case DiagLevel::Ignored:
        return 0;
    case DiagLevel::Note:
        return 1;
    case DiagLevel::Remark:
        return 2;
    case DiagLevel::Warning:
        return 3;
    case DiagLevel::Error:
        return 4;
    case DiagLevel::Fatal:
        return 5;
    }
    return -1;
}

void IgnoreConsumer::consume(const InFlightDiag &diag) {
    consumed_count_++;
}

bool IgnoreConsumer::is_a_tty() const {
    return false;
}

InFlightDiag IgnoreConsumer::consumed_at(std::size_t i) const {
    panic("ignore_consumer::consumed_at: not implemented");
}

std::uint64_t IgnoreConsumer::consumed_count() const {
    return consumed_count_;
}

void stderr_consumer::consume(const InFlightDiag &diag) {
    consumed_count_++;
    const auto msg = diag.msg();
    std::fputs(msg.c_str(), stderr);
}

bool stderr_consumer::is_a_tty() const {
    return isatty(fileno(stderr));
}

InFlightDiag stderr_consumer::consumed_at(std::size_t i) const {
    panic("stderr_consumer::consumed_at: not implemented");
}

std::uint64_t stderr_consumer::consumed_count() const {
    return consumed_count_;
}

void VectorConsumer::consume(const InFlightDiag &diag) {
    diags_.emplace_back(diag);
}

bool VectorConsumer::is_a_tty() const {
    return false;
}

InFlightDiag VectorConsumer::consumed_at(std::size_t i) const {
    try {
        return diags_.at(i);
    } catch (const std::out_of_range &) {
        panic(fmt::format("vector_consumer::at: index {} out of range for size {}", i, diags_.size()));
    }
}

std::uint64_t VectorConsumer::consumed_count() const {
    return diags_.size();
}

// clang-format off
static const DiagTempl N_GENERIC_TEMPL{N_GENERIC, DiagLevel::Note, "{}", {}};

static const DiagTempl W_COLOR_PRECISION_LOSS_NOTE_TEMPL{
    "color-precision-loss-previously-seen-note",
    DiagLevel::Note,
    [](const DiagEngine &eng, const DiagLevel in_flight_level, const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(4, args.size(), std::source_location::current().function_name());

        const auto tile = any_cast_or_panic<RGBATile>(args[0], std::source_location::current());
        const auto color = any_cast_or_panic<std::string>(args[1], std::source_location::current());
        const auto row = any_cast_or_panic<std::size_t>(args[2], std::source_location::current());
        const auto col = any_cast_or_panic<std::size_t>(args[3], std::source_location::current());

        // FIXME : this template is incomplete, we want to show the mode since it's
        // possible to have precision loss across a primary-secondary boundary
        constexpr auto msg_templ = "{}: previously saw: '{}' at col '{}', row '{}'";
        std::vector<std::string> msg{};
        msg.push_back(fmt::format(msg_templ, eng.bold(tile.prettify()), eng.bold(color), eng.bold(col),
                                 eng.bold(row)));
        auto highlight = build_tile_pixel_highlight(eng, in_flight_level, tile, row, col);
        msg.insert(std::end(msg), std::begin(highlight), std::end(highlight));

        return msg;
    }
};
static const DiagTempl W_COLOR_PRECISION_LOSS_TEMPL{
    W_COLOR_PRECISION_LOSS,
    DiagLevel::Warning,
    [](const DiagEngine &eng, const DiagLevel in_flight_level, const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(5, args.size(), std::source_location::current().function_name());

        const auto tile = any_cast_or_panic<RGBATile>(args[0], std::source_location::current());
        const auto color = any_cast_or_panic<std::string>(args[1], std::source_location::current());
        const auto mode = any_cast_or_panic<std::string>(args[2], std::source_location::current());
        const auto row = any_cast_or_panic<std::size_t>(args[3], std::source_location::current());
        const auto col = any_cast_or_panic<std::size_t>(args[4], std::source_location::current());

        constexpr auto msg_templ = "{} {}: collapsed to duplicate BGR: '{}' at col '{}', row '{}'";
        std::vector<std::string> msg{};
        msg.push_back(fmt::format(msg_templ, eng.bold(mode), eng.bold(tile.prettify()), eng.bold(color), eng.bold(col),
                                  eng.bold(row)));
        auto highlight = build_tile_pixel_highlight(eng, in_flight_level, tile, row, col);
        msg.insert(std::end(msg), std::begin(highlight), std::end(highlight));

        return msg;
    },
    {
        W_COLOR_PRECISION_LOSS_NOTE_TEMPL
    }
};

// TODO : show mode information (primary vs secondary)
static const DiagTempl W_KEY_FRAME_NO_MATCHING_TILE_TEMPL{
    W_KEY_FRAME_NO_MATCHING_TILE,
    DiagLevel::Warning,
    "animation '{}' key frame tile '{}' was not present in any metatile entries",
    {}
};

// TODO : show mode information (primary vs secondary)
static const DiagTempl W_KEY_FRAME_MISSING_COLORS_NOTE_TEMPL{
    "key-frame-missing-colors-list-note",
    DiagLevel::Note,
    [](const DiagEngine &eng, const DiagLevel in_flight_level, const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(1, args.size(), std::source_location::current().function_name());

        const auto missing_colors =
            any_cast_or_panic<std::vector<RGBA32>>(&args[0], std::source_location::current());
        std::vector<std::string> msg{};
        msg.emplace_back("the following colors were missing from the key frame tile:");
        std::stringstream ss{};
        push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
        ss << "|--- {}";
        for (const auto &color : missing_colors) {
            msg.push_back(fmt::format(fmt::runtime(ss.str()), eng.bold(color.jasc())));
        }
        reset_ss(ss);
        push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
        ss << "| If left uncorrected, this may lead to the issue described here:";
        msg.push_back(ss.str());
        reset_ss(ss);
        push_to_ss_n_times(ss, " ", DIAG_MARGIN_SIZE);
        ss << "|    https://github.com/grunt-lucas/porytiles/issues/60";
        msg.push_back(ss.str());
        return msg;
    }
};
static const DiagTempl W_KEY_FRAME_MISSING_COLORS_TEMPL{
    W_KEY_FRAME_MISSING_COLORS,
    DiagLevel::Warning,
    [](const DiagEngine &eng, const DiagLevel in_flight_level, const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(2, args.size(), std::source_location::current().function_name());

        const auto anim_name = any_cast_or_panic<std::string>(args[0], std::source_location::current());
        const auto tile_index = any_cast_or_panic<std::size_t>(args[1], std::source_location::current());
        constexpr auto msg_templ = "anim '{}' key frame tile '{}' missing essential colors";
        std::vector<std::string> msg{};
        msg.push_back(fmt::format(msg_templ, eng.bold(anim_name), eng.bold(tile_index)));
        return msg;
    },
    {
        W_KEY_FRAME_MISSING_COLORS_NOTE_TEMPL
    }
};

// TODO : make message shorter, possibly shorten file name?
static const DiagTempl W_ATTRIBUTE_FORMAT_MISMATCH_TEMPL{
    W_ATTRIBUTE_FORMAT_MISMATCH,
    DiagLevel::Warning,
    "{}: too {} attribute columns for base game '{}'",
    {
        DiagTempl{
            "attribute-format-mismatch-note",
            DiagLevel::Note,
            "unspecified columns will receive default values"
        }
    }
};

static const DiagTempl W_MISSING_ATTRIBUTES_CSV_TEMPL{
    W_MISSING_ATTRIBUTES_CSV,
    DiagLevel::Warning,
    "{}: attributes.csv did not exist",
    {
        DiagTempl{
            "missing-attr-csv-note",
            DiagLevel::Note,
            "all attributes will receive default or inferred values"
        }
    }
};

static const DiagTempl W_UNUSED_ATTRIBUTE_TEMPL{
    W_UNUSED_ATTRIBUTE,
    DiagLevel::Warning,
    "found attribute for nonexistent metatile ID '{}'",
    {
        DiagTempl{
            "unused-attribute-note",
            DiagLevel::Note,
            "{} metatiles found at source path '{}'"
        }
    }
};

static const DiagTempl W_TRANSPARENCY_COLLAPSE_TEMPL{
    W_TRANSPARENCY_COLLAPSE,
    DiagLevel::Warning,
    "color '{}' at {} '{}' subtile pixel col '{}', row '{}' collapsed to transparent under BGR conversion",
    {
        DiagTempl{
            "transparency-collapse-note",
            DiagLevel::Note,
            "if you did not intend this pixel to be transparent, edit the color on the respective layer sheet"
        }
    }
};

static const DiagTempl W_UNUSED_MANUAL_PAL_COLOR_TEMPL{
    W_UNUSED_MANUAL_PAL_COLOR, DiagLevel::Warning, "{}: '{}' was not used in layers or anims", {}
};

static const DiagTempl W_TILE_INDEX_OUT_OF_RANGE_TEMPL{
    W_TILE_INDEX_OUT_OF_RANGE,
    DiagLevel::Warning,
    "{} '{}': tile index '{}' out of range (sheet size = {})",
    {
        DiagTempl{
            "tile-index-out-of-range-note",
            DiagLevel::Note,
            "substituting primary tile 0 (transparent tile) so decompilation can continue"
        }
    }
};

static const DiagTempl W_PALETTE_INDEX_OUT_OF_RANGE_TEMPL{
    W_PALETTE_INDEX_OUT_OF_RANGE,
    DiagLevel::Warning,
    "{} '{}': palette index '{}' out of range (numPalettesTotal = {})",
    {
        DiagTempl{
            "palette-index-out-of-range-note",
            DiagLevel::Note,
            "substituting palette 0 so decompilation can continue"
        }
    }
};

static const DiagTempl E_GENERIC_TEMPL{E_GENERIC, DiagLevel::Error, "{}", {}};

static const DiagTempl E_FATAL_GENERIC_TEMPL{E_FATAL_GENERIC, DiagLevel::Fatal, "{}", {}};

static const std::unordered_map<const char *, DiagTempl> DIAG_TEMPLS{
    // Standalone notes
    {N_GENERIC, N_GENERIC_TEMPL},

    // Tileset compilation warnings
    {W_COLOR_PRECISION_LOSS, W_COLOR_PRECISION_LOSS_TEMPL},
    {W_KEY_FRAME_NO_MATCHING_TILE, W_KEY_FRAME_NO_MATCHING_TILE_TEMPL},
    {W_KEY_FRAME_MISSING_COLORS, W_KEY_FRAME_MISSING_COLORS_TEMPL},
    {W_ATTRIBUTE_FORMAT_MISMATCH, W_ATTRIBUTE_FORMAT_MISMATCH_TEMPL},
    {W_MISSING_ATTRIBUTES_CSV, W_MISSING_ATTRIBUTES_CSV_TEMPL},
    {W_UNUSED_ATTRIBUTE, W_UNUSED_ATTRIBUTE_TEMPL},
    {W_TRANSPARENCY_COLLAPSE, W_TRANSPARENCY_COLLAPSE_TEMPL},
    {W_UNUSED_MANUAL_PAL_COLOR, W_UNUSED_MANUAL_PAL_COLOR_TEMPL},

    // Tileset decompilation warnings
    {W_TILE_INDEX_OUT_OF_RANGE, W_TILE_INDEX_OUT_OF_RANGE_TEMPL},
    {W_PALETTE_INDEX_OUT_OF_RANGE, W_PALETTE_INDEX_OUT_OF_RANGE_TEMPL},

    // Generic errors
    {E_GENERIC, E_GENERIC_TEMPL},
    {E_FATAL_GENERIC, E_FATAL_GENERIC_TEMPL}
};
// clang-format on

DiagTempl diag_templ_for(const std::string_view name) {
    assert_or_panic(DIAG_TEMPLS.contains(name.data()), fmt::format("diag_template_for: unknown diagnostic: {}", name));
    return DIAG_TEMPLS.at(name.data());
}

std::vector<const char *> all_diag_templ_names() {
    std::vector<const char *> keys{};
    keys.reserve(DIAG_TEMPLS.size());
    for (const auto &key : DIAG_TEMPLS | std::views::keys) {
        keys.push_back(key);
    }
    return keys;
}

} // namespace porytiles
