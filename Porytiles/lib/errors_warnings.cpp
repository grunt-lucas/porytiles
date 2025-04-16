#include "errors_warnings.h"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include <cstddef>
#include <fmt/color.h>
#include <png.hpp>
#include <stdexcept>
#include <string>
#include <tuple>

#include "compiler.h"
#include "driver.h"
#include "importer.h"
#include "logger.h"
#include "porytiles_exception.h"
#include "types.h"
#include "utilities.h"

namespace porytiles {

void internalerror(const std::string &message) {
    throw std::runtime_error(message);
}

void internalerror_unknownCompilerMode(const std::string &context) {
    internalerror(context + " unknown CompilerMode");
}

void internalerror_unknownDecompilerMode(const std::string &context) {
    internalerror(context + " unknown DecompilerMode");
}

void internalerror_unknownSubcommand(const std::string &context) {
    internalerror(context + " unknown Subcommand");
}

void fatalerror_assignCacheSyntaxError(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                       const CompilerMode &mode, std::string line, std::size_t lineNumber,
                                       std::string path) {
    if (err.printErrors) {
        pt_fatal_err("{}: invalid syntax `{}' at line {}", path, fmt::styled(line, fmt::emphasis::bold), lineNumber);
        pt_note("`assign.cache' expected line syntax is: {}", fmt::styled("key=value", fmt::emphasis::bold));
        pt_println(stderr, "");
    }
    die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("invalid assign syntax {}", line));
}

void fatalerror_assignCacheInvalidKey(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                      const CompilerMode &mode, std::string key, std::size_t lineNumber,
                                      std::string path) {
    if (err.printErrors) {
        pt_fatal_err("{}: invalid key `{}' at line {}", path, fmt::styled(key, fmt::emphasis::bold), lineNumber);
        pt_note("`assign.cache' expects keys to match the color assignment config options");
        pt_println(stderr, "");
    }
    die_compilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("invalid assign key {}", key));
}

void fatalerror_assignCacheInvalidValue(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                        const CompilerMode &mode, std::string key, std::string value,
                                        std::size_t lineNumber, std::string path) {
    if (err.printErrors) {
        pt_fatal_err("{}: invalid value `{}' for key `{}' at line {}", path, fmt::styled(value, fmt::emphasis::bold),
                     fmt::styled(key, fmt::emphasis::bold), lineNumber);
        pt_println(stderr, "");
    }
    die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                              fmt::format("invalid assign value {} for key {}", value, key));
}

void fatalerror_paletteAssignParamSearchMatrixFailed(const ErrorsAndWarnings &err, const CompilerSourcePaths &srcs,
                                                     const CompilerMode &mode) {
    if (err.printErrors) {
        pt_fatal_err("palette assignment parameter search matrix failed to find any suitable parameters");
        pt_note("please see the following wiki page for help with working through this error:");
        // TODO : fill in wiki page link
        pt_println(stderr, "      https://wiki-page-link-goes-here.com");
    }
    die_compilationTerminated(err, srcs.modeBasedSrcPath(mode),
                              fmt::format("palette assign param search matrix failed"));
}

void fatalerror_noImpliedLayerType(const ErrorsAndWarnings &err, const DecompilerSourcePaths &srcs,
                                   DecompilerMode mode) {
    if (err.printErrors) {
        pt_fatal_err("no layer type was implied by the supplied metatiles and attributes");
        pt_note("either you forgot to supply the correct `-target-base-game' option, or a file is corrupted");
        pt_println(stderr, "");
    }
    die_decompilationTerminated(err, srcs.modeBasedSrcPath(mode), fmt::format("no implied layer type"));
}

void die(const ErrorsAndWarnings &err, std::string errorMessage) {
    if (err.printErrors) {
        pt_println(stderr, "{}", errorMessage);
    }
    throw PorytilesException{errorMessage};
}

void die_compilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage) {
    if (err.printErrors) {
        pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
    }
    throw PorytilesException{errorMessage};
}

