#ifndef PORYTILES_CLI_OPTIONS_H
#define PORYTILES_CLI_OPTIONS_H

#include <string>
#define FMT_HEADER_ONLY
#include <fmt/color.h>

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

/*
 * Global Options
 *
 * These options are displayed in the global help menu. They must be supplied before the
 * subcommand.
 */

const std::string HELP = "help";
const std::string HELP_SHORT = "h";
const std::string HELP_DESC = std::string{fmt::format(R"(
    -{}, --{}
        Print help message.
)",
HELP_SHORT, HELP
)}.substr(1);
constexpr int HELP_VAL = 0000;

const std::string VERBOSE = "verbose";
const std::string VERBOSE_SHORT = "v";
const std::string VERBOSE_DESC = std::string{fmt::format(R"(
    -{}, --{}
        Enable verbose logging to stderr.
)",
VERBOSE_SHORT, VERBOSE
)}.substr(1);
constexpr int VERBOSE_VAL = 0001;

const std::string VERSION = "version";
const std::string VERSION_SHORT = "V";
const std::string VERSION_DESC = std::string{fmt::format(R"(
    -{}, --{}
        Print version info.
)",
VERSION_SHORT, VERSION
)}.substr(1);
constexpr int VERSION_VAL = 0002;


/*
 * Driver Options
 *
 * These options control driver output. It's a bit of a catch-all category.
 */

const std::string OUTPUT = "output";
const std::string OUTPUT_SHORT = "o";
const std::string OUTPUT_DESC = std::string{fmt::format(R"(
        -{}, -{}=<OUTPUT-PATH>
            Output generated files to the directory specified by OUTPUT-PATH.
            If any element of OUTPUT-PATH does not exist, it will be created.
            Defaults to the current working directory (i.e. `.').
)",
OUTPUT_SHORT, OUTPUT
)}.substr(1);
constexpr int OUTPUT_VAL = 1000;

const std::string TILES_OUTPUT_PAL = "tiles-output-pal";
const std::string TILES_OUTPUT_PAL_DESC = std::string{fmt::format(R"(
        -{}=<MODE>
            Set the palette mode for the output `tiles.png'. Valid settings are
            `true-color' or `greyscale'. These settings are for human visual
            purposes only and have no effect on the final in-game tiles. Default
            value is `greyscale'.
)",
TILES_OUTPUT_PAL
)}.substr(1);
constexpr int TILES_OUTPUT_PAL_VAL = 1001;

const std::string DISABLE_METATILE_GENERATION = "disable-metatile-generation";
const std::string DISABLE_METATILE_GENERATION_DESC = std::string{fmt::format(R"(
        -{}
            Disable generation of `metatiles.bin'. Only enable this if you want
            to manage metatiles manually via Porymap.
)",
DISABLE_METATILE_GENERATION
)}.substr(1);
constexpr int DISABLE_METATILE_GENERATION_VAL = 1002;

const std::string DISABLE_ATTRIBUTE_GENERATION = "disable-attribute-generation";
const std::string DISABLE_ATTRIBUTE_GENERATION_DESC = std::string{fmt::format(R"(
        -{}
            Disable generation of `metatile_attributes.bin'. Only enable this if
            you want to manage metatile attributes manually via Porymap.
)",
DISABLE_ATTRIBUTE_GENERATION
)}.substr(1);
constexpr int DISABLE_ATTRIBUTE_GENERATION_VAL = 1003;


/*
 * Tileset Compilation and Decompilation Options
 *
 * These options change parameters specifically related to the compilation or decompilation process.
 */

const std::string TARGET_BASE_GAME = "target-base-game";
const std::string TARGET_BASE_GAME_DESC = std::string{fmt::format(R"(
        -{}=<TARGET>
            Set the (de)compilation target base game to TARGET. This option
            affects default values for the fieldmap parameters, as well as the
            metatile attribute format. Valid settings for TARGET are
            `pokeemerald', `pokefirered', or `pokeruby'. If this option is not
            specified, defaults to `pokeemerald'. See the wiki docs for more
            information.
)",
TARGET_BASE_GAME
)}.substr(1);
constexpr int TARGET_BASE_GAME_VAL = 2000;

