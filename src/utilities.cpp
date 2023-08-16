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
getAttributesFromCsv(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                     std::string filePath)
{
  std::unordered_map<std::size_t, Attributes> attributeMap{};
  std::unordered_map<std::size_t, std::size_t> lineFirstSeen{};
  io::CSVReader<4> in{filePath};
  try {
    in.read_header(io::ignore_missing_column, "id", "behavior", "terrainType", "encounterType");
  }
  catch (const std::exception &e) {
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, filePath);
  }

  std::size_t id;
  bool hasId = in.has_column("id");

  std::string behavior;
  bool hasBehavior = in.has_column("behavior");

  std::string terrainType;
  bool hasTerrainType = in.has_column("terrainType");

  std::string encounterType;
  bool hasEncounterType = in.has_column("encounterType");

  if (!hasId || !hasBehavior || (hasTerrainType && !hasEncounterType) || (!hasTerrainType && hasEncounterType)) {
    fatalerror_invalidAttributesCsvHeader(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode, filePath);
  }

  if (ctx.targetBaseGame == TargetBaseGame::FIRERED && (!hasTerrainType || !hasEncounterType)) {
    warn_tooFewAttributesForTargetGame(ctx.err, filePath, ctx.targetBaseGame);
  }
  if ((ctx.targetBaseGame == TargetBaseGame::EMERALD || ctx.targetBaseGame == TargetBaseGame::RUBY) &&
      (hasTerrainType || hasEncounterType)) {
    warn_tooManyAttributesForTargetGame(ctx.err, filePath, ctx.targetBaseGame);
  }

  // processedUpToLine starts at 1 since we processed the header already, which was on line 1
  std::size_t processedUpToLine = 1;
  while (true) {
    bool readRow = false;
    try {
      readRow = in.read_row(id, behavior, terrainType, encounterType);
      processedUpToLine++;
    }
    catch (const std::exception &e) {
      // increment processedUpToLine here, since we threw before we could increment in the try
      processedUpToLine++;
      error_invalidCsvRowFormat(ctx.err, filePath, processedUpToLine);
      continue;
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
      error_unknownMetatileBehavior(ctx.err, filePath, processedUpToLine, behavior);
    }

    if (hasTerrainType) {
      try {
        attribute.terrainType = stringToTerrainType(terrainType);
      }
      catch (const std::invalid_argument &e) {
        error_invalidTerrainType(ctx.err, filePath, processedUpToLine, terrainType);
      }
    }
    if (hasEncounterType) {
      try {
        attribute.encounterType = stringToEncounterType(encounterType);
      }
      catch (const std::invalid_argument &e) {
        error_invalidEncounterType(ctx.err, filePath, processedUpToLine, encounterType);
      }
    }

    auto inserted = attributeMap.insert(std::pair{id, attribute});
    if (!inserted.second) {
      error_duplicateAttribute(ctx.err, filePath, processedUpToLine, id, lineFirstSeen.at(id));
    }
    if (!lineFirstSeen.contains(id)) {
      lineFirstSeen.insert(std::pair{id, processedUpToLine});
    }
  }

  if (ctx.err.errCount > 0) {
    die_errorCount(ctx.err, ctx.inputPaths.modeBasedInputPath(ctx.compilerConfig.mode),
                   "errors generated during attributes CSV parsing");
  }

  return attributeMap;
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

TEST_CASE("getAttributesFromCsv should parse input CSVs as expected")
{
  porytiles::PtContext ctx{};
  ctx.compilerConfig.mode = porytiles::CompilerMode::PRIMARY;
  ctx.err.printErrors = false;

  std::unordered_map<std::string, std::uint8_t> behaviorMap = {{"MB_NORMAL", 0}};

  SUBCASE("It should parse an Emerald-style attributes CSV correctly")
  {
    porytiles::getAttributesFromCsv(ctx, behaviorMap, "res/tests/csv/correct_1.csv");
  }
}