void die_compilationTerminatedFailHard(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage) {
    if (err.printErrors) {
        pt_println(stderr, "terminating compilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
    }
    std::exit(1);
}

void die_decompilationTerminated(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage) {
    if (err.printErrors) {
        pt_println(stderr, "terminating decompilation of {}", fmt::styled(srcPath, fmt::emphasis::bold));
    }
    throw PorytilesException{errorMessage};
}

void die_errorCount(const ErrorsAndWarnings &err, std::string srcPath, std::string errorMessage) {
    if (err.printErrors) {
        // TODO : display warn and err count here once all errors are migrated
        pt_println(stderr, "terminating compilation of {}", styled(srcPath, fmt::emphasis::bold));
    }
    throw PorytilesException{errorMessage};
}

} // namespace porytiles

#ifndef DOCTEST_CONFIG_DISABLE
TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly") {
    SUBCASE("it should work for regular tiles") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 3;
        ctx.fieldmapConfig.numPalettesTotal = 6;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/error_tooManyUniqueColorsInTile_regular";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 6);
    }

    SUBCASE("it should work for anim tiles") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 3;
        ctx.fieldmapConfig.numPalettesTotal = 6;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/error_tooManyUniqueColorsInTile_anim";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 4);
    }
}

TEST_CASE("error_invalidAlphaValue should trigger correctly for regular tiles") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 3;
    ctx.fieldmapConfig.numPalettesTotal = 6;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_invalidAlphaValue";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                         porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
}

TEST_CASE("error_animFrameWasNotAPng should trigger correctly when an anim frame is missing") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/error_animFrameWasNotAPng";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "found anim frame that was not a png", porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
}

TEST_CASE("error_allThreeLayersHadNonTransparentContent should trigger correctly when a dual-layer inference fails") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.compilerConfig.tripleLayer = false;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/error_allThreeLayersHadNonTransparentContent";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                         porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
}

TEST_CASE("error_invalidCsvRowFormat should trigger correctly when a row format is invalid") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Emerald row format, missing field") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/incorrect_row_format_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }
    SUBCASE("Firered row format, missing field") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/incorrect_row_format_2.csv"),
                             "errors generated during attributes CSV parsing", porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
    }
}

TEST_CASE("error_unknownMetatileBehavior should trigger correctly when a row has an unrecognized behavior") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Emerald row format, missing metatile behavior") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/unknown_behavior_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
    }
}

TEST_CASE("error_duplicateAttribute should trigger correctly when two rows specify the same metatile id") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Duplicate metatile definition test 1") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/duplicate_definition_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
    }
}

TEST_CASE("error_invalidTerrainType should trigger correctly when a row specifies an invalid TerrainType") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Invalid TerrainType test 1") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/invalid_terrain_type_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }
}

TEST_CASE("error_invalidEncounterType should trigger correctly when a row specifies an invalid EncounterType") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Invalid EncounterType test 1") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/invalid_encounter_type_1.csv"),
                             "errors generated during attributes CSV parsing", porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular primary tiles") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "too many unique colors total", porytiles::PorytilesException);
}

TEST_CASE("fatalerror_tooManyUniqueColorsTotal should trigger correctly for regular secondary tiles") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/simple_metatiles_1";
    ctx.compilerSrcPaths.secondarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_tooManyUniqueColorsTotal";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "too many unique colors total", porytiles::PorytilesException);
}

TEST_CASE("fatalerror_missingRequiredAnimFrameFile should trigger correctly in both cases:") {
    SUBCASE("when an anim frame is missing") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 1;
        ctx.fieldmapConfig.numPalettesTotal = 2;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_skipCase";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.err.printErrors = false;
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 01.png",
                             porytiles::PorytilesException);
    }

    SUBCASE("when there are no regular frames supplied") {
        porytiles::PorytilesContext ctx{};
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 1;
        ctx.fieldmapConfig.numPalettesTotal = 2;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/fatalerror_missingRequiredAnimFrameFile_keyOnlyCase";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.err.printErrors = false;
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing required anim frame file 00.png",
                             porytiles::PorytilesException);
    }
}

