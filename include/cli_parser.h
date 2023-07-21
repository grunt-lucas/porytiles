#ifndef PORYTILES_CLI_PARSER_H
#define PORYTILES_CLI_PARSER_H

#include <string>

#include "config.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

// TODO : these consts should have a different home
extern const std::string PROGRAM_NAME;
extern const std::string VERSION;
extern const std::string RELEASE_DATE;

void parseOptions(Config& config, int argc, char** argv);

}

#endif // PORYTILES_CLI_PARSER_H
