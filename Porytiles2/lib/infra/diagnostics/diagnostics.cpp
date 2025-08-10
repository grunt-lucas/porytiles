#include "porytiles2/infra/diagnostics/diagnostics.hpp"

#include <any>
#include <ranges>
#include <sstream>
#include <unistd.h>

#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/templates/panic.hpp"

namespace {

using namespace porytiles2;

constexpr std::size_t diag_margin_size = 7;

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

void push_to_stream(std::stringstream &ss, const std::string_view s, const std::size_t n) {
    for (std::size_t i = 0; i < n; i++) {
        ss << s;
    }
}

void reset_stream(std::stringstream &ss) {
    ss.clear();
    ss.str(std::string{});
}

// TODO: this is using code from the legacy library, refactor
// std::vector<std::string> BuildTileHighlight(const DiagEngine &eng, const
// DiagLevel in_flight_level,
//                                             const RGBATile &tile, const
//                                             std::size_t row, const
//                                             std::size_t col) {
//     std::vector<std::string> highlight{};
//     std::stringstream ss{};
//     const fmt::terminal_color level_color = ColorForLevel(in_flight_level);

//     // TODO: std::variant here, see note below
//     // Eventually we can remove this outer check by introducing better
//     metadata
//     // handling in RGBTile. Specifically, metadata can be a std::variant that
//     // changes based on the TileType. Then, we can use the visitor pattern to
//     // create different visit implementations for the different TileTypes.
//     if (tile.type == TileType::LAYERED) {
//         for (std::size_t i = 0; i < 16; i++) {
//             for (std::size_t j = 0; j < 16; j++) {
//                 // First cell of each row is margin followed by a bar: " |"
//                 if (j == 0) {
//                     PushToStream(ss, " ", DIAG_MARGIN_SIZE);
//                     ss << "|";
//                 }

//                 // General case. Decide if we are drawing the highlighted
//                 tile
//                 // and pixel. If not, draw a "-".

//                 auto styled_x = eng.Style(" X ", fg(level_color) |
//                 fmt::emphasis::bold); auto styled_star = eng.Style(" * ",
//                 fmt::emphasis::bold); if (tile.subtile == Subtile::NORTHWEST
//                 && i < 8 && j < 8) {
//                     if (row == i && col == j) {
//                         ss << format(fmt::runtime("{}"), styled_x);
//                     } else {
//                         ss << format(fmt::runtime("{}"), styled_star);
//                     }
//                 } else if (tile.subtile == Subtile::NORTHEAST && i < 8 && j
//                 >= 8) {
//                     if (row == i && col == j - 8) {
//                         ss << format(fmt::runtime("{}"), styled_x);
//                     } else {
//                         ss << format(fmt::runtime("{}"), styled_star);
//                     }
//                 } else if (tile.subtile == Subtile::SOUTHWEST && i >= 8 && j
//                 < 8) {
//                     if (row == i - 8 && col == j) {
//                         ss << format(fmt::runtime("{}"), styled_x);
//                     } else {
//                         ss << format(fmt::runtime("{}"), styled_star);
//                     }
//                 } else if (tile.subtile == Subtile::SOUTHEAST && i >= 8 && j
//                 >= 8) {
//                     if (row == i - 8 && col == j - 8) {
//                         ss << format(fmt::runtime("{}"), styled_x);
//                     } else {
//                         ss << format(fmt::runtime("{}"), styled_star);
//                     }
//                 } else {
//                     ss << " - ";
//                 }

//                 // If we're at the midpoint cell, add an extra space.
//                 if (j == 7) {
//                     ss << " ";
//                 }

//                 // Reset once this row is exhausted
//                 if (j == 15) {
//                     highlight.push_back(ss.str());
//                     ResetStream(ss);
//                 }
//             }

//             // Insert a spacer line between top and bottom tiles
//             if (i == 7) {
//                 PushToStream(ss, " ", DIAG_MARGIN_SIZE);
//                 ss << "|";
//                 highlight.push_back(ss.str());
//                 ResetStream(ss);
//             }
//         }
//     }
//     return highlight;
// }

} // namespace

