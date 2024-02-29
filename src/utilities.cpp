#include "utilities.h"

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include <algorithm>
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "logger.h"
#include "porytiles_context.h"
#include "types.h"

namespace porytiles {

std::vector<std::string> split(std::string input, const std::string &delimiter)
{
  std::vector<std::string> result;
  size_t pos;
  std::string token;
  while ((pos = input.find(delimiter)) != std::string::npos) {
    token = input.substr(0, pos);
    result.push_back(token);
    input.erase(0, pos + delimiter.length());
  }
  result.push_back(input);
  return result;
}

void trim(std::string &string)
{
  string.erase(string.begin(),
               std::find_if(string.begin(), string.end(), [](unsigned char ch) { return !std::isspace(ch); }));
  string.erase(std::find_if(string.rbegin(), string.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
               string.end());
}

std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName)
{
  return std::filesystem::temp_directory_path() / parentDir / fileName;
}

std::filesystem::path createTmpdir()
{
  int maxTries = 1000;
  auto tmpDir = std::filesystem::temp_directory_path();
  int i = 0;
  std::random_device randomDevice;
  std::mt19937 mersennePrng(randomDevice());
  std::uniform_int_distribution<uint64_t> uniformIntDistribution(0);
  std::filesystem::path path;
  while (true) {
    std::stringstream stringStream;
    stringStream << std::hex << uniformIntDistribution(mersennePrng);
    path = tmpDir / ("porytiles_" + stringStream.str());
    if (std::filesystem::create_directory(path)) {
      break;
    }
    if (i == maxTries) {
      internalerror("tmpfiles::createTmpdir getTmpdirPath took too many tries");
    }
    i++;
  }
  return path;
}

RGBA32 parseJascLine(PorytilesContext &ctx, DecompilerMode mode, const std::string &jascLine)
{
  // FIXME 1.0.0 : this function is a mess, mixing DecompilerMode and CompilerMode paradigms
  std::vector<std::string> colorComponents = split(jascLine, " ");
  if (colorComponents.size() != 3) {
    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 fmt::format("expected valid JASC line in pal file, saw {}", jascLine));
    }
    else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY || ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode,
                 fmt::format("expected valid JASC line in pal file, saw {}", jascLine));
    }
    else {
      internalerror_unknownSubcommand("utilities::parseJascLine");
    }
  }

  if (colorComponents[0].at(colorComponents[0].size() - 1) == '\r') {
    colorComponents[0].pop_back();
  }
  if (colorComponents[1].at(colorComponents[1].size() - 1) == '\r') {
    colorComponents[1].pop_back();
  }
  if (colorComponents[2].at(colorComponents[2].size() - 1) == '\r') {
    colorComponents[2].pop_back();
  }

  int red = parseInteger<int>(colorComponents[0].c_str());
  int green = parseInteger<int>(colorComponents[1].c_str());
  int blue = parseInteger<int>(colorComponents[2].c_str());

  if (red < 0 || red > 255) {
    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 "invalid red component: range must be 0 <= red <= 255");
    }
    else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY || ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode, "invalid red component: range must be 0 <= red <= 255");
    }
    else {
      internalerror_unknownSubcommand("utilities::parseJascLine");
    }
  }
  if (green < 0 || green > 255) {
    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 "invalid green component: range must be 0 <= green <= 255");
    }
    else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY || ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode, "invalid green component: range must be 0 <= green <= 255");
    }
    else {
      internalerror_unknownSubcommand("utilities::parseJascLine");
    }
  }
  if (blue < 0 || blue > 255) {
    if (ctx.subcommand == Subcommand::COMPILE_PRIMARY || ctx.subcommand == Subcommand::COMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode,
                 "invalid blue component: range must be 0 <= blue <= 255");
    }
    else if (ctx.subcommand == Subcommand::DECOMPILE_PRIMARY || ctx.subcommand == Subcommand::DECOMPILE_SECONDARY) {
      fatalerror(ctx.err, ctx.decompilerSrcPaths, mode, "invalid blue component: range must be 0 <= blue <= 255");
    }
    else {
      internalerror_unknownSubcommand("utilities::parseJascLine");
    }
  }

  return RGBA32{static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue),
                ALPHA_OPAQUE};
}

void doctestAssertFileBytesIdentical(std::filesystem::path expectedPath, std::filesystem::path actualPath)
{
  REQUIRE(std::filesystem::exists(expectedPath));
  REQUIRE(std::filesystem::exists(actualPath));
  std::FILE *expected;
  std::FILE *actual;
  expected = fopen(expectedPath.string().c_str(), "r");
  if (expected == NULL) {
    FAIL("std::FILE `expected' was null");
  }
  actual = fopen(actualPath.string().c_str(), "r");
  if (actual == NULL) {
    fclose(expected);
    FAIL("std::FILE `expected' was null");
  }
  fseek(expected, 0, SEEK_END);
  long expectedSize = ftell(expected);
  rewind(expected);
  fseek(actual, 0, SEEK_END);
  long actualSize = ftell(actual);
  rewind(actual);
  CHECK(expectedSize == actualSize);

  std::uint8_t expectedByte;
  std::uint8_t actualByte;
  std::size_t bytesRead;
  for (long i = 0; i < actualSize; i++) {
    bytesRead = fread(&expectedByte, 1, 1, expected);
    if (bytesRead != 1) {
      FAIL("did not read exactly 1 byte");
    }
    bytesRead = fread(&actualByte, 1, 1, actual);
    if (bytesRead != 1) {
      FAIL("did not read exactly 1 byte");
    }
    CHECK(expectedByte == actualByte);
  }
  fclose(expected);
  fclose(actual);
}

void doctestAssertFileLinesIdentical(std::filesystem::path expectedPath, std::filesystem::path actualPath)
{
  REQUIRE(std::filesystem::exists(expectedPath));
  REQUIRE(std::filesystem::exists(actualPath));

  std::string expectedLine;
  std::string actualLine;
  std::string dummy;
  std::size_t expectedLinesCount = 0;
  std::size_t actualLinesCount = 0;
  std::ifstream expected{expectedPath};
  std::ifstream actual{actualPath};

  while (std::getline(expected, dummy)) {
    expectedLinesCount++;
  }
  while (std::getline(actual, dummy)) {
    actualLinesCount++;
  }
  expected.close();
  expected.clear();
  actual.close();
  actual.clear();
  CHECK(expectedLinesCount == actualLinesCount);

  expected.open(expectedPath);
  actual.open(actualPath);
  while (std::getline(expected, expectedLine) && std::getline(actual, actualLine)) {
    CHECK(expectedLine == actualLine);
  }
}

} // namespace porytiles
