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
"            Output compiled files to the directory specified by OUTPUT-PATH. If any element of\n"
"            OUTPUT-PATH does not exist, it will be created. Defaults to the current working\n"
"            directory (i.e. `.').\n";

const std::string TILES_PRIMARY_OVERRIDE = "tiles-primary-override";
const std::string TILES_PRIMARY_OVERRIDE_DESC =
"        -" + TILES_PRIMARY_OVERRIDE + "=<N>\n"
"            Override the target base game's default number of primary set tiles. The value\n"
"            specified here should match the corresponding value in your project's `fieldmap.h'.\n"
"            Defaults to 512 (inherited from `pokeemerald' default target base game).\n";
constexpr int TILES_PRIMARY_OVERRIDE_VAL = 1000;

const std::string TILES_OVERRIDE_TOTAL = "tiles-total-override";
const std::string TILES_TOTAL_OVERRIDE_DESC =
"        -" + TILES_OVERRIDE_TOTAL + "=<N>\n"
"            Override the target base game's default number of total tiles. The value specified\n"
"            here should match the corresponding value in your project's `fieldmap.h'. Defaults\n"
"            to 1024 (inherited from `pokeemerald' default target base game).\n";
constexpr int TILES_TOTAL_OVERRIDE_VAL = 1001;

const std::string METATILES_OVERRIDE_PRIMARY = "metatiles-primary-override";
const std::string METATILES_PRIMARY_OVERRIDE_DESC =
"        -" + METATILES_OVERRIDE_PRIMARY + "=<N>\n"
"            Override the target base game's default number of primary set metatiles. The value\n"
"            specified here should match the corresponding value in your project's `fieldmap.h'.\n"
"            Defaults to 512 (inherited from `pokeemerald' default target base game).\n";
constexpr int METATILES_PRIMARY_OVERRIDE_VAL = 1002;

const std::string METATILES_OVERRIDE_TOTAL = "metatiles-total-override";
const std::string METATILES_TOTAL_OVERRIDE_DESC =
"        -" + METATILES_OVERRIDE_TOTAL + "=<N>\n"
"            Override the target base game's default number of total metatiles. The value specified\n"
"            here should match the corresponding value in your project's `fieldmap.h'. Defaults\n"
"            to 1024 (inherited from `pokeemerald' default target base game).\n";
constexpr int METATILES_TOTAL_OVERRIDE_VAL = 1003;

const std::string PALS_PRIMARY_OVERRIDE = "pals-primary-override";
const std::string PALS_PRIMARY_OVERRIDE_DESC =
"        -" + PALS_PRIMARY_OVERRIDE + "=<N>\n"
"            Override the target base game's default number of primary set palettes. The value\n"
"            specified here should match the corresponding value in your project's `fieldmap.h'.\n"
"            Defaults to 6 (inherited from `pokeemerald' default target base game).\n";
constexpr int PALS_PRIMARY_OVERRIDE_VAL = 1004;

const std::string PALS_TOTAL_OVERRIDE = "pals-total-override";
const std::string PALS_TOTAL_OVERRIDE_DESC =
"        -" + PALS_TOTAL_OVERRIDE + "=<N>\n"
"            Override the target base game's default number of total palettes. The value specified\n"
"            here should match the corresponding value in your project's `fieldmap.h'. Defaults\n"
"            Defaults to 13 (inherited from `pokeemerald' default target base game).\n";
constexpr int PALS_TOTAL_OVERRIDE_VAL = 1005;

const std::string TILES_OUTPUT_PAL = "tiles-output-pal";
const std::string TILES_OUTPUT_PAL_DESC =
"        -" + TILES_OUTPUT_PAL + "=<MODE>\n"
"            Set the palette mode for the output `tiles.png'. Valid settings are `true-color' or\n"
"            `greyscale'. These settings are for human visual purposes only and have no effect on\n"
"            the final in-game tiles. Default value is `greyscale'.\n";
constexpr int TILES_OUTPUT_PAL_VAL = 1006;

const std::string TARGET_BASE_GAME = "target-base-game";
const std::string TARGET_BASE_GAME_DESC =
"        -" + TARGET_BASE_GAME + "=<TARGET>\n"
"             Set the compilation target base game to TARGET. This option affects default values\n"
"             for the fieldmap parameters, as well as the metatile attribute format. Valid\n"
"             settings for TARGET are `pokeemerald', `pokefirered', and `pokeruby'. If this option\n"
"             is not specified, defaults to `pokeemerald'. See the wiki docs for more information.\n";
constexpr int TARGET_BASE_GAME_VAL = 1008;

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