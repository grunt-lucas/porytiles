#ifndef NORMALIZE_H
#define NORMALIZE_H

#include <array>
#include <cstdint>
#include <vector>

struct BGR15 {
  std::uint16_t bgr;

  auto operator<=>(BGR15 const&) const = default;
};

struct RGBA32 {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;

  auto operator<=>(RGBA32 const&) const = default;
};

struct GBATile {
  std::array<std::uint8_t, 32> paletteIndexPairs;

  auto operator<=>(GBATile const&) const = default;
};

struct GBAPalette {
  std::array<BGR15, 16> colors;

  auto operator<=>(GBAPalette const&) const = default;
};

struct Assignment {
  std::size_t tileIndex;
  std::size_t paletteIndex;
  bool hFlip;
  bool vFlip;
};

struct Compiled {
  std::vector<GBATile> tiles;
  std::vector<GBAPalette> palettes;
  std::vector<Assignment> assignments;
};

struct RGBATile {
  std::array<RGBA32, 64> pixels;
};

struct Decompiled {
  std::vector<RGBATile> tiles;
};

struct Config {
  std::size_t maxTiles;
  std::size_t maxPalettes;
};

Compiled compile(Config const&, Decompiled const&);
Decompiled decompile(Config const&, Compiled const&);

#endif
