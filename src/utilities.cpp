#include "utilities.h"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <csv.h>
#include <doctest.h>
#include <filesystem>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "errors_warnings.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

std::unordered_map<std::size_t, Attributes> getEmeraldRubyAttributesFromCsv(const ErrorsAndWarnings &err,
                                                                            const InputPaths &inputs, CompilerMode mode,
                                                                            std::string filePath)
{
  std::unordered_map<std::size_t, Attributes> attributeMap{};
  io::CSVReader<2> in{filePath};
  try {
    in.read_header(io::ignore_no_column, "id", "behaviour");
  }
  catch (const std::exception &e) {
    fatalerror(err, inputs, mode, fmt::format("{} had incorrect header row format, expected 'id,behaviour'", filePath));
  }
  return attributeMap;
}

std::unordered_map<std::size_t, Attributes> getFireredAttributesFromCsv(const ErrorsAndWarnings &err,
                                                                        const InputPaths &inputs, CompilerMode mode,
                                                                        std::string filePath)
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

  SUBCASE("It should fail on an Emerald-style CSV with no header")
  {
    CHECK_THROWS_WITH_AS(
        porytiles::getEmeraldRubyAttributesFromCsv(ctx.err, ctx.inputPaths, ctx.compilerConfig.mode,
                                                   "res/tests/csv/emerald_incorrect_1.csv"),
        "res/tests/csv/emerald_incorrect_1.csv had incorrect header row format, expected 'id,behaviour'",
        porytiles::PtException);
  }
}
