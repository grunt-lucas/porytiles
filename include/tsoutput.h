#ifndef PORYTILES_TSOUTPUT_H
#define PORYTILES_TSOUTPUT_H

#include "cli_parser.h"

#include <iostream>
#include <string>

namespace porytiles {
inline void verboseLog(const std::string& message) {
    if (gOptVerboseOutput)
        std::cout << PROGRAM_NAME << ": " << message << std::endl;
}
} // namespace porytiles

#endif // PORYTILES_TSOUTPUT_H