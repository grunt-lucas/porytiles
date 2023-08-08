#include "errors_warnings.h"

#include <cstddef>
#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <string>

#include "logger.h"
#include "ptexception.h"
#include "types.h"

namespace porytiles {

void internalerror_custom(std::string customMessage) { throw std::runtime_error(customMessage); }

void internalerror_numPalettesInPrimaryNeqPrimaryPalettesSize(std::string context, std::size_t configNumPalettesPrimary,
                                                              std::size_t primaryPalettesSize)
{
  internalerror_custom("config.numPalettesInPrimary did not match primary palette set size (" +
                       std::to_string(configNumPalettesPrimary) + " != " + std::to_string(primaryPalettesSize) + ")");
}

void internalerror_unknownCompilerMode(std::string context) { internalerror_custom(context + " unknown CompilerMode"); }

void error_layerHeightNotDivisibleBy16(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 height)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{} layer input PNG height '{}' was not divisible by 16", layerString(layer),
           fmt::styled(height, fmt::emphasis::bold));
  }
}

void error_layerWidthNeq128(ErrorsAndWarnings &err, TileLayer layer, png::uint_32 width)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("{} layer input PNG width '{}' was not 128", layerString(layer), fmt::styled(width, fmt::emphasis::bold));
  }
}

void error_layerHeightsMustEq(ErrorsAndWarnings &err, png::uint_32 bottom, png::uint_32 middle, png::uint_32 top)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("bottom, middle, top layer input PNG heights '{}, {}, {}' were not equivalent",
           fmt::styled(bottom, fmt::emphasis::bold), fmt::styled(middle, fmt::emphasis::bold),
           fmt::styled(top, fmt::emphasis::bold));
  }
}

void error_animFrameWasNotAPng(ErrorsAndWarnings &err, const std::string &animation, const std::string &file)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err("animation '{}' frame file '{}' was not a valid PNG file", fmt::styled(animation, fmt::emphasis::bold),
           fmt::styled(file, fmt::emphasis::bold));
  }
}

void error_tooManyUniqueColorsInTile(ErrorsAndWarnings &err, const RGBATile &tile, std::size_t row, std::size_t col)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err_rgbatile(tile, "too many unique colors at pixel col {}, row {}", col, row);
    pt_note_rgbatile(tile, "cannot have more than {} unique colors, including the transparency color", PAL_SIZE);
  }
}

void error_invalidAlphaValue(ErrorsAndWarnings &err, const RGBATile &tile, std::uint8_t alpha, std::size_t row,
                             std::size_t col)
{
  err.errCount++;
  if (err.printErrors) {
    pt_err_rgbatile(tile, "invalid alpha value '{}' at pixel col {}, row {}", fmt::styled(alpha, fmt::emphasis::bold),
                    col, row);
    pt_note_rgbatile(tile, "alpha value must be either {} for opaque or {} for transparent",
                     fmt::styled(ALPHA_OPAQUE, fmt::emphasis::bold),
                     fmt::styled(ALPHA_TRANSPARENT, fmt::emphasis::bold));
  }
}

void fatalerror_missingRequiredAnimFrameFile(ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                             const std::string &animation, std::size_t index)
{
  std::string file = std::to_string(index) + ".png";
  if (index < 10) {
    file = "0" + file;
  }
  if (err.printErrors) {
    pt_fatal_err("animation '{}' was missing expected frame file '{}'", fmt::styled(animation, fmt::emphasis::bold),
                 fmt::styled(file, fmt::emphasis::bold));
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode),
                            fmt::format("animation {} missing required anim frame file {}", animation, file));
}

void fatalerror_tooManyUniqueColorsTotal(ErrorsAndWarnings &err, const InputPaths &inputs, CompilerMode mode,
                                         std::size_t allowed, std::size_t found)
{
  if (err.printErrors) {
    pt_fatal_err("too many unique colors in {} tileset", compilerModeString(mode));
    pt_note("{} allowed based on fieldmap configuration, but found {}", allowed, found);
  }
  die_compilationTerminated(err, inputs.modeBasedInputPath(mode), fmt::format("too many unique colors total"));
}

