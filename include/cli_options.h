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
const std::string HELP_LONG = "help";
constexpr char HELP_SHORT = "h"[0];
const std::string HELP_DESCRIPTION =
"    -" + std::string{HELP_SHORT} + ", --" + HELP_LONG + "\n"
"        Print help message.\n";

const std::string VERBOSE_LONG = "verbose";
constexpr char VERBOSE_SHORT = "v"[0];
const std::string VERBOSE_DESCRIPTION = 
"    -" + std::string{VERBOSE_SHORT} + ", --" + VERBOSE_LONG + "\n"
"        Enable verbose logging to stderr.\n";

const std::string VERSION_LONG = "version";
constexpr char VERSION_SHORT = "V"[0];
const std::string VERSION_DESCRIPTION = 
"    -" + std::string{VERSION_SHORT} + ", --" + VERSION_LONG + "\n"
"        Print version info.\n";

const std::string OUTPUT_LONG = "output";
constexpr char OUTPUT_SHORT = "o"[0];
const std::string OUTPUT_DESCRIPTION =
"    -" + std::string{OUTPUT_SHORT} + ", --" + OUTPUT_LONG + "=<PATH>\n"
"        Output build files to the directory specified by PATH. If any element\n"
"        of PATH does not exist, it will be created. Defaults to the current\n"
"        working directory (i.e. `.').\n";

const std::string NUM_TILES_IN_PRIMARY_LONG = "num-tiles-primary";
const std::string NUM_TILES_IN_PRIMARY_DESCRIPTION =
"    --" + NUM_TILES_IN_PRIMARY_LONG + "=<N>\n"
"        Set the number of tiles in a primary set to N. This value should match\n"
"        the corresponding value in your project's `include/fieldmap.h'.\n"
"        Defaults to 512 (the pokeemerald default).\n";
constexpr int NUM_TILES_IN_PRIMARY_VAL = 1000;

const std::string NUM_TILES_TOTAL_LONG = "num-tiles-total";
const std::string NUM_TILES_TOTAL_DESCRIPTION =
"    --" + NUM_TILES_TOTAL_LONG + "=<N>\n"
"        Set the total number of tiles (primary + secondary) to N. This value\n"
"        should match the corresponding value in your project's\n"
"        `include/fieldmap.h'. Defaults to 1024 (the pokeemerald default).\n";
constexpr int NUM_TILES_TOTAL_VAL = 1001;

const std::string NUM_METATILES_IN_PRIMARY_LONG = "num-metatiles-primary";
const std::string NUM_METATILES_IN_PRIMARY_DESCRIPTION =
"    --" + NUM_METATILES_IN_PRIMARY_LONG + "=<N>\n"
"        Set the number of metatiles in a primary set to N. This value should\n"
"        match the corresponding value in your project's `include/fieldmap.h'.\n"
"        Defaults to 512 (the pokeemerald default).\n";
constexpr int NUM_METATILES_IN_PRIMARY_VAL = 1002;

const std::string NUM_METATILES_TOTAL_LONG = "num-metatiles-total";
const std::string NUM_METATILES_TOTAL_DESCRIPTION =
"    --" + NUM_METATILES_TOTAL_LONG + "=<N>\n"
"        Set the total number of metatiles (primary + secondary) to N. This\n"
"        value should match the corresponding value in your project's\n"
"        `include/fieldmap.h'. Defaults to 1024 (the pokeemerald default).\n";
constexpr int NUM_METATILES_TOTAL_VAL = 1003;

const std::string NUM_PALETTES_IN_PRIMARY_LONG = "num-pals-primary";
const std::string NUM_PALETTES_IN_PRIMARY_DESCRIPTION =
"    --" + NUM_PALETTES_IN_PRIMARY_LONG + "=<N>\n"
"        Set the number of palettes in a primary set to N. This value should\n"
"        match the corresponding value in your project's `include/fieldmap.h'.\n"
"        Defaults to 6 (the pokeemerald default).\n";
constexpr int NUM_PALETTES_IN_PRIMARY_VAL = 1004;

const std::string NUM_PALETTES_TOTAL_LONG = "num-pals-total";
const std::string NUM_PALETTES_TOTAL_DESCRIPTION =
"    --" + NUM_PALETTES_TOTAL_LONG + "=<N>\n"
"        Set the total number of palettes (primary + secondary) to N. This\n"
"        value should match the corresponding value in your project's\n"
"        `include/fieldmap.h'. Defaults to 13 (the pokeemerald default).\n";
constexpr int NUM_PALETTES_TOTAL_VAL = 1005;

const std::string TILES_PNG_PALETTE_MODE_LONG = "tiles-png-pal-mode";
const std::string TILES_PNG_PALETTE_MODE_DESCRIPTION =
"    --" + TILES_PNG_PALETTE_MODE_LONG + "=<MODE>\n"
"        Set the palette mode for the output `tiles.png'. Valid settings are\n"
"        `pal0', `true-color', or `greyscale'. These settings are for human\n"
"        visual purposes only and have no effect on the final in-game tiles.\n"
"        Default value is `greyscale'.\n";
constexpr int TILES_PNG_PALETTE_MODE_VAL = 1006;

const std::string SECONDARY_LONG = "secondary";
const std::string SECONDARY_DESCRIPTION =
"    --" + SECONDARY_LONG + "\n"
"        Specify that this tileset should be treated as a secondary tileset.\n"
"        Secondary tilesets are able to reuse tiles and palettes from their\n"
"        paired primary tileset. Note: the paired primary tileset must be\n"
"        a Porytiles-handled tileset.\n";
constexpr int SECONDARY_VAL = 1007;

const std::string PRESET_POKEEMERALD_LONG = "preset-emerald";
const std::string PRESET_POKEEMERALD_DESCRIPTION =
"    --" + PRESET_POKEEMERALD_LONG + "\n"
"        Set the fieldmap parameters to match those of `pokeemerald'. These\n"
"        can be found in `include/fieldmap.h'. This is the default preset.\n";
constexpr int PRESET_POKEEMERALD_VAL = 1008;

const std::string PRESET_POKEFIRERED_LONG = "preset-firered";
const std::string PRESET_POKEFIRERED_DESCRIPTION =
"    --" + PRESET_POKEFIRERED_LONG + "\n"
"        Set the fieldmap parameters to match those of `pokefirered'. These\n"
"        can be found in `include/fieldmap.h'.\n";
constexpr int PRESET_POKEFIRERED_VAL = 1009;

const std::string PRESET_POKERUBY_LONG = "preset-ruby";
const std::string PRESET_POKERUBY_DESCRIPTION =
"    --" + PRESET_POKERUBY_LONG + "\n"
"        Set the fieldmap parameters to match those of `pokeruby'. These\n"
"        can be found in `include/fieldmap.h'.\n";
constexpr int PRESET_POKERUBY_VAL = 1010;
// @formatter:on
// clang-format on

} // namespace porytiles

#endif // PORYTILES_CLI_OPTIONS_H