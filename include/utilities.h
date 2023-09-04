#ifndef PORYTILES_UTILITIES_H
#define PORYTILES_UTILITIES_H

#include <filesystem>
#include <string>
#include <unordered_map>

#include "errors_warnings.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

template <typename T> static T parseInteger(const char *integerString)
{
  // TODO : rewrite cli_parser::parseIntegralOption to use this function
  try {
    size_t pos;
    T arg = std::stoi(integerString, &pos, 0);
    if (std::string{integerString}.size() != pos) {
      // throw here so it catches below and prints an error message
      throw std::runtime_error{"invalid integral string: " + std::string{integerString}};
    }
    return arg;
  }
  catch (const std::exception &e) {
    throw std::runtime_error{e.what()};
  }
  // unreachable, here for compiler
  throw std::runtime_error("cli_parser::parseIntegralOption reached unreachable code path");
}

std::vector<std::string> split(std::string input, const std::string &delimiter);

std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName);

std::filesystem::path createTmpdir();

} // namespace porytiles

#endif // PORYTILES_UTILITIES_H
