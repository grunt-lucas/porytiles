#include "types.h"

#include <filesystem>
#include <stdexcept>

#include "errors_warnings.h"

namespace porytiles {

static RGBATile uniformTile(const RGBA32 &color) noexcept
{
  RGBATile tile{};
  for (std::size_t i = 0; i < TILE_NUM_PIX; i++) {
    tile.pixels[i] = color;
  }
  return tile;
}

static GBATile uniformTile(const uint8_t &index) noexcept
{
  GBATile tile{};
  for (std::size_t i = 0; i < TILE_NUM_PIX; i++) {
    tile.colorIndexes[i] = index;
  }
  return tile;
}

const RGBA32 RGBA_BLACK = RGBA32{0, 0, 0, ALPHA_OPAQUE};
const RGBA32 RGBA_RED = RGBA32{255, 0, 0, ALPHA_OPAQUE};
const RGBA32 RGBA_GREEN = RGBA32{0, 255, 0, ALPHA_OPAQUE};
const RGBA32 RGBA_BLUE = RGBA32{0, 0, 255, ALPHA_OPAQUE};
const RGBA32 RGBA_YELLOW = RGBA32{255, 255, 0, ALPHA_OPAQUE};
const RGBA32 RGBA_MAGENTA = RGBA32{255, 0, 255, ALPHA_OPAQUE};
const RGBA32 RGBA_CYAN = RGBA32{0, 255, 255, ALPHA_OPAQUE};
const RGBA32 RGBA_WHITE = RGBA32{255, 255, 255, ALPHA_OPAQUE};
const RGBA32 RGBA_GREY = RGBA32{128, 128, 128, ALPHA_OPAQUE};
const RGBA32 RGBA_PURPLE = RGBA32{128, 0, 255, ALPHA_OPAQUE};
const RGBA32 RGBA_LIME = RGBA32{128, 255, 128, ALPHA_OPAQUE};

const BGR15 BGR_BLACK = rgbaToBgr(RGBA_BLACK);
const BGR15 BGR_RED = rgbaToBgr(RGBA_RED);
const BGR15 BGR_GREEN = rgbaToBgr(RGBA_GREEN);
const BGR15 BGR_BLUE = rgbaToBgr(RGBA_BLUE);
const BGR15 BGR_YELLOW = rgbaToBgr(RGBA_YELLOW);
const BGR15 BGR_MAGENTA = rgbaToBgr(RGBA_MAGENTA);
const BGR15 BGR_CYAN = rgbaToBgr(RGBA_CYAN);
const BGR15 BGR_WHITE = rgbaToBgr(RGBA_WHITE);
const BGR15 BGR_GREY = rgbaToBgr(RGBA_GREY);
const BGR15 BGR_PURPLE = rgbaToBgr(RGBA_PURPLE);
const BGR15 BGR_LIME = rgbaToBgr(RGBA_LIME);

const RGBATile RGBA_TILE_RED = uniformTile(RGBA_RED);
const RGBATile RGBA_TILE_GREEN = uniformTile(RGBA_GREEN);
const RGBATile RGBA_TILE_BLUE = uniformTile(RGBA_BLUE);
const RGBATile RGBA_TILE_YELLOW = uniformTile(RGBA_YELLOW);
const RGBATile RGBA_TILE_MAGENTA = uniformTile(RGBA_MAGENTA);

const GBATile GBA_TILE_TRANSPARENT = uniformTile(0);

std::string tileTypeString(TileType type)
{
  switch (type) {
  case TileType::FREESTANDING:
    return "freestanding";
  case TileType::LAYERED:
    return "layered";
  case TileType::ANIM:
    return "anim";
  default:
    internalerror("types::tileTypeString unknown TileType");
  }
  // unreachable, here for compiler
  throw std::runtime_error("types::tileTypeString reached unreachable code path");
}

std::string layerString(TileLayer layer)
{
  switch (layer) {
  case TileLayer::BOTTOM:
    return "bottom";
  case TileLayer::MIDDLE:
    return "middle";
  case TileLayer::TOP:
    return "top";
  default:
    internalerror("types::layerString unknown TileLayer");
  }
  // unreachable, here for compiler
  throw std::runtime_error("types::layerString reached unreachable code path");
}

std::string subtileString(Subtile subtile)
{
  switch (subtile) {
  case Subtile::NORTHWEST:
    return "northwest";
  case Subtile::NORTHEAST:
    return "northeast";
  case Subtile::SOUTHWEST:
    return "southwest";
  case Subtile::SOUTHEAST:
    return "southeast";
  default:
    internalerror("types::subtileString unknown Subtile");
  }
  // unreachable, here for compiler
  throw std::runtime_error("types::subtileString reached unreachable code path");
}

std::string layerTypeString(LayerType layerType)
{
  switch (layerType) {
  case LayerType::NORMAL:
    return "normal";
  case LayerType::COVERED:
    return "covered";
  case LayerType::SPLIT:
    return "split";
  default:
    internalerror("types::layerTypeString unknown LayerType");
  }
  // unreachable, here for compiler
  throw std::runtime_error("types::layerTypeString reached unreachable code path");
}

std::ostream &operator<<(std::ostream &os, const BGR15 &bgr)
{
  os << std::to_string(bgr.bgr);
  return os;
}

std::ostream &operator<<(std::ostream &os, const RGBA32 &rgba)
{
  // For debugging purposes, print the solid colors with names rather than int values
  if (rgba == RGBA_BLACK || rgba == bgrToRgba(BGR_BLACK)) {
    os << "black";
  }
  else if (rgba == RGBA_RED || rgba == bgrToRgba(BGR_RED)) {
    os << "red";
  }
  else if (rgba == RGBA_GREEN || rgba == bgrToRgba(BGR_GREEN)) {
    os << "green";
  }
  else if (rgba == RGBA_BLUE || rgba == bgrToRgba(BGR_BLUE)) {
    os << "blue";
  }
  else if (rgba == RGBA_YELLOW || rgba == bgrToRgba(BGR_YELLOW)) {
    os << "yellow";
  }
  else if (rgba == RGBA_MAGENTA || rgba == bgrToRgba(BGR_MAGENTA)) {
    os << "magenta";
  }
  else if (rgba == RGBA_CYAN || rgba == bgrToRgba(BGR_CYAN)) {
    os << "cyan";
  }
  else if (rgba == RGBA_WHITE || rgba == bgrToRgba(BGR_WHITE)) {
    os << "white";
  }
  else if (rgba == RGBA_GREY || rgba == bgrToRgba(BGR_GREY)) {
    os << "grey";
  }
  else if (rgba == RGBA_PURPLE || rgba == bgrToRgba(BGR_PURPLE)) {
    os << "purple";
  }
  else if (rgba == RGBA_LIME || rgba == bgrToRgba(BGR_LIME)) {
    os << "lime";
  }
  else {
    os << std::to_string(rgba.red) << "," << std::to_string(rgba.green) << "," << std::to_string(rgba.blue);
    if (rgba.alpha != 255) {
      // Only show alpha if not opaque
      os << "," << std::to_string(rgba.alpha);
    }
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const RGBATile &tile)
{
  os << "{";
  for (std::size_t i = 0; i < TILE_NUM_PIX; i++) {
    if (i % TILE_SIDE_LENGTH == 0) {
      os << "[" << i / 8 << "]=";
    }
    os << tile.pixels[i] << ";";
  }
  os << "}";
  return os;
}

BGR15 rgbaToBgr(const RGBA32 &rgba) noexcept
{
  // Convert each color channel from 8-bit to 5-bit, then shift into the right position
  return {std::uint16_t(((rgba.blue / 8) << 10) | ((rgba.green / 8) << 5) | (rgba.red / 8))};
}

RGBA32 bgrToRgba(const BGR15 &bgr) noexcept
{
  RGBA32 rgba{};
  rgba.red = (bgr.bgr & 0x1f) * 8;
  rgba.green = ((bgr.bgr >> 5) & 0x1f) * 8;
  rgba.blue = ((bgr.bgr >> 10) & 0x1f) * 8;
  rgba.alpha = 255;
  return rgba;
}

std::filesystem::path InputPaths::modeBasedInputPath(CompilerMode mode) const
{
  switch (mode) {
  case CompilerMode::PRIMARY:
    return primaryInputPath;
  case CompilerMode::SECONDARY:
    return secondaryInputPath;
  default:
    internalerror_unknownCompilerMode("types::InputPaths::modeBasedInputPath");
  }
  // unreachable, here for compiler
  throw std::runtime_error("types::modeBasedInputPath reached unreachable code path");
}

std::string compilerModeString(CompilerMode mode)
{
  switch (mode) {
  case CompilerMode::PRIMARY:
    return "primary";
  case CompilerMode::SECONDARY:
    return "secondary";
  default:
    internalerror_unknownCompilerMode("types::compilerModeString");
  }
  // unreachable, here for compiler
  throw std::runtime_error("types::compilerModeString reached unreachable code path");
}

} // namespace porytiles

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("RGBA32 to BGR15 should lose precision")
{
  porytiles::RGBA32 rgb1 = {0, 1, 2, 3};
  porytiles::RGBA32 rgb2 = {255, 255, 255, 255};

  porytiles::BGR15 bgr1 = rgbaToBgr(rgb1);
  porytiles::BGR15 bgr2 = rgbaToBgr(rgb2);

  CHECK(bgr1 == porytiles::BGR15{0});
  CHECK(bgr2 == porytiles::BGR15{32767}); // this value is uint16 max divided by two, i.e. 15 bits are set
}

TEST_CASE("RGBA32 should be ordered component-wise")
{
  porytiles::RGBA32 rgb1 = {0, 1, 2, 3};
  porytiles::RGBA32 rgb2 = {1, 2, 3, 4};
  porytiles::RGBA32 rgb3 = {2, 3, 4, 5};
  porytiles::RGBA32 zeros = {0, 0, 0, 0};

  CHECK(zeros == zeros);
  CHECK(zeros < rgb1);
  CHECK(rgb1 < rgb2);
  CHECK(rgb2 < rgb3);
}

TEST_CASE("BGR15 to RGBA should upconvert RGB channels to multiples of 8")
{
  porytiles::RGBA32 rgb1 = {0, 8, 80, 255};
  porytiles::RGBA32 rgb2 = {255, 255, 255, 255};
  porytiles::RGBA32 rgb3 = {2, 165, 96, 255};

  porytiles::BGR15 bgr1 = rgbaToBgr(rgb1);
  porytiles::BGR15 bgr2 = rgbaToBgr(rgb2);
  porytiles::BGR15 bgr3 = rgbaToBgr(rgb3);

  CHECK(porytiles::bgrToRgba(bgr1) == porytiles::RGBA32{0, 8, 80, 255});
  CHECK(porytiles::bgrToRgba(bgr2) == porytiles::RGBA32{248, 248, 248, 255});
  CHECK(porytiles::bgrToRgba(bgr3) == porytiles::RGBA32{0, 160, 96, 255});
}