void warn_colorPrecisionLoss(ErrorsAndWarnings &err)
{
  // TODO : better message
  if (err.colorPrecisionLossMode == WarningMode::ERR) {
    err.errCount++;
    pt_err("color precision loss");
  }
  else if (err.colorPrecisionLossMode == WarningMode::WARN) {
    err.warnCount++;
    pt_warn("color precision loss");
  }
}

void warn_transparentRepresentativeAnimTile(ErrorsAndWarnings &err)
{
  // TODO : better message
  if (err.transparentRepresentativeAnimTileMode == WarningMode::ERR) {
    err.errCount++;
    pt_err("transparent representative tile");
  }
  else if (err.transparentRepresentativeAnimTileMode == WarningMode::WARN) {
    err.warnCount++;
    pt_warn("transparent representative tile");
  }
}

void die_compilationTerminated(const ErrorsAndWarnings &err, std::string inputPath, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "terminating compilation of {}", inputPath);
  }
  throw PtException{errorMessage};
}

void die_errorCount(const ErrorsAndWarnings &err, std::string inputPath, std::string errorMessage)
{
  if (err.printErrors) {
    pt_println(stderr, "{} errors generated.", std::to_string(err.errCount));
    pt_println(stderr, "terminating compilation of {}", inputPath);
  }
  throw PtException{errorMessage};
}

} // namespace porytiles

// Test cases that deliberately check for end-to-end warning correctness go here

#include "compiler.h"
#include "importer.h"

TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly for regular tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.inputPaths.primaryInputPath = "/my/primary_tileset";
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInTile/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInTile/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInTile/top.png"));
  png::image<png::rgba_pixel> bottom{"res/tests/errors_and_warnings/tooManyUniqueColorsInTile/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/errors_and_warnings/tooManyUniqueColorsInTile/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/errors_and_warnings/tooManyUniqueColorsInTile/top.png"};
  porytiles::DecompiledTileset decompiled = porytiles::importLayeredTilesFromPngs(ctx, bottom, middle, top);

  CHECK_THROWS_WITH_AS(porytiles::compile(ctx, decompiled), "errors generated during tile normalization",
                       porytiles::PtException);
  CHECK(ctx.err.errCount == 6);
}

TEST_CASE("error_tooManyUniqueColorsInTile should trigger correctly for anim tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.inputPaths.primaryInputPath = "/my/primary_tileset";
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/top.png"));
  png::image<png::rgba_pixel> bottom{"res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/top.png"};
  porytiles::DecompiledTileset decompiled = porytiles::importLayeredTilesFromPngs(ctx, bottom, middle, top);

  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/anims/anim1"));

  porytiles::AnimationPng<png::rgba_pixel> anim1_00{
      png::image<png::rgba_pixel>{"res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/anims/anim1/00.png"},
      "anim1", "00.png"};
  porytiles::AnimationPng<png::rgba_pixel> anim1_01{
      png::image<png::rgba_pixel>{"res/tests/errors_and_warnings/tooManyUniqueColorsInAnimTile/anims/anim1/01.png"},
      "anim1", "01.png"};

  std::vector<porytiles::AnimationPng<png::rgba_pixel>> anim1{};
  anim1.push_back(anim1_00);
  anim1.push_back(anim1_01);

  std::vector<std::vector<porytiles::AnimationPng<png::rgba_pixel>>> anims{};
  anims.push_back(anim1);

  porytiles::importAnimTiles(anims, decompiled);

  CHECK_THROWS_WITH_AS(porytiles::compile(ctx, decompiled), "errors generated during animated tile normalization",
                       porytiles::PtException);
  CHECK(ctx.err.errCount == 4);
}

