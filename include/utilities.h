#ifndef PORYTILES_UTILITIES_H
#define PORYTILES_UTILITIES_H

#include <filesystem>
#include <string>
#include <unordered_map>

#include "errors_warnings.h"
#include "types.h"

namespace porytiles {

std::unordered_map<std::size_t, Attributes> getEmeraldRubyAttributesFromCsv(const ErrorsAndWarnings &err,
                                                                            const InputPaths &inputs, CompilerMode mode,
                                                                            std::string filePath);
std::unordered_map<std::size_t, Attributes> getFireredAttributesFromCsv(const ErrorsAndWarnings &err,
                                                                        const InputPaths &inputs, CompilerMode mode,
                                                                        std::string filePath);
std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName);
std::filesystem::path createTmpdir();

} // namespace porytiles

#endif // PORYTILES_UTILITIES_H
