#ifndef TSCREATE_CLI_PARSER_H
#define TSCREATE_CLI_PARSER_H

#include <string>
#include <png.hpp>

namespace tscreate {
extern const char* const PROGRAM_NAME;
extern const char* const VERSION;
extern const char* const RELEASE_DATE;

// Options
extern bool         gOptVerboseOutput;
extern std::string  gOptStructureFilePath;
extern std::string  gOptTransparentColor;
extern int          gOptMaxPalettes;

// Arguments (required)
extern std::string  gArgMasterPngPath;
extern std::string  gArgOutputPath;

extern void parseOptions(int argc, char** argv);
}

#endif // TSCREATE_CLI_PARSER_H