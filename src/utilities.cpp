#include "utilities.h"

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include <csv.h>
#include <doctest.h>
#include <filesystem>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "logger.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

std::unordered_map<std::size_t, Attributes>
getEmeraldRubyAttributesFromCsv(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                                std::string filePath)
{
  std::unordered_map<std::size_t, Attributes> attributeMap{};
  io::CSVReader<2> in{filePath};
  try {
    in.read_header(io::ignore_no_column, "id", "behavior");
  }
  catch (const std::exception &e) {
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, filePath, "id,behavior");
  }
  std::size_t id;
  std::string behavior;

  // processedUpToLine starts at 1 since we processed the header already, which was on line 1
  std::size_t processedUpToLine = 1;
  while (true) {
    bool readRow = false;
    try {
      readRow = in.read_row(id, behavior);
      processedUpToLine++;
    }
    catch (const std::exception &e) {
      // add 1 to processedUpToLine display, since we threw before we could increment the counter
      // TDOD : make this a regular error
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: invalid row on line {}", filePath, processedUpToLine + 1));
    }
    if (!readRow) {
      break;
    }

    Attributes attribute{};
    attribute.baseGame = ctx.targetBaseGame;
    if (behaviorMap.contains(behavior)) {
      attribute.metatileBehavior = behaviorMap.at(behavior);
    }
    else {
      // TDOD : make this a regular error
      fatalerror(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                 fmt::format("{}: unknown metatile behavior '{}' at line {}", filePath, behavior, processedUpToLine));
    }
    auto inserted = attributeMap.insert(std::pair{id, attribute});
    if (!inserted.second) {
      // TODO : make this a regular error
      throw std::runtime_error{"failed to insert attribute, duplicate"};
    }
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "errors generated during attributes CSV parsing");
  }

  return attributeMap;
}

std::unordered_map<std::size_t, Attributes> getFireredAttributesFromCsv(PtContext &ctx, std::string filePath)
{
  throw std::runtime_error{"TODO : implement"};
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

} // namespace porytiles

TEST_CASE("getEmeraldRubyAttributesFromCsv should parse input CSVs as expected")
{
  porytiles::PtContext ctx{};
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("It should parse an Emerald-style attributes CSV correctly")
  {
    porytiles::getEmeraldRubyAttributesFromCsv(ctx, behaviorMap, "res/tests/csv/emerald1.csv");
  }

  SUBCASE("It should fail on an Emerald-style CSV with no header")
  {
    CHECK_THROWS_WITH_AS(
        porytiles::getEmeraldRubyAttributesFromCsv(ctx, behaviorMap, "res/tests/csv/emerald_missing_header.csv"),
        "res/tests/csv/emerald_missing_header.csv: incorrect header row format, expected 'id,behavior'",
        porytiles::PtException);
  }
}
