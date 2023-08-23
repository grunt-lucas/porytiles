#ifndef PORYTILES_UTILITIES_H
#define PORYTILES_UTILITIES_H

#include <filesystem>
#include <string>
#include <unordered_map>

#include "errors_warnings.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName);

std::filesystem::path createTmpdir();

} // namespace porytiles

#endif // PORYTILES_UTILITIES_H