const std::string DUAL_LAYER = "dual-layer";
const std::string DUAL_LAYER_DESC = std::string{fmt::format(R"(
        -{}
            Enable dual-layer compilation mode. The layer type will be inferred
            from your source layer PNGs, so compilation will error out if any
            metatiles contain content on all three layers. If this option is not
            supplied, Porytiles assumes you are compiling a triple-layer
            tileset.
)",
DUAL_LAYER
)}.substr(1);
constexpr int DUAL_LAYER_VAL = 2001;

const std::string TRANSPARENCY_COLOR = "transparency-color";
const std::string TRANSPARENCY_COLOR_DESC = std::string{fmt::format(R"(
        -{}=<R,G,B>
            Select RGB color <R,G,B> to represent transparency in your layer
            source PNGs. Defaults to `255,0,255'.
)",
TRANSPARENCY_COLOR
)}.substr(1);
constexpr int TRANSPARENCY_COLOR_VAL = 2002;

const std::string DEFAULT_BEHAVIOR = "default-behavior";
const std::string DEFAULT_BEHAVIOR_DESC = std::string{fmt::format(R"(
        -{}=<BEHAVIOR>
            Select the default behavior for metatiles that do not have an entry
            in the `attributes.csv' file. You may use either a raw integral
            value or a metatile behavior label defined in the provided behaviors
            header. If unspecified, defaults to `0'.
)",
DEFAULT_BEHAVIOR
)}.substr(1);
constexpr int DEFAULT_BEHAVIOR_VAL = 2004;

const std::string DEFAULT_ENCOUNTER_TYPE = "default-encounter-type";
const std::string DEFAULT_ENCOUNTER_TYPE_DESC = std::string{fmt::format(R"(
        -{}=<TYPE>
            Select the default encounter type for metatiles that do not have an
            entry in the `attributes.csv' file. You may use either a raw
            integral value or an EncounterType label defined in the
            `include/global.fieldmap.h' file. If unspecified, defaults to `0'.
)",
DEFAULT_ENCOUNTER_TYPE
)}.substr(1);
constexpr int DEFAULT_ENCOUNTER_TYPE_VAL = 2005;

const std::string DEFAULT_TERRAIN_TYPE = "default-terrain-type";
const std::string DEFAULT_TERRAIN_TYPE_DESC = std::string{fmt::format(R"(
        -{}=<TYPE>
            Select the default terrain type for metatiles that do not have an
            entry in the `attributes.csv' file. You may use either a raw
            integral value or an TerrainType label defined in the
            `include/global.fieldmap.h' file. If unspecified, defaults to `0'.
)",
DEFAULT_TERRAIN_TYPE
)}.substr(1);
constexpr int DEFAULT_TERRAIN_TYPE_VAL = 2006;

const std::string NORMALIZE_TRANSPARENCY = "normalize-transparency";
const std::string NORMALIZE_TRANSPARENCY_DESC = std::string{fmt::format(R"(
        -{}[=<R,G,B>]
            When emitting the decompiled tileset, replace all source transparent
            colors with the given RGB color. The default Porytiles behavior is
            equivalent to: `-normalize-transparency=255,0,255', which is best
            suited for decompiling the vanilla tilesets.
)",
NORMALIZE_TRANSPARENCY
)}.substr(1);
constexpr int NORMALIZE_TRANSPARENCY_VAL = 2007;

const std::string PRESERVE_TRANSPARENCY = "preserve-transparency";
const std::string PRESERVE_TRANSPARENCY_DESC = std::string{fmt::format(R"(
        -{}
            Preserve the original transparency colors in the source tileset.
            This option may be useful when decompiling custom tilesets for which
            you are already satisfied with the transparency configuration.
)",
PRESERVE_TRANSPARENCY
)}.substr(1);
constexpr int PRESERVE_TRANSPARENCY_VAL = 2008;


/*
 * Color Assignment Config Options
 *
 * These options adjust parameters for the palette assignment algorithm.
 */

const std::string ASSIGN_ALGO = "assign-algorithm";
const std::string ASSIGN_ALGO_DESC = std::string{fmt::format(R"(
        -{}=<ALGORITHM>
            Select the palette assignment algorithm. Valid options are `dfs' and
            `bfs'. Default is `dfs'.
)",
ASSIGN_ALGO
)}.substr(1);
constexpr int ASSIGN_ALGO_VAL = 3000;

