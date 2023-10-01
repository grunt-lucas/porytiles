#ifndef PORYTILES_CLI_OPTIONS_H
#define PORYTILES_CLI_OPTIONS_H

#include <string>

#include "errors_warnings.h"

namespace porytiles {

// ----------------------------
// |    OPTION DEFINITIONS    |
// ----------------------------
/*
 * We'll define all the options and help strings here to reduce code repetition. Some options will be shared between
 * subcommands so we want to avoid duplicating message strings, etc.
 */
// @formatter:off
// clang-format off


// Global options

const std::string HELP = "help";
const std::string HELP_SHORT = "h";
const std::string HELP_DESC =
"    -" + std::string{HELP_SHORT} + ", --" + HELP + "\n"
"         Print help message.\n";
constexpr int HELP_VAL = 0000;

const std::string VERBOSE = "verbose";
const std::string VERBOSE_SHORT = "v";
const std::string VERBOSE_DESC =
"    -" + std::string{VERBOSE_SHORT} + ", --" + VERBOSE + "\n"
"         Enable verbose logging to stderr.\n";
constexpr int VERBOSE_VAL = 0001;

const std::string VERSION = "version";
const std::string VERSION_SHORT = "V";
const std::string VERSION_DESC =
"    -" + std::string{VERSION_SHORT} + ", --" + VERSION + "\n"
"         Print version info.\n";
constexpr int VERSION_VAL = 0002;


// Driver options

const std::string OUTPUT = "output";
const std::string OUTPUT_SHORT = "o";
const std::string OUTPUT_DESC =
"        -" + std::string{OUTPUT_SHORT} + ", -" + OUTPUT + "=<OUTPUT-PATH>\n"
"             Output generated files to the directory specified by OUTPUT-PATH. If any element of\n"
"             OUTPUT-PATH does not exist, it will be created. Defaults to the current working\n"
"             directory (i.e. `.').\n";
constexpr int OUTPUT_VAL = 1000;

const std::string TILES_OUTPUT_PAL = "tiles-output-pal";
const std::string TILES_OUTPUT_PAL_DESC =
"        -" + TILES_OUTPUT_PAL + "=<MODE>\n"
"             Set the palette mode for the output `tiles.png'. Valid settings are `true-color' or\n"
"             `greyscale'. These settings are for human visual purposes only and have no effect on\n"
"             the final in-game tiles. Default value is `greyscale'.\n";
constexpr int TILES_OUTPUT_PAL_VAL = 1001;


// Tileset generation options

const std::string TARGET_BASE_GAME = "target-base-game";
const std::string TARGET_BASE_GAME_DESC =
"        -" + TARGET_BASE_GAME + "=<TARGET>\n"
"             Set the (de)compilation target base game to TARGET. This option affects default\n"
"             values for the fieldmap parameters, as well as the metatile attribute format. Valid\n"
"             settings for TARGET are `pokeemerald', `pokefirered', and `pokeruby'. If this option\n"
"             is not specified, defaults to `pokeemerald'. See the wiki docs for more information.\n";
constexpr int TARGET_BASE_GAME_VAL = 2000;

const std::string DUAL_LAYER = "dual-layer";
const std::string DUAL_LAYER_DESC =
"        -" + DUAL_LAYER + "\n"
"             Enable dual-layer compilation mode. The layer type will be inferred from your source\n"
"             layer PNGs, so compilation will error out if any metatiles contain content on all\n"
"             three layers. If this option is not supplied, Porytiles assumes you are compiling\n"
"             a triple-layer tileset.\n";
constexpr int DUAL_LAYER_VAL = 2001;

const std::string TRANSPARENCY_COLOR = "transparency-color";
const std::string TRANSPARENCY_COLOR_DESC =
"        -" + TRANSPARENCY_COLOR + "=<R,G,B>\n"
"             Select RGB color <R,G,B> to represent transparency in your layer source PNGs.\n"
"             Defaults to <255,0,255>.\n";
constexpr int TRANSPARENCY_COLOR_VAL = 2002;

const std::string DEFAULT_BEHAVIOR = "default-behavior";
const std::string DEFAULT_BEHAVIOR_DESC =
"        -" + DEFAULT_BEHAVIOR + "=<BEHAVIOR>\n"
"             Select the default behavior for metatiles that do not have an entry in the\n"
"             'attributes.csv' file. If unspecified, defaults to 'MB_NORMAL'.\n";
constexpr int DEFAULT_BEHAVIOR_VAL = 2003;

// Color assignment config options

const std::string ASSIGN_EXPLORE_CUTOFF = "assign-explore-cutoff";
const std::string ASSIGN_EXPLORE_CUTOFF_DESC =
"        -" + ASSIGN_EXPLORE_CUTOFF + "=<FACTOR>\n"
"             Select the cutoff FACTOR for palette assignment tree node exploration. Defaults to 2,\n"
"             which should be sufficient for most cases. Increase the number to let the algorithm\n"
"             run for longer before failing out.\n";
constexpr int ASSIGN_EXPLORE_CUTOFF_VAL = 3000;

const std::string ASSIGN_ALGO = "assign-algorithm";
const std::string ASSIGN_ALGO_DESC =
"        -" + ASSIGN_ALGO + "=<ALGORITHM>\n"
"             Select the assignment algorithm. Valid options are `dfs' and `bfs'. Default is `dfs'.\n";
constexpr int ASSIGN_ALGO_VAL = 3001;

const std::string BEST_BRANCHES = "best-branches";
const std::string BEST_BRANCHES_DESC =
"        -" + BEST_BRANCHES + "=<N>\n"
"             Use only the N most promising branches at each node in the assignment tree search.\n"
"             Specify `smart' instead of an integer to use a computed `smart' cutoff at each node\n"
"             instead of just a constant integer cutoff.\n";
constexpr int BEST_BRANCHES_VAL = 3002;

const std::string PRIMARY_ASSIGN_EXPLORE_CUTOFF = "primary-assign-explore-cutoff";
const std::string PRIMARY_ASSIGN_EXPLORE_CUTOFF_DESC =
"        -" + PRIMARY_ASSIGN_EXPLORE_CUTOFF + "=<FACTOR>\n"
"             Same as '-assign-explore-cutoff', but for the paired primary set.\n";
constexpr int PRIMARY_ASSIGN_EXPLORE_CUTOFF_VAL = 3003;

const std::string PRIMARY_ASSIGN_ALGO = "primary-assign-algorithm";
const std::string PRIMARY_ASSIGN_ALGO_DESC =
"        -" + PRIMARY_ASSIGN_ALGO + "=<ALGORITHM>\n"
"             Same as '-assign-algorithm', but for the paired primary set.\n";
constexpr int PRIMARY_ASSIGN_ALGO_VAL = 3004;

const std::string PRIMARY_BEST_BRANCHES = "primary-best-branches";
const std::string PRIMARY_BEST_BRANCHES_DESC =
"        -" + PRIMARY_BEST_BRANCHES + "=<N>\n"
"             Same as '-best-branches', but for the paired primary set.\n";
constexpr int PRIMARY_BEST_BRANCHES_VAL = 3005;


// Fieldmap override options

const std::string TILES_PRIMARY_OVERRIDE = "tiles-primary-override";
const std::string TILES_PRIMARY_OVERRIDE_DESC =
"        -" + TILES_PRIMARY_OVERRIDE + "=<N>\n"
"             Override the target base game's default number of primary set tiles. The value\n"
"             specified here should match the corresponding value in your project's `fieldmap.h'.\n"
"             Defaults to 512 (inherited from `pokeemerald' default target base game).\n";
constexpr int TILES_PRIMARY_OVERRIDE_VAL = 4000;

const std::string TILES_OVERRIDE_TOTAL = "tiles-total-override";
const std::string TILES_TOTAL_OVERRIDE_DESC =
"        -" + TILES_OVERRIDE_TOTAL + "=<N>\n"
"             Override the target base game's default number of total tiles. The value specified\n"
"             here should match the corresponding value in your project's `fieldmap.h'. Defaults\n"
"             to 1024 (inherited from `pokeemerald' default target base game).\n";
constexpr int TILES_TOTAL_OVERRIDE_VAL = 4001;

const std::string METATILES_OVERRIDE_PRIMARY = "metatiles-primary-override";
const std::string METATILES_PRIMARY_OVERRIDE_DESC =
"        -" + METATILES_OVERRIDE_PRIMARY + "=<N>\n"
"             Override the target base game's default number of primary set metatiles. The value\n"
"             specified here should match the corresponding value in your project's `fieldmap.h'.\n"
"             Defaults to 512 (inherited from `pokeemerald' default target base game).\n";
constexpr int METATILES_PRIMARY_OVERRIDE_VAL = 4002;

const std::string METATILES_OVERRIDE_TOTAL = "metatiles-total-override";
const std::string METATILES_TOTAL_OVERRIDE_DESC =
"        -" + METATILES_OVERRIDE_TOTAL + "=<N>\n"
"             Override the target base game's default number of total metatiles. The value specified\n"
"             here should match the corresponding value in your project's `fieldmap.h'. Defaults\n"
"             to 1024 (inherited from `pokeemerald' default target base game).\n";
constexpr int METATILES_TOTAL_OVERRIDE_VAL = 4003;

const std::string PALS_PRIMARY_OVERRIDE = "pals-primary-override";
const std::string PALS_PRIMARY_OVERRIDE_DESC =
"        -" + PALS_PRIMARY_OVERRIDE + "=<N>\n"
"             Override the target base game's default number of primary set palettes. The value\n"
"             specified here should match the corresponding value in your project's `fieldmap.h'.\n"
"             Defaults to 6 (inherited from `pokeemerald' default target base game).\n";
constexpr int PALS_PRIMARY_OVERRIDE_VAL = 4004;

const std::string PALS_TOTAL_OVERRIDE = "pals-total-override";
const std::string PALS_TOTAL_OVERRIDE_DESC =
"        -" + PALS_TOTAL_OVERRIDE + "=<N>\n"
"             Override the target base game's default number of total palettes. The value specified\n"
"             here should match the corresponding value in your project's `fieldmap.h'. Defaults\n"
"             Defaults to 13 (inherited from `pokeemerald' default target base game).\n";
constexpr int PALS_TOTAL_OVERRIDE_VAL = 4005;


// Warning options

const std::string WALL = "Wall";
const std::string WALL_DESC =
"        -" + WALL + "\n"
"             Enable all warnings.\n";
constexpr int WALL_VAL = 5000;

const std::string W_GENERAL = "W";
const std::string W_GENERAL_DESC =
"        -" + W_GENERAL + "<WARNING>, -" + W_GENERAL + "no-<WARNING>\n"
"             Explicitly enable warning WARNING, or explicitly disable it if the 'no' form of the\n"
"             option is specified. If WARNING is already off, the 'no' form will no-op. If more\n"
"             than one specifier for the same warning appears on the same command line, the\n"
"             right-most specifier will take precedence.\n";

const std::string WNONE = "Wnone";
const std::string WNONE_SHORT = "w";
const std::string WNONE_DESC =
"        -" + std::string{WNONE_SHORT} + ", -" + WNONE + "\n"
"             Disable all warnings.\n";
constexpr int WNONE_VAL = 5001;

const std::string WNO_ERROR = "Wno-error";
constexpr int WNO_ERROR_VAL = 5002;

const std::string WERROR = "Werror";
const std::string WERROR_DESC =
"        -" + WERROR + "[=<WARNING>], -" + WNO_ERROR + "=<WARNING>\n"
"             Force all enabled warnings to generate errors, or optionally force WARNING to enable\n"
"             as an error. If the 'no' form of the option is specified, downgrade WARNING from an\n"
"             error to the highest previously seen level. If WARNING is already off, the 'no' form\n"
"             will no-op. If more than one specifier for the same warning appears on the same\n"
"             command line, the right-most specifier will take precedence.\n";
constexpr int WERROR_VAL = 5003;

// Specific warnings
const std::string WCOLOR_PRECISION_LOSS = W_GENERAL + WARN_COLOR_PRECISION_LOSS;
const std::string WNO_COLOR_PRECISION_LOSS = W_GENERAL + "no-" + WARN_COLOR_PRECISION_LOSS;
constexpr int WCOLOR_PRECISION_LOSS_VAL = 50000;
constexpr int WNO_COLOR_PRECISION_LOSS_VAL = 60000;

const std::string WKEY_FRAME_DID_NOT_APPEAR = W_GENERAL + WARN_KEY_FRAME_DID_NOT_APPEAR;
const std::string WNO_KEY_FRAME_DID_NOT_APPEAR = W_GENERAL + "no-" + WARN_KEY_FRAME_DID_NOT_APPEAR;
constexpr int WKEY_FRAME_DID_NOT_APPEAR_VAL = 50010;
constexpr int WNO_KEY_FRAME_DID_NOT_APPEAR_VAL = 60010;

const std::string WUSED_TRUE_COLOR_MODE = W_GENERAL + WARN_USED_TRUE_COLOR_MODE;
const std::string WNO_USED_TRUE_COLOR_MODE = W_GENERAL + "no-" + WARN_USED_TRUE_COLOR_MODE;
constexpr int WUSED_TRUE_COLOR_MODE_VAL = 50020;
constexpr int WNO_USED_TRUE_COLOR_MODE_VAL = 60020;

const std::string WATTRIBUTE_FORMAT_MISMATCH = W_GENERAL + WARN_ATTRIBUTE_FORMAT_MISMATCH;
const std::string WNO_ATTRIBUTE_FORMAT_MISMATCH = W_GENERAL + "no-" + WARN_ATTRIBUTE_FORMAT_MISMATCH;
constexpr int WATTRIBUTE_FORMAT_MISMATCH_VAL = 50030;
constexpr int WNO_ATTRIBUTE_FORMAT_MISMATCH_VAL = 60030;

const std::string WMISSING_ATTRIBUTES_CSV = W_GENERAL + WARN_MISSING_ATTRIBUTES_CSV;
const std::string WNO_MISSING_ATTRIBUTES_CSV = W_GENERAL + "no-" + WARN_MISSING_ATTRIBUTES_CSV;
constexpr int WMISSING_ATTRIBUTES_CSV_VAL = 50040;
constexpr int WNO_MISSING_ATTRIBUTES_CSV_VAL = 60040;

const std::string WUNUSED_ATTRIBUTE = W_GENERAL + WARN_UNUSED_ATTRIBUTE;
const std::string WNO_UNUSED_ATTRIBUTE = W_GENERAL + "no-" + WARN_UNUSED_ATTRIBUTE;
constexpr int WUNUSED_ATTRIBUTE_VAL = 50060;
constexpr int WNO_UNUSED_ATTRIBUTE_VAL = 60060;

const std::string WTRANSPARENCY_COLLAPSE = W_GENERAL + WARN_TRANSPARENCY_COLLAPSE;
const std::string WNO_TRANSPARENCY_COLLAPSE = W_GENERAL + "no-" + WARN_TRANSPARENCY_COLLAPSE;
constexpr int WTRANSPARENCY_COLLAPSE_VAL = 50070;
constexpr int WNO_TRANSPARENCY_COLLAPSE_VAL = 60070;

// @formatter:on
// clang-format on

} // namespace porytiles

#endif // PORYTILES_CLI_OPTIONS_H