#ifndef TSCREATE_TSOUTPUT_H
#define TSCREATE_TSOUTPUT_H

#include "cli_parser.h"

#include <iostream>
#include <string>

namespace tscreate {
inline void verboseLog(const std::string& message) {
    if (gOptVerboseOutput)
        std::cerr << PROGRAM_NAME << ": info: " << message << std::endl;
}
} // namespace tscreate

#endif // TSCREATE_TSOUTPUT_H