TEST_CASE("error_invalidAlphaValue should trigger correctly for regular tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 3;
  ctx.fieldmapConfig.numPalettesTotal = 6;
  ctx.inputPaths.primaryInputPath = "/my/primary_tileset";
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/invalidAlphaValue/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/invalidAlphaValue/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/invalidAlphaValue/top.png"));
  png::image<png::rgba_pixel> bottom{"res/tests/errors_and_warnings/invalidAlphaValue/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/errors_and_warnings/invalidAlphaValue/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/errors_and_warnings/invalidAlphaValue/top.png"};
  porytiles::DecompiledTileset decompiled = porytiles::importLayeredTilesFromPngs(ctx, bottom, middle, top);

  CHECK_THROWS_WITH_AS(porytiles::compile(ctx, decompiled), "errors generated during tile normalization",
                       porytiles::PtException);
  CHECK(ctx.err.errCount == 2);
}

TEST_CASE("error_tooManyUniqueColorsTotal should trigger correctly for regular tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "/my/primary_tileset";
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsTotal/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsTotal/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsTotal/top.png"));
  png::image<png::rgba_pixel> bottom{"res/tests/errors_and_warnings/tooManyUniqueColorsTotal/bottom.png"};
  png::image<png::rgba_pixel> middle{"res/tests/errors_and_warnings/tooManyUniqueColorsTotal/middle.png"};
  png::image<png::rgba_pixel> top{"res/tests/errors_and_warnings/tooManyUniqueColorsTotal/top.png"};
  porytiles::DecompiledTileset decompiled = porytiles::importLayeredTilesFromPngs(ctx, bottom, middle, top);

  CHECK_THROWS_WITH_AS(porytiles::compile(ctx, decompiled), "too many unique colors total", porytiles::PtException);
}

TEST_CASE("error_tooManyUniqueColorsTotal should trigger correctly for regular secondary tiles")
{
  porytiles::PtContext ctx{};
  ctx.fieldmapConfig.numPalettesInPrimary = 1;
  ctx.fieldmapConfig.numPalettesTotal = 2;
  ctx.inputPaths.primaryInputPath = "/my/primary_tileset";
  ctx.inputPaths.secondaryInputPath = "/my/secondary_tileset";
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/top.png"));
  png::image<png::rgba_pixel> bottomPrimary{"res/tests/simple_metatiles_1/bottom.png"};
  png::image<png::rgba_pixel> middlePrimary{"res/tests/simple_metatiles_1/middle.png"};
  png::image<png::rgba_pixel> topPrimary{"res/tests/simple_metatiles_1/top.png"};
  porytiles::DecompiledTileset decompiledPrimary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomPrimary, middlePrimary, topPrimary);

  auto primaryCompiled = porytiles::compile(ctx, decompiledPrimary);
  ctx.compilerConfig.mode = porytiles::CompilerMode::SECONDARY;
  ctx.compilerContext.pairedPrimaryTiles = std::move(primaryCompiled);

  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsTotal/bottom.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsTotal/middle.png"));
  REQUIRE(std::filesystem::exists("res/tests/errors_and_warnings/tooManyUniqueColorsTotal/top.png"));
  png::image<png::rgba_pixel> bottomSecondary{"res/tests/errors_and_warnings/tooManyUniqueColorsTotal/bottom.png"};
  png::image<png::rgba_pixel> middleSecondary{"res/tests/errors_and_warnings/tooManyUniqueColorsTotal/middle.png"};
  png::image<png::rgba_pixel> topSecondary{"res/tests/errors_and_warnings/tooManyUniqueColorsTotal/top.png"};
  porytiles::DecompiledTileset decompiledSecondary =
      porytiles::importLayeredTilesFromPngs(ctx, bottomSecondary, middleSecondary, topSecondary);

  CHECK_THROWS_WITH_AS(porytiles::compile(ctx, decompiledSecondary), "too many unique colors total",
                       porytiles::PtException);
}
