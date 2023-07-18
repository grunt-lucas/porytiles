#ifndef PORYTILES_TMPFILES_H
#define PORYTILES_TMPFILES_H

#include <filesystem>
#include <string>

namespace porytiles {

std::filesystem::path getTmpfilePath(std::string fileName);

}

#endif