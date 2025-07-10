#include "porytiles2/infra/diagnostics/Diagnostics.hpp"

#include <any>
#include <ranges>
#include <sstream>
#include <unistd.h>

#include "porytiles2/infra/diagnostics/DiagnosticEngine.hpp"
#include "porytiles2/templates/Panic.hpp"

namespace {

using namespace porytiles2;

constexpr std::size_t DIAG_MARGIN_SIZE = 7;

void AssertArgSize(std::size_t expected, std::size_t actual, const char *func_name) {
  if (actual != expected) {
    panic(fmt::format("{}: found {} args but expected {}", func_name, actual, expected));
  }
}

template <typename T> T AnyCastOrPanic(const std::any &a, const std::source_location &loc) {
  try {
    return std::any_cast<T>(a);
  } catch (std::bad_any_cast &) {
    panic(fmt::format("bad any cast: {}:{}", loc.file_name(), loc.line()));
  }
}

template <typename T> const T &AnyCastOrPanic(const std::any *a, const std::source_location &loc) {
  auto any_unwrapped = any_cast<T>(a);
  if (any_unwrapped == nullptr) {
    panic(fmt::format("bad any cast: {}:{}", loc.file_name(), loc.line()));
  }
  return *any_unwrapped;
}

void PushToStream(std::stringstream &ss, const std::string_view s, const std::size_t n) {
  for (std::size_t i = 0; i < n; i++) {
    ss << s;
  }
}

void ResetStream(std::stringstream &ss) {
  ss.clear();
  ss.str(std::string{});
}

// TODO : this is using code from the legacy library, refactor
// std::vector<std::string> BuildTileHighlight(const DiagEngine &eng, const
// DiagLevel in_flight_level,
//                                             const RGBATile &tile, const
//                                             std::size_t row, const
//                                             std::size_t col) {
//     std::vector<std::string> highlight{};
//     std::stringstream ss{};
//     const fmt::terminal_color level_color = ColorForLevel(in_flight_level);

//     // TODO : std::variant here, see note below
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
std::string LevelToStr(DiagLevel level) {
  switch (level) {
  case DiagLevel::kIgnored:
    return "ignored";
  case DiagLevel::kNote:
    return "note";
  case DiagLevel::kRemark:
    return "remark";
  case DiagLevel::kWarning:
    return "warning";
  case DiagLevel::kError:
    return "error";
  case DiagLevel::kFatal:
    return "fatal error";
  default:
    panic("level_to_str: unknown diag_level");
  }
}

fmt::terminal_color ColorForLevel(DiagLevel level) {
  switch (level) {
  case DiagLevel::kIgnored:
    return fmt::terminal_color::white;
  case DiagLevel::kNote:
    return fmt::terminal_color::cyan;
  case DiagLevel::kRemark:
    return fmt::terminal_color::green;
  case DiagLevel::kWarning:
    return fmt::terminal_color::magenta;
  case DiagLevel::kError:
  case DiagLevel::kFatal:
    return fmt::terminal_color::red;
  default:
    panic("color_for_level: unknown diag_level");
  }
}

int LevelPriority(DiagLevel level) {
  switch (level) {
  case DiagLevel::kIgnored:
    return 0;
  case DiagLevel::kNote:
    return 1;
  case DiagLevel::kRemark:
    return 2;
  case DiagLevel::kWarning:
    return 3;
  case DiagLevel::kError:
    return 4;
  case DiagLevel::kFatal:
    return 5;
  }
  return -1;
}

void IgnoreConsumer::Consume(const InFlightDiag &diag) { consumed_count_++; }

bool IgnoreConsumer::IsATty() const { return false; }

InFlightDiag IgnoreConsumer::ConsumedAt(std::size_t i) const {
  panic("ignore_consumer::consumed_at: not implemented");
}

std::uint64_t IgnoreConsumer::ConsumedCount() const { return consumed_count_; }

void StderrConsumer::Consume(const InFlightDiag &diag) {
  consumed_count_++;
  const auto msg = diag.msg();
  std::fputs(msg.c_str(), stderr);
}

bool StderrConsumer::IsATty() const { return isatty(fileno(stderr)); }

InFlightDiag StderrConsumer::ConsumedAt(std::size_t i) const {
  panic("stderr_consumer::consumed_at: not implemented");
}

std::uint64_t StderrConsumer::ConsumedCount() const { return consumed_count_; }

void VectorConsumer::Consume(const InFlightDiag &diag) { diags_.emplace_back(diag); }

bool VectorConsumer::IsATty() const { return false; }

InFlightDiag VectorConsumer::ConsumedAt(std::size_t i) const {
  try {
    return diags_.at(i);
  } catch (const std::out_of_range &) {
    panic(fmt::format("vector_consumer::at: index {} out of range for size {}", i, diags_.size()));
  }
}

std::uint64_t VectorConsumer::ConsumedCount() const { return diags_.size(); }

static const DiagTempl N_GENERIC_TEMPL{NoteGeneric, DiagLevel::kNote, "{}", {}};

static const DiagTempl W_COLOR_PRECISION_LOSS_NOTE_TEMPL{
    "color-precision-loss-previously-seen-note", DiagLevel::kNote,
    [](const DiagEngine &eng, const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
      AssertArgSize(4, args.size(), std::source_location::current().function_name());
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
static const DiagTempl W_COLOR_PRECISION_LOSS_TEMPL{
    WarnColorPrecisionLoss,
    DiagLevel::kWarning,
    [](const DiagEngine &eng, const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
      AssertArgSize(5, args.size(), std::source_location::current().function_name());
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
    {W_COLOR_PRECISION_LOSS_NOTE_TEMPL}};

// TODO : show mode information (primary vs secondary)
static const DiagTempl W_KEY_FRAME_NO_MATCHING_TILE_TEMPL{
    WarnKeyFrameNoMatchingTile,
    DiagLevel::kWarning,
    "animation '{}' key frame tile '{}' was not present in any metatile "
    "entries",
    {}};

// TODO : show mode information (primary vs secondary)
static const DiagTempl W_KEY_FRAME_MISSING_COLORS_NOTE_TEMPL{
    "key-frame-missing-colors-list-note", DiagLevel::kNote,
    [](const DiagEngine &eng, const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
      AssertArgSize(1, args.size(), std::source_location::current().function_name());
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
static const DiagTempl W_KEY_FRAME_MISSING_COLORS_TEMPL{
    WarnKeyFrameMissingColors,
    DiagLevel::kWarning,
    [](const DiagEngine &eng, const DiagLevel in_flight_level,
       const std::vector<std::any> &args) -> std::vector<std::string> {
      AssertArgSize(2, args.size(), std::source_location::current().function_name());
      std::vector<std::string> msg{};

      const auto anim_name = AnyCastOrPanic<std::string>(args[0], std::source_location::current());
      const auto tile_index = AnyCastOrPanic<std::size_t>(args[1], std::source_location::current());
      constexpr auto msg_templ = "anim '{}' key frame tile '{}' missing essential colors";

      msg.push_back(fmt::format(msg_templ, eng.Bold(anim_name), eng.Bold(tile_index)));
      return msg;
    },
    {W_KEY_FRAME_MISSING_COLORS_NOTE_TEMPL}};

// TODO : make message shorter, possibly shorten file name?
static const DiagTempl W_ATTRIBUTE_FORMAT_MISMATCH_TEMPL{
    WarnAttributeFormatMismatch,
    DiagLevel::kWarning,
    "{}: too {} attribute columns for base game '{}'",
    {DiagTempl{"attribute-format-mismatch-note", DiagLevel::kNote,
               "unspecified columns will receive default values"}}};

static const DiagTempl W_MISSING_ATTRIBUTES_CSV_TEMPL{
    WarnMissingAttributesCsv,
    DiagLevel::kWarning,
    "{}: attributes.csv did not exist",
    {DiagTempl{"missing-attr-csv-note", DiagLevel::kNote,
               "all attributes will receive default or inferred values"}}};

static const DiagTempl W_UNUSED_ATTRIBUTE_TEMPL{
    WarnUnusedAttribute,
    DiagLevel::kWarning,
    "found attribute for nonexistent metatile ID '{}'",
    {DiagTempl{"unused-attribute-note", DiagLevel::kNote,
               "{} metatiles found at source path '{}'"}}};

static const DiagTempl W_TRANSPARENCY_COLLAPSE_TEMPL{
    WarnTransparencyCollapse,
    DiagLevel::kWarning,
    "color '{}' at {} '{}' subtile pixel col '{}', row '{}' collapsed to "
    "transparent under BGR conversion",
    {DiagTempl{"transparency-collapse-note", DiagLevel::kNote,
               "if you did not intend this pixel to be transparent, edit the "
               "color on the respective layer sheet"}}};

static const DiagTempl W_UNUSED_MANUAL_PAL_COLOR_TEMPL{
    WarnUnusedManualPalColor, DiagLevel::kWarning, "{}: '{}' was not used in layers or anims", {}};

static const DiagTempl W_TILE_INDEX_OUT_OF_RANGE_TEMPL{
    WarnTileIndexOutOfRange,
    DiagLevel::kWarning,
    "{} '{}': tile index '{}' out of range (sheet size = {})",
    {DiagTempl{"tile-index-out-of-range-note", DiagLevel::kNote,
               "substituting primary tile 0 (transparent tile) so "
               "decompilation can continue"}}};

static const DiagTempl W_PALETTE_INDEX_OUT_OF_RANGE_TEMPL{
    WarnPaletteIndexOutOfRange,
    DiagLevel::kWarning,
    "{} '{}': palette index '{}' out of range (numPalettesTotal = {})",
    {DiagTempl{"palette-index-out-of-range-note", DiagLevel::kNote,
               "substituting palette 0 so decompilation can continue"}}};

static const DiagTempl E_GENERIC_TEMPL{ErrGeneric, DiagLevel::kError, "{}", {}};

static const DiagTempl E_FATAL_GENERIC_TEMPL{FatalGeneric, DiagLevel::kFatal, "{}", {}};

static const std::unordered_map<const char *, DiagTempl> DIAG_TEMPLS{
    // Standalone notes
    {NoteGeneric, N_GENERIC_TEMPL},

    // Tileset compilation warnings
    {WarnColorPrecisionLoss, W_COLOR_PRECISION_LOSS_TEMPL},
    {WarnKeyFrameNoMatchingTile, W_KEY_FRAME_NO_MATCHING_TILE_TEMPL},
    {WarnKeyFrameMissingColors, W_KEY_FRAME_MISSING_COLORS_TEMPL},
    {WarnAttributeFormatMismatch, W_ATTRIBUTE_FORMAT_MISMATCH_TEMPL},
    {WarnMissingAttributesCsv, W_MISSING_ATTRIBUTES_CSV_TEMPL},
    {WarnUnusedAttribute, W_UNUSED_ATTRIBUTE_TEMPL},
    {WarnTransparencyCollapse, W_TRANSPARENCY_COLLAPSE_TEMPL},
    {WarnUnusedManualPalColor, W_UNUSED_MANUAL_PAL_COLOR_TEMPL},

    // Tileset decompilation warnings
    {WarnTileIndexOutOfRange, W_TILE_INDEX_OUT_OF_RANGE_TEMPL},
    {WarnPaletteIndexOutOfRange, W_PALETTE_INDEX_OUT_OF_RANGE_TEMPL},

    // Generic errors
    {ErrGeneric, E_GENERIC_TEMPL},
    {FatalGeneric, E_FATAL_GENERIC_TEMPL}};

DiagTempl DiagFor(const std::string_view name) {
  AssertOrPanic(DIAG_TEMPLS.contains(name.data()),
                fmt::format("diag_template_for: unknown diagnostic: {}", name));
  return DIAG_TEMPLS.at(name.data());
}

std::vector<const char *> AllDiagNames() {
  std::vector<const char *> keys{};
  keys.reserve(DIAG_TEMPLS.size());
  for (const auto &key : DIAG_TEMPLS | std::views::keys) {
    keys.push_back(key);
  }
  return keys;
}

std::vector<const char *> AllDiagNames(const DiagLevel level) {
  std::vector<const char *> keys{};
  keys.reserve(DIAG_TEMPLS.size());
  for (const auto &[name, templ] : DIAG_TEMPLS) {
    if (templ.level() == level) {
      keys.push_back(name);
    }
  }
  return keys;
}

} // namespace porytiles2