TEST_CASE("fatalerror_missingKeyFrameFile should trigger correctly when there is no key frame supplied") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_missingKeyFrameFile";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 missing key frame file",
                         porytiles::PorytilesException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame width "
          "is mismatched") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_widthCase";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "anim anim1 frame 01.png dimension width mismatch",
                         porytiles::PorytilesException);
}

TEST_CASE("fatalerror_animFrameDimensionsDoNotMatchOtherFrames should trigger correctly when an anim frame height "
          "is mismatched") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_animFrameDimensionsDoNotMatchOtherFrames_heightCase";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "anim anim1 frame 02.png dimension height mismatch",
                         porytiles::PorytilesException);
}

TEST_CASE("fatalerror_transparentKeyFrameTile should trigger when an anim has a transparent tile") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_transparentKeyFrameTile";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 had a transparent key frame tile",
                         porytiles::PorytilesException);
}

TEST_CASE(
    "fatalerror_duplicateKeyFrameTile should trigger when two different animations have a duplicate key frame tile") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/fatalerror_duplicateKeyFrameTile";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim2 had a duplicate key frame tile",
                         porytiles::PorytilesException);
}

TEST_CASE("fatalerror_keyFramePresentInPairedPrimary should trigger when an animation key frame tile is present in the "
          "paired primary tileset") {
    porytiles::PorytilesContext ctx{};
    ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 4;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/primary";
    ctx.compilerSrcPaths.secondarySourcePath =
        "Resources/Tests/errors_and_warnings/fatalerror_keyFramePresentInPairedPrimary/secondary";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.err.printErrors = false;
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "animation anim1 key frame tile present in paired primary",
                         porytiles::PorytilesException);
}

TEST_CASE("fatalerror_invalidAttributesCsvHeader should trigger when an attributes file is missing a header") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Completely missing header") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/missing_header_1.csv"),
                             "Resources/Tests/csv/missing_header_1.csv: incorrect header row format",
                             porytiles::PorytilesException);
    }

    SUBCASE("Header missing id field") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/missing_header_2.csv"),
                             "Resources/Tests/csv/missing_header_2.csv: incorrect header row format",
                             porytiles::PorytilesException);
    }

    SUBCASE("Header missing behavior field") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/missing_header_3.csv"),
                             "Resources/Tests/csv/missing_header_3.csv: incorrect header row format",
                             porytiles::PorytilesException);
    }

    SUBCASE("Header has terrainType but missing encounterType") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/missing_header_4.csv"),
                             "Resources/Tests/csv/missing_header_4.csv: incorrect header row format",
                             porytiles::PorytilesException);
    }
}

TEST_CASE(
    "fatalerror_invalidIdInCsv should trigger when the id column in attribute csv contains a non-integral value") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

    SUBCASE("Invalid integer format 1") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/invalid_id_column_1.csv"),
                             "Resources/Tests/csv/invalid_id_column_1.csv: invalid id foo",
                             porytiles::PorytilesException);
    }

    SUBCASE("Invalid integer format 2") {
        CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                                "Resources/Tests/csv/invalid_id_column_2.csv"),
                             "Resources/Tests/csv/invalid_id_column_2.csv: invalid id 6bar",
                             porytiles::PorytilesException);
    }
}

