#ifndef PORYTILES_CLI_PARSER_H
#define PORYTILES_CLI_PARSER_H

#include "config.h"
#include "types.h"

/**
 * TODO : fill in doc comment
 */

namespace porytiles {

// TODO : these consts should have a different home
extern const char* const PROGRAM_NAME;
extern const char* const VERSION;
extern const char* const RELEASE_DATE;

void parseOptions(Config& config, int argc, char** argv);

}

#endif