const std::string EXPLORE_CUTOFF = "explore-cutoff";
const std::string EXPLORE_CUTOFF_DESC = std::string{fmt::format(R"(
        -{}=<FACTOR>
            Select the cutoff for palette assignment tree node exploration.
            Defaults to 2000000, which should be sufficient for most cases.
            Increase the number to let the algorithm run for longer before
            failing out.
)",
EXPLORE_CUTOFF
)}.substr(1);
constexpr int EXPLORE_CUTOFF_VAL = 3001;

const std::string BEST_BRANCHES = "best-branches";
const std::string BEST_BRANCHES_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Use only the N most promising branches at each node in the
            assignment tree search. Specify `smart' instead of an integer to use
            a computed `smart' cutoff at each node instead of a constant integer
            cutoff. Default is to use all available branches.
)",
BEST_BRANCHES
)}.substr(1);
constexpr int BEST_BRANCHES_VAL = 3002;
const std::string SMART_PRUNE = "smart";

const std::string DISABLE_ASSIGN_CACHING = "disable-assign-caching";
const std::string DISABLE_ASSIGN_CACHING_DESC = std::string{fmt::format(R"(
        -{}
            Do not write palette assignment search parameters to `assign.cache'
            after a successful compilation.
)",
DISABLE_ASSIGN_CACHING
)}.substr(1);
constexpr int DISABLE_ASSIGN_CACHING_VAL = 3003;

const std::string FORCE_ASSIGN_PARAM_MATRIX = "force-assign-param-matrix";
const std::string FORCE_ASSIGN_PARAM_MATRIX_DESC = std::string{fmt::format(R"(
        -{}
            Force the full palette assignment parameter search matrix to run. If
            `assign.cache' exists in either source folder, its contents will be
            ignored.
)",
FORCE_ASSIGN_PARAM_MATRIX
)}.substr(1);
constexpr int FORCE_ASSIGN_PARAM_MATRIX_VAL = 3004;

const std::string PRIMARY_ASSIGN_ALGO = "primary-assign-algorithm";
const std::string PRIMARY_ASSIGN_ALGO_DESC = std::string{fmt::format(R"(
        -{}=<FACTOR>
            Same as `-assign-algorithm', but for the paired primary set. Only to
            be used when compiling in secondary mode via `compile-secondary'.
)",
PRIMARY_ASSIGN_ALGO
)}.substr(1);
constexpr int PRIMARY_ASSIGN_ALGO_VAL = 3005;

const std::string PRIMARY_EXPLORE_CUTOFF = "primary-explore-cutoff";
const std::string PRIMARY_EXPLORE_CUTOFF_DESC = std::string{fmt::format(R"(
        -{}=<FACTOR>
            Same as `-assign-explore-cutoff', but for the paired primary set.
            Only to be used when compiling in secondary mode via
            `compile-secondary'.
)",
PRIMARY_EXPLORE_CUTOFF
)}.substr(1);
constexpr int PRIMARY_EXPLORE_CUTOFF_VAL = 3006;

const std::string PRIMARY_BEST_BRANCHES = "primary-best-branches";
const std::string PRIMARY_BEST_BRANCHES_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Same as `-best-branches', but for the paired primary set. Only to be
            used when compiling in secondary mode via `compile-secondary'.
)",
PRIMARY_BEST_BRANCHES
)}.substr(1);
constexpr int PRIMARY_BEST_BRANCHES_VAL = 3007;


/*
 * Fieldmap Override Options
 *
 * These options override the `fieldmap.h' parameters that are automatically set by the target base
 * game.
 */