TEST_CASE("fatalerror_invalidBehaviorValue should trigger when the metatile behavior header has a non-integral "
          "behavior value") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;

    SUBCASE("Invalid integer format 1") {
        std::ifstream behaviorFile{"Resources/Tests/metatile_behaviors_invalid_1.h"};
        CHECK_THROWS_WITH_AS(
            porytiles::importMetatileBehaviorHeader(ctx, porytiles::CompilerMode::PRIMARY, behaviorFile),
            "invalid behavior value foo", porytiles::PorytilesException);
        behaviorFile.close();
    }

    SUBCASE("Invalid integer format 2") {
        std::ifstream behaviorFile{"Resources/Tests/metatile_behaviors_invalid_2.h"};
        CHECK_THROWS_WITH_AS(
            porytiles::importMetatileBehaviorHeader(ctx, porytiles::CompilerMode::PRIMARY, behaviorFile),
            "invalid behavior value 6bar", porytiles::PorytilesException);
        behaviorFile.close();
    }
}

TEST_CASE("warn_colorPrecisionLoss should trigger correctly when a color collapses") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->enable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::DiagLevel::Error);
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_colorPrecisionLoss";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                         porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_COLOR_PRECISION_LOSS) == 3);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 3);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Note) == 3);
}

TEST_CASE("warn_keyFrameNoMatchingTile should trigger correctly when a key frame tile is not used") {
    SUBCASE("it should trigger correctly for a primary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_KEY_FRAME_NO_MATCHING_TILE, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during primary tile assignment",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_KEY_FRAME_NO_MATCHING_TILE) == 2);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
    }

    SUBCASE("it should trigger correctly for a secondary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_KEY_FRAME_NO_MATCHING_TILE, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/primary_correct";
        ctx.compilerSrcPaths.secondarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_keyFrameTileDidNotAppearInAssignment/secondary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during secondary tile assignment",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_KEY_FRAME_NO_MATCHING_TILE) == 2);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
    }
}

TEST_CASE("warn_tooManyAttributesForTargetGame should correctly warn") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->enable_at_level(porytiles::W_ATTRIBUTE_FORMAT_MISMATCH, porytiles::DiagLevel::Error);
    ctx.targetBaseGame = porytiles::TargetBaseGame::EMERALD;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/correct_2.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_ATTRIBUTE_FORMAT_MISMATCH) == 1);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
}

TEST_CASE("warn_tooFewAttributesForTargetGame should correctly warn") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->enable_at_level(porytiles::W_ATTRIBUTE_FORMAT_MISMATCH, porytiles::DiagLevel::Error);
    ctx.targetBaseGame = porytiles::TargetBaseGame::FIRERED;

    std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};
    CHECK_THROWS_WITH_AS(porytiles::importAttributesFromCsv(ctx, porytiles::CompilerMode::PRIMARY, behaviorMap,
                                                            "Resources/Tests/csv/correct_1.csv"),
                         "errors generated during attributes CSV parsing", porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_ATTRIBUTE_FORMAT_MISMATCH) == 1);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
}

TEST_CASE("warn_attributesFileNotFound should correctly warn") {
    SUBCASE("it should trigger correctly for a primary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_MISSING_ATTRIBUTES_CSV, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_attributesFileNotFound/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during primary attributes import",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_MISSING_ATTRIBUTES_CSV) == 1);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }

    SUBCASE("it should trigger correctly for a secondary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_MISSING_ATTRIBUTES_CSV, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_attributesFileNotFound/primary_correct";
        ctx.compilerSrcPaths.secondarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_attributesFileNotFound/secondary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.err.printErrors = false;
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during secondary attributes import",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_MISSING_ATTRIBUTES_CSV) == 1);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }
}

