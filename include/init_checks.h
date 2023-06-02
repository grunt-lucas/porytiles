#ifndef INIT_CHECKS_H
#define INIT_CHECKS_H

#include <png.hpp>

namespace tscreate {
void validateMasterPngExistsAndDimensions(const std::string& masterPngPath);
}

#endif // INIT_CHECKS_H