namespace porytiles2 {
std::string level_to_str(DiagLevel level) {
    switch (level) {
    case DiagLevel::ignored:
        return "ignored";
    case DiagLevel::note:
        return "note";
    case DiagLevel::remark:
        return "remark";
    case DiagLevel::warning:
        return "warning";
    case DiagLevel::error:
        return "error";
    case DiagLevel::fatal:
        return "fatal error";
    default:
        panic("level_to_str: unknown diag_level");
    }
}

fmt::terminal_color color_for_level(DiagLevel level) {
    switch (level) {
    case DiagLevel::ignored:
        return fmt::terminal_color::white;
    case DiagLevel::note:
        return fmt::terminal_color::cyan;
    case DiagLevel::remark:
        return fmt::terminal_color::green;
    case DiagLevel::warning:
        return fmt::terminal_color::magenta;
    case DiagLevel::error:
    case DiagLevel::fatal:
        return fmt::terminal_color::red;
    default:
        panic("color_for_level: unknown diag_level");
    }
}

int level_priority(DiagLevel level) {
    switch (level) {
    case DiagLevel::ignored:
        return 0;
    case DiagLevel::note:
        return 1;
    case DiagLevel::remark:
        return 2;
    case DiagLevel::warning:
        return 3;
    case DiagLevel::error:
        return 4;
    case DiagLevel::fatal:
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

void StderrConsumer::consume(const InFlightDiag &diag) {
    consumed_count_++;
    const auto msg = diag.msg();
    std::fputs(msg.c_str(), stderr);
}

bool StderrConsumer::is_a_tty() const {
    return isatty(fileno(stderr));
}

InFlightDiag StderrConsumer::consumed_at(std::size_t i) const {
    panic("stderr_consumer::consumed_at: not implemented");
}

std::uint64_t StderrConsumer::consumed_count() const {
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

static const DiagTempl n_generic_templ{note_generic, DiagLevel::note, "{}", {}};

static const DiagTempl w_color_precision_loss_note_templ{
    "color-precision-loss-previously-seen-note",
    DiagLevel::note,
    [](const DiagEngine &eng,
       const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(4, args.size(), std::source_location::current().function_name());
        std::vector<std::string> msg{};

        // const auto tile = AnyCastOrPanic<RGBATile>(args[0],
        // std::source_location::current()); const auto color =
        // AnyCastOrPanic<std::string>(args[1], std::source_location::current());
        // const auto row = AnyCastOrPanic<std::size_t>(args[2],
        // std::source_location::current()); const auto col =
        // AnyCastOrPanic<std::size_t>(args[3], std::source_location::current());

        // // FIXME : this template is incomplete, we want to show the mode since
        // it's
        // // possible to have precision loss across a primary-secondary boundary
        // constexpr auto msg_templ = "{}: previously saw: '{}' at col '{}', row
        // '{}'"; msg.push_back(fmt::format(msg_templ, eng.Bold(tile.prettify()),
        // eng.Bold(color), eng.Bold(col),
        //                          eng.Bold(row)));
        // // auto highlight = BuildTileHighlight(eng, in_flight_level, tile, row,
        // col);
        // // msg.insert(std::end(msg), std::begin(highlight),
        // std::end(highlight));

        return msg;
    }};
static const DiagTempl w_color_precision_loss_templ{
    warn_color_precision_loss,
    DiagLevel::warning,
    [](const DiagEngine &eng,
       const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(5, args.size(), std::source_location::current().function_name());
        std::vector<std::string> msg{};

        // const auto tile = AnyCastOrPanic<RGBATile>(args[0],
        // std::source_location::current()); const auto color =
        // AnyCastOrPanic<std::string>(args[1],
        // std::source_location::current()); const auto mode =
        // AnyCastOrPanic<std::string>(args[2],
        // std::source_location::current()); const auto row =
        // AnyCastOrPanic<std::size_t>(args[3],
        // std::source_location::current()); const auto col =
        // AnyCastOrPanic<std::size_t>(args[4],
        // std::source_location::current());

        // constexpr auto msg_templ = "{} {}: collapsed to duplicate BGR:
        // '{}' at col '{}', row '{}'";
        // msg.push_back(fmt::format(msg_templ, eng.Bold(mode),
        // eng.Bold(tile.prettify()), eng.Bold(color), eng.Bold(col),
        //                           eng.Bold(row)));
        // auto highlight = BuildTileHighlight(eng, in_flight_level, tile,
        // row, col); msg.insert(std::end(msg), std::begin(highlight),
        // std::end(highlight));

        return msg;
    },
    {w_color_precision_loss_note_templ}};

// TODO: show mode information (primary vs secondary)
static const DiagTempl w_key_frame_no_matching_tile_templ{
    warn_key_frame_no_matching_tile,
    DiagLevel::warning,
    "animation '{}' key frame tile '{}' was not present in any metatile "
    "entries",
    {}};

// TODO: show mode information (primary vs secondary)
static const DiagTempl w_key_frame_missing_colors_note_templ{
    "key-frame-missing-colors-list-note",
    DiagLevel::note,
    [](const DiagEngine &eng,
       const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(1, args.size(), std::source_location::current().function_name());
        std::vector<std::string> msg{};

        // const auto missing_colors =
        //     AnyCastOrPanic<std::vector<RGBA32>>(&args[0],
        //     std::source_location::current());
        // msg.emplace_back("the following colors were missing from the key frame
        // tile:"); std::stringstream ss{}; PushToStream(ss, " ",
        // DIAG_MARGIN_SIZE); ss << "|--- {}"; for (const auto &color :
        // missing_colors) {
        //     msg.push_back(fmt::format(fmt::runtime(ss.str()),
        //     eng.Bold(color.jasc())));
        // }
        // ResetStream(ss);
        // PushToStream(ss, " ", DIAG_MARGIN_SIZE);
        // ss << "| If left uncorrected, this may lead to the issue described
        // here:"; msg.push_back(ss.str()); ResetStream(ss); PushToStream(ss, " ",
        // DIAG_MARGIN_SIZE); ss << "|
        // https://github.com/grunt-lucas/porytiles/issues/60";
        // msg.push_back(ss.str());
        return msg;
    }};
static const DiagTempl w_key_frame_missing_colors_templ{
    warn_key_frame_missing_colors,
    DiagLevel::warning,
    [](const DiagEngine &eng,
       const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
        assert_arg_size(2, args.size(), std::source_location::current().function_name());
        std::vector<std::string> msg{};

        const auto anim_name = any_cast_or_panic<std::string>(args[0], std::source_location::current());
        const auto tile_index = any_cast_or_panic<std::size_t>(args[1], std::source_location::current());
        constexpr auto msg_templ = "anim '{}' key frame tile '{}' missing essential colors";

        msg.push_back(fmt::format(msg_templ, eng.Bold(anim_name), eng.Bold(tile_index)));
        return msg;
    },
    {w_key_frame_missing_colors_note_templ}};

// TODO: make message shorter, possibly shorten file name?
static const DiagTempl w_attribute_format_mismatch_templ{
    warn_attribute_format_mismatch,
    DiagLevel::warning,
    "{}: too {} attribute columns for base game '{}'",
    {DiagTempl{"attribute-format-mismatch-note", DiagLevel::note, "unspecified columns will receive default values"}}};

static const DiagTempl w_missing_attributes_csv_templ{
    warn_missing_attributes_csv,
    DiagLevel::warning,
    "{}: attributes.csv did not exist",
    {DiagTempl{"missing-attr-csv-note", DiagLevel::note, "all attributes will receive default or inferred values"}}};

static const DiagTempl w_unused_attribute_templ{
    warn_unused_attribute,
    DiagLevel::warning,
    "found attribute for nonexistent metatile ID '{}'",
    {DiagTempl{"unused-attribute-note", DiagLevel::note, "{} metatiles found at source path '{}'"}}};

static const DiagTempl w_transparency_collapse_templ{
    warn_transparency_collapse,
    DiagLevel::warning,
    "color '{}' at {} '{}' subtile pixel col '{}', row '{}' collapsed to "
    "transparent under BGR conversion",
    {DiagTempl{
        "transparency-collapse-note",
        DiagLevel::note,
        "if you did not intend this pixel to be transparent, edit the "
        "color on the respective layer sheet"}}};

static const DiagTempl w_unused_manual_pal_color_templ{
    warn_unused_manual_pal_color, DiagLevel::warning, "{}: '{}' was not used in layers or anims", {}};

static const DiagTempl w_tile_index_out_of_range_templ{
    warn_tile_index_out_of_range,
    DiagLevel::warning,
    "{} '{}': tile index '{}' out of range (sheet size = {})",
    {DiagTempl{
        "tile-index-out-of-range-note",
        DiagLevel::note,
        "substituting primary tile 0 (transparent tile) so "
        "decompilation can continue"}}};

static const DiagTempl w_palette_index_out_of_range_templ{
    warn_palette_index_out_of_range,
    DiagLevel::warning,
    "{} '{}': palette index '{}' out of range (numPalettesTotal = {})",
    {DiagTempl{
        "palette-index-out-of-range-note", DiagLevel::note, "substituting palette 0 so decompilation can continue"}}};

static const DiagTempl e_generic_templ{err_generic, DiagLevel::error, "{}", {}};

static const DiagTempl e_fatal_generic_templ{fatal_generic, DiagLevel::fatal, "{}", {}};

static const std::unordered_map<const char *, DiagTempl> diag_templs{
    // Standalone notes
    {note_generic, n_generic_templ},

    // Tileset compilation warnings
    {warn_color_precision_loss, w_color_precision_loss_templ},
    {warn_key_frame_no_matching_tile, w_key_frame_no_matching_tile_templ},
    {warn_key_frame_missing_colors, w_key_frame_missing_colors_templ},
    {warn_attribute_format_mismatch, w_attribute_format_mismatch_templ},
    {warn_missing_attributes_csv, w_missing_attributes_csv_templ},
    {warn_unused_attribute, w_unused_attribute_templ},
    {warn_transparency_collapse, w_transparency_collapse_templ},
    {warn_unused_manual_pal_color, w_unused_manual_pal_color_templ},

    // Tileset decompilation warnings
    {warn_tile_index_out_of_range, w_tile_index_out_of_range_templ},
    {warn_palette_index_out_of_range, w_palette_index_out_of_range_templ},

    // Generic errors
    {err_generic, e_generic_templ},
    {fatal_generic, e_fatal_generic_templ}};

DiagTempl diag_for(const std::string_view name) {
    assert_or_panic(diag_templs.contains(name.data()), fmt::format("diag_template_for: unknown diagnostic: {}", name));
    return diag_templs.at(name.data());
}

std::vector<const char *> all_diag_names() {
    std::vector<const char *> keys{};
    keys.reserve(diag_templs.size());
    for (const auto &key : diag_templs | std::views::keys) {
        keys.push_back(key);
    }
    return keys;
}

std::vector<const char *> all_diag_names(const DiagLevel level) {
    std::vector<const char *> keys{};
    keys.reserve(diag_templs.size());
    for (const auto &[name, templ] : diag_templs) {
        if (templ.level() == level) {
            keys.push_back(name);
        }
    }
    return keys;
}

} // namespace porytiles2
