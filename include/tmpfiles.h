#ifndef PORYTILES_TMPFILES_H
#define PORYTILES_TMPFILES_H

#include <filesystem>
#include <string>

namespace porytiles {

std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName);
std::filesystem::path createTmpdir();

} // namespace porytiles

#endif // PORYTILES_TMPFILES_H
