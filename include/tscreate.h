#ifndef TSCREATE_H
#define TSCREATE_H

#include <string>
#include <png.hpp>

namespace tscreate {
// Options
extern bool gOptVerboseOutput;
extern std::string_view gOptStructureFilePath;
extern std::string_view gOptTransparentColor;
extern std::string_view gOptMaxPalettes;

// Arguments (required)
extern std::string_view gArgMasterPngPath;
extern std::string_view gArgOutputPath;

extern const png::uint_32 TILE_DIMENSION;
extern const png::uint_32 PAL_SIZE_4BPP;

// Generic tscreate exception class
class TsException : public std::runtime_error
{
public:
    TsException(const std::string& msg) : std::runtime_error{msg} {}
};
}

#endif // TSCREATE_H
