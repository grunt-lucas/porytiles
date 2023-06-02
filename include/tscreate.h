#ifndef TSCREATE_H
#define TSCREATE_H

#include <string>
#include <png.hpp>

namespace tscreate {
extern const png::uint_32 TILE_DIMENSION;
extern const png::uint_32 PAL_SIZE_4BPP;
extern const png::uint_32 NUM_BG_PALS;

// Options
extern bool gOptVerboseOutput;
extern std::string gOptStructureFilePath;
extern std::string gOptTransparentColor;
extern png::uint_32 gOptMaxPalettes;

// Arguments (required)
extern std::string gArgMasterPngPath;
extern std::string gArgOutputPath;

// Generic tscreate exception class
class TsException : public std::runtime_error
{
public:
    TsException(const std::string& msg) : std::runtime_error{msg} {}
};
}

#endif // TSCREATE_H