const std::string TILES_PRIMARY_OVERRIDE = "tiles-primary-override";
const std::string TILES_PRIMARY_OVERRIDE_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Override the target base game's default number of primary set tiles.
            The value specified here should match the corresponding value in
            your project's `fieldmap.h'. Defaults to 512 (inherited from
            `pokeemerald' default target base game).
)",
TILES_PRIMARY_OVERRIDE
)}.substr(1);
constexpr int TILES_PRIMARY_OVERRIDE_VAL = 4000;

const std::string TILES_TOTAL_OVERRIDE = "tiles-total-override";
const std::string TILES_TOTAL_OVERRIDE_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Override the target base game's default number of total tiles. The
            value specified here should match the corresponding value in your
            project's `fieldmap.h'. Defaults to 1024 (inherited from
            `pokeemerald' default target base game).
)",
TILES_TOTAL_OVERRIDE
)}.substr(1);
constexpr int TILES_TOTAL_OVERRIDE_VAL = 4001;

const std::string METATILES_PRIMARY_OVERRIDE = "metatiles-primary-override";
const std::string METATILES_PRIMARY_OVERRIDE_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Override the target base game's default number of primary set
            metatiles. The value specified here should match the corresponding
            value in your project's `fieldmap.h'. Defaults to 512 (inherited
            from `pokeemerald' default target base game).
)",
METATILES_PRIMARY_OVERRIDE
)}.substr(1);
constexpr int METATILES_PRIMARY_OVERRIDE_VAL = 4002;

const std::string METATILES_TOTAL_OVERRIDE = "metatiles-total-override";
const std::string METATILES_TOTAL_OVERRIDE_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Override the target base game's default number of total metatiles.
            The value specified here should match the corresponding value in
            your project's `fieldmap.h'. Defaults to 1024 (inherited from
            `pokeemerald' default target base game).
)",
METATILES_TOTAL_OVERRIDE
)}.substr(1);
constexpr int METATILES_TOTAL_OVERRIDE_VAL = 4003;

const std::string PALS_PRIMARY_OVERRIDE = "pals-primary-override";
const std::string PALS_PRIMARY_OVERRIDE_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Override the target base game's default number of primary set
            palettes. The value specified here should match the corresponding
            value in your project's `fieldmap.h'. Defaults to 6 (inherited from
            `pokeemerald' default target base game).
)",
PALS_PRIMARY_OVERRIDE
)}.substr(1);
constexpr int PALS_PRIMARY_OVERRIDE_VAL = 4004;

const std::string PALS_TOTAL_OVERRIDE = "pals-total-override";
const std::string PALS_TOTAL_OVERRIDE_DESC = std::string{fmt::format(R"(
        -{}=<N>
            Override the target base game's default number of total palettes.
            The value specified here should match the corresponding value in
            your project's `fieldmap.h'. Defaults to 13 (inherited from
            `pokeemerald' default target base game).
)",
PALS_TOTAL_OVERRIDE
)}.substr(1);
constexpr int PALS_TOTAL_OVERRIDE_VAL = 4005;


/*
 * Warning Options
 *
 * These options enable/disable various warnings.
 */

const std::string WALL = "Wall";
const std::string WALL_DESC = std::string{fmt::format(R"(
        -{}
            Enable all warnings.
)",
WALL
)}.substr(1);
constexpr int WALL_VAL = 5000;

const std::string W_GENERAL = "W";
const std::string W_GENERAL_DESC = std::string{fmt::format(R"(
        -{}<WARNING>, -{}no-<WARNING>
            Explicitly enable warning WARNING, or explicitly disable it if the
            `no' form of the option is specified. If WARNING is already off, the
            `no' form will no-op. If more than one specifier for the same
            warning appears on the same command line, the right-most specifier
            will take precedence.
)",
W_GENERAL, W_GENERAL
)}.substr(1);

const std::string WNONE = "Wnone";
const std::string WNONE_SHORT = "w";
const std::string WNONE_DESC = std::string{fmt::format(R"(
        -{}, -{}
            Disable all warnings.
)",
WNONE_SHORT, WNONE
)}.substr(1);
constexpr int WNONE_VAL = 5001;

const std::string WNO_ERROR = "Wno-error";
constexpr int WNO_ERROR_VAL = 5002;

const std::string WERROR = "Werror";
const std::string WERROR_DESC = std::string{fmt::format(R"(
        -{}[=<WARNING>], -{}=<WARNING>
            Force all enabled warnings to generate errors, or optionally force
            WARNING to enable as an error. If the `no' form of the option is
            specified, downgrade WARNING from an error to the highest previously
            seen level. If WARNING is already off, the `no' form will no-op. If
            more than one specifier for the same warning appears on the same
            command line, the right-most specifier will take precedence.
)",
WERROR, WNO_ERROR
)}.substr(1);
constexpr int WERROR_VAL = 5003;

// Compilation warnings
const std::string WCOLOR_PRECISION_LOSS = W_GENERAL + WARN_COLOR_PRECISION_LOSS;
const std::string WNO_COLOR_PRECISION_LOSS = W_GENERAL + "no-" + WARN_COLOR_PRECISION_LOSS;
constexpr int WCOLOR_PRECISION_LOSS_VAL = 50000;
constexpr int WNO_COLOR_PRECISION_LOSS_VAL = 60000;

const std::string WKEY_FRAME_DID_NOT_APPEAR = W_GENERAL + WARN_KEY_FRAME_NO_MATCHING_TILE;
const std::string WNO_KEY_FRAME_DID_NOT_APPEAR = W_GENERAL + "no-" + WARN_KEY_FRAME_NO_MATCHING_TILE;
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

const std::string WASSIGN_CONFIG_OVERRIDE = W_GENERAL + WARN_ASSIGN_CACHE_OVERRIDE;
const std::string WNO_ASSIGN_CONFIG_OVERRIDE = W_GENERAL + "no-" + WARN_ASSIGN_CACHE_OVERRIDE;
constexpr int WASSIGN_CONFIG_OVERRIDE_VAL = 50080;
constexpr int WNO_ASSIGN_CONFIG_OVERRIDE_VAL = 60080;

const std::string WINVALID_ASSIGN_CONFIG_CACHE = W_GENERAL + WARN_INVALID_ASSIGN_CACHE;
const std::string WNO_INVALID_ASSIGN_CONFIG_CACHE = W_GENERAL + "no-" + WARN_INVALID_ASSIGN_CACHE;
constexpr int WINVALID_ASSIGN_CONFIG_CACHE_VAL = 50090;
constexpr int WNO_INVALID_ASSIGN_CONFIG_CACHE_VAL = 60090;

const std::string WMISSING_ASSIGN_CONFIG = W_GENERAL + WARN_MISSING_ASSIGN_CACHE;
const std::string WNO_MISSING_ASSIGN_CONFIG = W_GENERAL + "no-" + WARN_MISSING_ASSIGN_CACHE;
constexpr int WMISSING_ASSIGN_CONFIG_VAL = 50100;
constexpr int WNO_MISSING_ASSIGN_CONFIG_VAL = 60100;

// Decompilation warnings
const std::string WTILE_INDEX_OUT_OF_RANGE = W_GENERAL + WARN_TILE_INDEX_OUT_OF_RANGE;
const std::string WNO_TILE_INDEX_OUT_OF_RANGE = W_GENERAL + "no-" + WARN_TILE_INDEX_OUT_OF_RANGE;
constexpr int WTILE_INDEX_OUT_OF_RANGE_VAL = 70110;
constexpr int WNO_TILE_INDEX_OUT_OF_RANGE_VAL = 80110;

const std::string WPALETTE_INDEX_OUT_OF_RANGE = W_GENERAL + WARN_PALETTE_INDEX_OUT_OF_RANGE;
const std::string WNO_PALETTE_INDEX_OUT_OF_RANGE = W_GENERAL + "no-" + WARN_PALETTE_INDEX_OUT_OF_RANGE;
constexpr int WPALETTE_INDEX_OUT_OF_RANGE_VAL = 70120;
constexpr int WNO_PALETTE_INDEX_OUT_OF_RANGE_VAL = 80120;

// @formatter:on
// clang-format on

} // namespace porytiles

#endif // PORYTILES_CLI_OPTIONS_H