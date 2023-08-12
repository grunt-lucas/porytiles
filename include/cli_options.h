#ifndef PORYTILES_CLI_OPTIONS_H
#define PORYTILES_CLI_OPTIONS_H

#include <string>

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
const std::string HELP = "help";
constexpr char HELP_SHORT = "h"[0];
const std::string HELP_DESC =
"    -" + std::string{HELP_SHORT} + ", --" + HELP + "\n"
"        Print help message.\n";

const std::string VERBOSE = "verbose";
constexpr char VERBOSE_SHORT = "v"[0];
const std::string VERBOSE_DESC =
"    -" + std::string{VERBOSE_SHORT} + ", --" + VERBOSE + "\n"
"        Enable verbose logging to stderr.\n";

const std::string VERSION = "version";
constexpr char VERSION_SHORT = "V"[0];
const std::string VERSION_DESC =
"    -" + std::string{VERSION_SHORT} + ", --" + VERSION + "\n"
"        Print version info.\n";

const std::string OUTPUT = "output";
constexpr char OUTPUT_SHORT = "o"[0];
const std::string OUTPUT_DESC =
"        -" + std::string{OUTPUT_SHORT} + ", -" + OUTPUT + "=<OUTPUT-PATH>\n"
"            Output build files to the directory specified by OUTPUT-PATH. If any element\n"
"            of OUTPUT-PATH does not exist, it will be created. Defaults to the current\n"
"            working directory (i.e. `.').\n";

const std::string F_TILES_PRIMARY = "ftiles-primary";
const std::string F_TILES_PRIMARY_DESC =
"        -" + F_TILES_PRIMARY + "=<N>\n"
"            Set the number of tiles in a primary set to N. This value should match\n"
"            the corresponding value in your project's `include/fieldmap.h'.\n"
"            Defaults to 512 (the pokeemerald default).\n";
constexpr int F_TILES_PRIMARY_VAL = 1000;

const std::string F_TILES_TOTAL = "ftiles-total";
const std::string F_TILES_TOTAL_DESC =
"        -" + F_TILES_TOTAL + "=<N>\n"
"            Set the total number of tiles (primary + secondary) to N. This value\n"
"            should match the corresponding value in your project's\n"
"            `include/fieldmap.h'. Defaults to 1024 (the pokeemerald default).\n";
constexpr int F_TILES_TOTAL_VAL = 1001;

const std::string F_METATILES_PRIMARY = "fmetatiles-primary";
const std::string F_METATILES_PRIMARY_DESC =
"        -" + F_METATILES_PRIMARY + "=<N>\n"
"            Set the number of metatiles in a primary set to N. This value should\n"
"            match the corresponding value in your project's `include/fieldmap.h'.\n"
"            Defaults to 512 (the pokeemerald default).\n";
constexpr int F_METATILES_PRIMARY_VAL = 1002;

const std::string F_METATILES_TOTAL = "fmetatiles-total";
const std::string F_METATILES_TOTAL_DESC =
"        -" + F_METATILES_TOTAL + "=<N>\n"
"            Set the total number of metatiles (primary + secondary) to N. This\n"
"            value should match the corresponding value in your project's\n"
"            `include/fieldmap.h'. Defaults to 1024 (the pokeemerald default).\n";
constexpr int F_METATILES_TOTAL_VAL = 1003;

const std::string F_PALS_PRIMARY = "fpals-primary";
const std::string F_PALS_PRIMARY_DESC =
"        -" + F_PALS_PRIMARY + "=<N>\n"
"            Set the number of palettes in a primary set to N. This value should\n"
"            match the corresponding value in your project's `include/fieldmap.h'.\n"
"            Defaults to 6 (the pokeemerald default).\n";
constexpr int F_PALS_PRIMARY_VAL = 1004;

const std::string F_PALS_TOTAL = "fpals-total";
const std::string F_PALS_TOTAL_DESC =
"        -" + F_PALS_TOTAL + "=<N>\n"
"            Set the total number of palettes (primary + secondary) to N. This\n"
"            value should match the corresponding value in your project's\n"
"            `include/fieldmap.h'. Defaults to 13 (the pokeemerald default).\n";
constexpr int F_PALS_TOTAL_VAL = 1005;

const std::string M_TILES_OUTPUT_PAL = "mtiles-output-pal";
const std::string M_TILES_OUTPUT_PAL_DESC =
"        -" + M_TILES_OUTPUT_PAL + "=<MODE>\n"
"            Set the palette mode for the output `tiles.png'. Valid settings are `true-color' or\n"
"            `greyscale'. These settings are for human visual purposes only and have no effect on\n"
"            the final in-game tiles. Default value is `greyscale'.\n";
constexpr int M_TILES_OUTPUT_PAL_VAL = 1006;

const std::string M_TARGET_BASE_GAME = "mtarget-base-game";
const std::string M_TARGET_BASE_GAME_DESC =
"        -" + M_TARGET_BASE_GAME + "=<TARGET>\n"
"             Set the compilation target base game to TARGET. This option affects default values\n"
"             for the fieldmap parameters, as well as the metatile attribute format. Valid\n"
"             settings for TARGET are `pokeemerald', `pokefirered', and `pokeruby'. If this option\n"
"             is not specified, defaults to `pokeemerald'. See the wiki docs for more information.\n";
constexpr int M_TARGET_BASE_GAME_VAL = 1008;

const std::string WALL = "Wall";
const std::string WALL_DESC =
"        -" + WALL + "\n"
"            Enable all warnings.\n";
constexpr int WALL_VAL = 1011;

const std::string WERROR = "Werror";
const std::string WERROR_DESC =
"        -" + WERROR + "\n"
"            Force all enabled warnings to generate errors.\n";
constexpr int WERROR_VAL = 1012;
// @formatter:on
// clang-format on

} // namespace porytiles

#endif // PORYTILES_CLI_OPTIONS_H