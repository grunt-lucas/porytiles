#ifndef PORYTILES_UTILITIES_H
#define PORYTILES_UTILITIES_H

#include <filesystem>
#include <string>
#include <unordered_map>

#include "errors_warnings.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

std::unordered_map<std::size_t, Attributes>
getEmeraldRubyAttributesFromCsv(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                                std::string filePath);
std::unordered_map<std::size_t, Attributes> getFireredAttributesFromCsv(PtContext &ctx, std::string filePath);
std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName);
std::filesystem::path createTmpdir();

} // namespace porytiles

#endif // PORYTILES_UTILITIES_H