TEST_CASE("warn_unusedAttribute should correctly warn") {
    SUBCASE("it should trigger correctly for a primary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_UNUSED_ATTRIBUTE, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedAttribute/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_UNUSED_ATTRIBUTE) == 1);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }

    SUBCASE("it should trigger correctly for a secondary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_UNUSED_ATTRIBUTE, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_SECONDARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_unusedAttribute/primary_correct";
        ctx.compilerSrcPaths.secondarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedAttribute/secondary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_UNUSED_ATTRIBUTE) == 1);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }

    SUBCASE("it should trigger correctly for a dual layer primary set") {
        porytiles::PorytilesContext ctx{};
        ctx.err.printErrors = false;
        auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
        ctx.set_diag_engine(std::move(engine));
        ctx.diag->enable_at_level(porytiles::W_UNUSED_ATTRIBUTE, porytiles::DiagLevel::Error);
        ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
        ctx.fieldmapConfig.numPalettesInPrimary = 2;
        ctx.fieldmapConfig.numPalettesTotal = 4;
        ctx.compilerConfig.tripleLayer = false;
        ctx.compilerSrcPaths.primarySourcePath =
            "Resources/Tests/errors_and_warnings/warn_unusedAttribute/dual/primary";
        ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
        ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
        ctx.compilerConfig.cacheAssign = false;

        CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during layered tile import",
                             porytiles::PorytilesException);
        CHECK(ctx.diag->in_flight_count_for(porytiles::W_UNUSED_ATTRIBUTE) == 1);
        CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 1);
    }
}

TEST_CASE("warn_nonTransparentRgbaCollapsedToTransparentBgr should trigger correctly when a color collapses") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->enable_at_level(porytiles::W_TRANSPARENCY_COLLAPSE, porytiles::DiagLevel::Error);
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 1;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_nonTransparentRgbaCollapsedToTransparentBgr";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                         porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_TRANSPARENCY_COLLAPSE) == 2);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 2);
}

TEST_CASE("warn_unusedManualPalColor should trigger correctly") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->enable_at_level(porytiles::W_UNUSED_MANUAL_PAL_COLOR, porytiles::DiagLevel::Error);
    ctx.subcommand = porytiles::Subcommand::COMPILE_PRIMARY;
    ctx.fieldmapConfig.numPalettesInPrimary = 2;
    ctx.fieldmapConfig.numPalettesTotal = 2;
    ctx.compilerSrcPaths.primarySourcePath = "Resources/Tests/errors_and_warnings/warn_unusedManualPalColor";
    ctx.compilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";
    ctx.compilerConfig.primaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.secondaryAssignAlgorithm = porytiles::AssignAlgorithm::DFS;
    ctx.compilerConfig.cacheAssign = false;

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors generated during tile normalization",
                         porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_UNUSED_MANUAL_PAL_COLOR) == 4);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 4);
}

TEST_CASE("warn_indexOutOfRangeWarnings should trigger correctly") {
    porytiles::PorytilesContext ctx{};
    ctx.err.printErrors = false;
    auto engine = std::make_unique<porytiles::DiagEngine>(std::make_unique<porytiles::IgnoreConsumer>());
    ctx.set_diag_engine(std::move(engine));
    ctx.diag->enable_at_level(porytiles::W_TILE_INDEX_OUT_OF_RANGE, porytiles::DiagLevel::Error);
    ctx.diag->enable_at_level(porytiles::W_PALETTE_INDEX_OUT_OF_RANGE, porytiles::DiagLevel::Error);
    ctx.subcommand = porytiles::Subcommand::DECOMPILE_SECONDARY;
    ctx.decompilerSrcPaths.primarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_indexOutOfRangeWarnings/general";
    ctx.decompilerSrcPaths.secondarySourcePath =
        "Resources/Tests/errors_and_warnings/warn_indexOutOfRangeWarnings/petalburg";
    ctx.decompilerSrcPaths.metatileBehaviors = "Resources/Tests/metatile_behaviors.h";

    CHECK_THROWS_WITH_AS(porytiles::drive(ctx), "errors encountered while decompiling tileset",
                         porytiles::PorytilesException);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_TILE_INDEX_OUT_OF_RANGE) == 8);
    CHECK(ctx.diag->in_flight_count_for(porytiles::W_PALETTE_INDEX_OUT_OF_RANGE) == 8);
    CHECK(ctx.diag->in_flight_count_for_level(porytiles::DiagLevel::Error) == 16);
}

#endif // DOCTEST_CONFIG_DISABLE
