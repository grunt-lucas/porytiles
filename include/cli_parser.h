#ifndef TSCREATE_CLI_PARSER_H
#define TSCREATE_CLI_PARSER_H

#include "rgb_color.h"

#include <string>
#include <png.hpp>

namespace tscreate {
extern const char* const PROGRAM_NAME;
extern const char* const VERSION;
extern const char* const RELEASE_DATE;

// Options
extern bool gOptVerboseOutput;
extern RgbColor gOptTransparentColor;
extern RgbColor gOptPrimerColor;
extern RgbColor gOptSiblingColor;
extern int gOptMaxPalettes;

// Arguments (required)
extern std::string gArgMasterPngPath;
extern std::string gArgOutputPath;

extern void parseOptions(int argc, char** argv);
} // namespace tscreate

#endif // TSCREATE_CLI_PARSER_H