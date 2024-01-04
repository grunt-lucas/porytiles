#include "utilities.h"

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include <algorithm>
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

} // namespace porytiles
