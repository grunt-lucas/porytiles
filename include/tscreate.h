#ifndef TSCREATE_H
#define TSCREATE_H

#include <string>

namespace tscreate {
// Options
extern bool gOptVerboseOutput;
extern std::string_view gOptStructureFilePath;
extern std::string_view gOptTransparentColor;
extern std::string_view gOptMaxPalettes;

// Arguments (required)
extern std::string_view gArgMasterPngPath;
extern std::string_view gArgOutputPath;

// Generic tscreate exception class
class TsException : public std::runtime_error
{
public:
    TsException(const std::string& msg) : std::runtime_error{msg} {}
};
}

#endif // TSCREATE_H
