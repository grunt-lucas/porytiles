#ifndef PORYTILES_TYPES_H
#define PORYTILES_TYPES_H

#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <png.hpp>
#include <stdint.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <doctest.h>

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {
constexpr std::size_t TILE_SIDE_LENGTH_PIX = 8;
constexpr std::size_t TILE_NUM_PIX = TILE_SIDE_LENGTH_PIX * TILE_SIDE_LENGTH_PIX;
constexpr std::size_t METATILE_TILE_SIDE_LENGTH_TILES = 2;
constexpr std::size_t METATILE_SIDE_LENGTH = TILE_SIDE_LENGTH_PIX * METATILE_TILE_SIDE_LENGTH_TILES;
constexpr std::size_t METATILES_IN_ROW = 8;
constexpr std::size_t PAL_SIZE = 16;
constexpr std::size_t MAX_BG_PALETTES = 16;
constexpr std::size_t TILES_PER_METATILE_DUAL = 8;
constexpr std::size_t TILES_PER_METATILE_TRIPLE = 12;
constexpr std::size_t BYTES_PER_METATILE_ENTRY = 2;
constexpr std::size_t BYTES_PER_ATTRIBUTE_FIRERED = 4;
constexpr std::size_t BYTES_PER_ATTRIBUTE_EMERALD = 2; // RUBY also uses this value
constexpr std::uint8_t INVALID_INDEX_PIXEL_VALUE = 255;

// --------------------
// |    DATA TYPES    |
// --------------------

// NOTE : For clang, default spaceship for std::array not yet supported by libc++
// https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
// https://reviews.llvm.org/D132265
// https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
// So we manually implement spaceship for some types here

/**
 * An PNG from png++, tagged with a name and frame.
 */
template <typename T> struct AnimationPng {
  png::image<T> png;
  std::string animName;
  std::string frameName;

  AnimationPng(png::image<T> png, std::string animName, std::string frameName)
      : png{png}, animName{animName}, frameName{frameName}
  {
  }
};

/**
 * BGR15 color format. 5 bits per color with blue in most significant bits. Top bit unused.
 */
struct BGR15 {
  std::uint16_t bgr;

  auto operator<=>(const BGR15 &) const = default;

  friend std::ostream &operator<<(std::ostream &, const BGR15 &);
};

extern const BGR15 BGR_BLACK;
extern const BGR15 BGR_RED;
extern const BGR15 BGR_GREEN;
extern const BGR15 BGR_BLUE;
extern const BGR15 BGR_YELLOW;
extern const BGR15 BGR_MAGENTA;
extern const BGR15 BGR_CYAN;
extern const BGR15 BGR_WHITE;
extern const BGR15 BGR_GREY;
} // namespace porytiles

template <> struct std::hash<porytiles::BGR15> {
  std::size_t operator()(const porytiles::BGR15 &bgr) const noexcept { return std::hash<uint16_t>{}(bgr.bgr); }
};

namespace porytiles {
/**
 * RGBA32 format. 1 byte per color and 1 byte for alpha channel.
 */
struct RGBA32 {
  std::uint8_t red;
  std::uint8_t green;
  std::uint8_t blue;
  std::uint8_t alpha;

  [[nodiscard]] std::string jasc() const
  {
    return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
  }

  auto operator<=>(const RGBA32 &rgba) const = default;

  friend std::ostream &operator<<(std::ostream &os, const RGBA32 &rgba);
};

constexpr std::uint8_t ALPHA_TRANSPARENT = 0;
constexpr std::uint8_t ALPHA_OPAQUE = 255;

extern const RGBA32 RGBA_BLACK;
extern const RGBA32 RGBA_RED;
extern const RGBA32 RGBA_GREEN;
extern const RGBA32 RGBA_BLUE;
extern const RGBA32 RGBA_YELLOW;
extern const RGBA32 RGBA_MAGENTA;
extern const RGBA32 RGBA_CYAN;
extern const RGBA32 RGBA_WHITE;
extern const RGBA32 RGBA_GREY;
extern const RGBA32 RGBA_PURPLE;
extern const RGBA32 RGBA_LIME;

BGR15 rgbaToBgr(const RGBA32 &rgba) noexcept;

RGBA32 bgrToRgba(const BGR15 &bgr) noexcept;

enum class TileType { FREESTANDING, LAYERED, ANIM, PRIMER };

std::string tileTypeString(TileType type);

enum class TileLayer { BOTTOM, MIDDLE, TOP };

std::string layerString(TileLayer layer);
TileLayer indexToLayer(std::size_t index, bool tripleLayer);

enum class Subtile { NORTHWEST = 0, NORTHEAST = 1, SOUTHWEST = 2, SOUTHEAST = 3 };

std::string subtileString(Subtile subtile);
Subtile indexToSubtile(std::size_t index);

// Normal = Middle/Top
// Covered = Bottom/Middle
// Split = Bottom/Top
enum class LayerType { NORMAL, COVERED, SPLIT, TRIPLE };
std::string layerTypeString(LayerType layerType);
std::uint8_t layerTypeValue(LayerType layerType);
LayerType layerTypeFromInt(std::uint8_t layerInt);

enum class EncounterType { NONE, LAND, WATER };
std::uint8_t encounterTypeValue(EncounterType encounterType);
std::string encounterTypeString(EncounterType encounterType);
EncounterType stringToEncounterType(std::string string);
EncounterType encounterTypeFromInt(std::uint8_t encounterInt);

enum class TerrainType { NORMAL, GRASS, WATER, WATERFALL };
std::uint8_t terrainTypeValue(TerrainType terrainType);
std::string terrainTypeString(TerrainType terrainType);
TerrainType stringToTerrainType(std::string string);
TerrainType terrainTypeFromInt(std::uint8_t terrainInt);

enum class TargetBaseGame { EMERALD, FIRERED, RUBY };
std::string targetBaseGameString(TargetBaseGame game);

struct Attributes {
  TargetBaseGame baseGame;
  LayerType layerType;
  std::uint16_t metatileBehavior;
  EncounterType encounterType;
  TerrainType terrainType;

  Attributes()
      : baseGame{TargetBaseGame::EMERALD}, layerType{LayerType::TRIPLE}, metatileBehavior{0},
        encounterType{EncounterType::NONE}, terrainType{TerrainType::NORMAL}
  {
  }
};

/**
 * A tile of RGBA32 colors.
 */
struct RGBATile {
  std::array<RGBA32, TILE_NUM_PIX> pixels;

  /*
   * Metadata Fields:
   * These are used by the various components to track metadata around the usage context of an RGBATile. Allows
   * Porytiles to give much more detailed error messages.
   */
  TileType type;

  // raw tile index, used for FREESTANDING and ANIM types
  std::size_t tileIndex;

  // LAYERED specific metadata
  TileLayer layer;
  std::size_t metatileIndex;
  Subtile subtile;

  // ANIM specific metadata
  std::string anim;
  std::string frame;

  // PRIMER specific metadata
  std::string primer;

  // Metatile attributes for this tile
  Attributes attributes;

  [[nodiscard]] RGBA32 getPixel(size_t row, size_t col) const
  {
    if (row >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: RGBATile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
    }
    if (col >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: RGBATile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
    }
    return pixels.at(row * TILE_SIDE_LENGTH_PIX + col);
  }

  [[nodiscard]] bool transparent(const RGBA32 &transparencyColor) const
  {
    for (std::size_t i = 0; i < pixels.size(); i++) {
      if (pixels[i] != transparencyColor && pixels[i].alpha != ALPHA_TRANSPARENT) {
        return false;
      }
    }
    return true;
  }

  void setPixel(std::size_t row, std::size_t col, const RGBA32 &value)
  {
    if (row >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: RGBATile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
    }
    if (col >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: RGBATile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
    }
    pixels.at(row * TILE_SIDE_LENGTH_PIX + col) = value;
  }

  bool equalsAfterBgrConversion(const RGBATile &other)
  {
    for (std::size_t i = 0; i < TILE_NUM_PIX; i++) {
      if (rgbaToBgr(this->pixels.at(i)) != rgbaToBgr(other.pixels.at(i))) {
        return false;
      }
    }
    return true;
  }

  auto operator==(const RGBATile &other) const { return this->pixels == other.pixels; }

  // Ignore the other fields for purposes of ordering the tiles
  auto operator<=>(const RGBATile &other) const
  {
    if (this->pixels == other.pixels) {
      return std::strong_ordering::equal;
    }
    else if (this->pixels < other.pixels) {
      return std::strong_ordering::less;
    }
    return std::strong_ordering::greater;
  }

  friend std::ostream &operator<<(std::ostream &os, const RGBATile &tile);
};

extern const RGBATile RGBA_TILE_BLACK;
extern const RGBATile RGBA_TILE_RED;
extern const RGBATile RGBA_TILE_GREEN;
extern const RGBATile RGBA_TILE_BLUE;
extern const RGBATile RGBA_TILE_YELLOW;
extern const RGBATile RGBA_TILE_MAGENTA;
extern const RGBATile RGBA_TILE_CYAN;
extern const RGBATile RGBA_TILE_WHITE;

/**
 * A tile of palette indexes.
 */
struct GBATile {
  std::array<std::uint8_t, TILE_NUM_PIX> colorIndexes;

  GBATile() : colorIndexes{} {}

  [[nodiscard]] std::uint8_t getPixel(size_t index) const
  {
    if (index >= TILE_NUM_PIX) {
      throw std::out_of_range{"internal: GBATile::getPixel index argument out of bounds (" + std::to_string(index) +
                              ")"};
    }
    return colorIndexes.at(index);
  }

  [[nodiscard]] std::uint8_t getPixel(size_t row, size_t col) const
  {
    if (row >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: GBATile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
    }
    if (col >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: GBATile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
    }
    return colorIndexes.at(row * TILE_SIDE_LENGTH_PIX + col);
  }

  auto operator<=>(const GBATile &other) const
  {
    if (this->colorIndexes == other.colorIndexes) {
      return std::strong_ordering::equal;
    }
    else if (this->colorIndexes < other.colorIndexes) {
      return std::strong_ordering::less;
    }
    return std::strong_ordering::greater;
  }

  auto operator==(const GBATile &other) const { return this->colorIndexes == other.colorIndexes; }
};

extern const GBATile GBA_TILE_TRANSPARENT;
} // namespace porytiles

template <> struct std::hash<porytiles::GBATile> {
  std::size_t operator()(const porytiles::GBATile &tile) const noexcept
  {
    // TODO : better hash function
    std::size_t hashValue = 0;
    for (auto index : tile.colorIndexes) {
      hashValue ^= std::hash<uint8_t>{}(index);
    }
    return hashValue;
  }
};

namespace porytiles {
/**
 * A palette of PAL_SIZE (16) BGR15 colors.
 */
struct GBAPalette {
  std::size_t size;
  std::array<BGR15, PAL_SIZE> colors;

  GBAPalette() : size{0}, colors{} {}
};

/**
 * A tile assignment, i.e. the representation of a tile within a metatile. Maps a given tile index to a hardware palette
 * index and the corresponding flips.
 */
struct MetatileEntry {
  std::size_t tileIndex;
  std::size_t paletteIndex;
  bool hFlip;
  bool vFlip;

  // Store this here for the attributes emitter
  Attributes attributes;
};

struct CompiledAnimFrame {
  std::vector<GBATile> tiles;
  std::string frameName;

  CompiledAnimFrame(std::string frameName) : tiles{}, frameName{frameName} {}
};

struct CompiledAnimation {
  std::vector<CompiledAnimFrame> frames;
  std::string animName;

  CompiledAnimation(std::string animName) : frames{}, animName{animName} {}

  const CompiledAnimFrame &keyFrame() const { return frames.at(keyFrameIndex()); }

  static const std::size_t keyFrameIndex() { return 0; }
};

/**
 * A compiled tileset.
 *
 * The `tiles' field contains the normalized tiles from the input tilesheets. This field can be directly written out to
 * `tiles.png'.
 *
 * The `paletteIndexesOfTile'
 *
 * The `paletteIndexes' contains the palette index (into the `palettes' field) for each corresponding GBATile in
 * `tiles'.
 *
 * The `palettes' field are hardware palettes, i.e. there should be numPalsInPrimary palettes for a primary tileset, or
 * `numPalettesTotal - numPalsInPrimary' palettes for a secondary tileset.
 *
 * The `metatileEntries' vector contains metatile entries, which are just a tile index, palette index, and flips.
 */
struct CompiledTileset {
  std::vector<GBATile> tiles;
  std::vector<std::size_t> paletteIndexesOfTile;
  std::vector<GBAPalette> palettes;
  std::vector<MetatileEntry> metatileEntries;
  std::unordered_map<BGR15, std::size_t> colorIndexMap;
  std::unordered_map<GBATile, std::size_t> tileIndexes;
  std::vector<CompiledAnimation> anims;

  CompiledTileset()
      : tiles{}, paletteIndexesOfTile{}, palettes{}, metatileEntries{}, colorIndexMap{}, tileIndexes{}, anims{}
  {
  }

  std::unordered_map<std::size_t, Attributes> generateAttributesMap(bool tripleLayer) const
  {
    std::unordered_map<std::size_t, Attributes> attributes{};
    for (std::size_t entryIndex = 0; const auto &metatileEntry : metatileEntries) {
      attributes.insert(std::pair{entryIndex / (tripleLayer ? 12 : 8), metatileEntry.attributes});
      entryIndex++;
    }
    return attributes;
  }
};

/**
 * An AnimFrame is just a vector of RGBATiles representing one frame of an animation
 */
struct DecompiledAnimFrame {
  std::vector<RGBATile> tiles;
  std::string frameName;

  DecompiledAnimFrame(std::string frameName) : tiles{}, frameName{frameName} {}

  std::size_t size() const { return tiles.size(); }
};

/**
 * An Animation is a vector of AnimFrames. The first frame is the 'key frame'. That is, the regular tileset
 * must use tiles from the key frame in order for them to properly link into the animation. The other frames
 * of the animation are not stored in tiles.png, but are dynamically copied in from ROM by the game engine.
 */
struct DecompiledAnimation {
  std::vector<DecompiledAnimFrame> frames;
  std::string animName;

  DecompiledAnimation(std::string animName) : frames{}, animName{animName} {}

  const DecompiledAnimFrame &keyFrame() const { return frames.at(keyFrameIndex()); }

  static const std::size_t keyFrameIndex() { return 0; }

  std::size_t size() const { return frames.size(); }
};

/**
 * A decompiled tileset, which is just two vectors. One for the standard tiles. Another that holds Animations, a
 * special type that handles animated tiles.
 */
struct DecompiledTileset {
  std::vector<RGBATile> tiles;

  /*
   * This field holds the decompiled anim data from the optionally supplied anims folder. Each anim is essentially a
   * vector of frames, where each frame is a vector of RGBATiles representing that frame. The compiler will ultimately
   * copy frame 0 into the start of VRAM. Users can "use" an animated tile by simply painting a frame 0 tile onto the
   * RGBA metatile sheet. The compiler will automatically link it to one of the anim tiles at the start of tiles.png
   */
  std::vector<DecompiledAnimation> anims;

  bool tripleLayer;
};

/*
 * Normalized types
 */

/**
 * TODO : fill in doc comment
 */
struct NormalizedPixels {
  std::array<std::uint8_t, TILE_NUM_PIX> colorIndexes;

  auto operator<=>(const NormalizedPixels &other) const
  {
    if (this->colorIndexes == other.colorIndexes) {
      return std::strong_ordering::equal;
    }
    else if (this->colorIndexes < other.colorIndexes) {
      return std::strong_ordering::less;
    }
    return std::strong_ordering::greater;
  }

  auto operator==(const NormalizedPixels &other) const { return this->colorIndexes == other.colorIndexes; }
};
} // namespace porytiles

template <> struct std::hash<porytiles::NormalizedPixels> {
  std::size_t operator()(const porytiles::NormalizedPixels &pixels) const noexcept
  {
    // TODO : better hash function
    std::size_t hashValue = 0;
    for (auto pixel : pixels.colorIndexes) {
      hashValue ^= std::hash<uint8_t>{}(pixel);
    }
    return hashValue;
  }
};

namespace porytiles {
/**
 * TODO : fill in doc comment
 */
struct NormalizedPalette {
  int size;
  std::array<BGR15, PAL_SIZE> colors;
};
} // namespace porytiles

template <> struct std::hash<porytiles::NormalizedPalette> {
  std::size_t operator()(const porytiles::NormalizedPalette &palette) const noexcept
  {
    // TODO : better hash function
    std::size_t hashValue = 0;
    hashValue ^= std::hash<int>{}(palette.size);
    for (auto color : palette.colors) {
      hashValue ^= std::hash<porytiles::BGR15>{}(color);
    }
    return hashValue;
  }
};

namespace porytiles {
/**
 * TODO : fill in doc comment
 */
struct NormalizedTile {
  /*
   * Vector here to represent frames. Animated tiles can have multiple frames, with each frame corresponding to
   * a frame in the animation. Regular tiles will have a size 1 vector, since they don't have frames. For tiles with
   * more than one frame, the first frame is the "key" frame. The key frame is the frame we place in tiles.png, and
   * it is the frame the user supplies in the layer PNGs to "key" in to a particular animation. However, the actual
   * animation is stored in the 00.png, 01.png, etc. The game will never display the key-frame in-game.
   */
  std::vector<NormalizedPixels> frames;
  /*
   * This palette is the combined palette for all frames. We want to handle it this way, since for anim tiles, each
   * frame in the animation must be placed in the same hardware palette.
   */
  NormalizedPalette palette;
  bool hFlip;
  bool vFlip;

  /*
   * Metadata Fields:
   * These are used by the various components to track metadata around the usage context of a NormalizedTile. Allows
   * Porytiles to give much more detailed error messages.
   */
  TileType type;

  // raw tile index, used for FREESTANDING and ANIM types
  std::size_t tileIndex;

  // LAYERED specific metadata
  TileLayer layer;
  std::size_t metatileIndex;
  Subtile subtile;

  // ANIM specific metadata
  std::string anim;

  // PRIMER specific metadata
  std::string primer;

  // Metatile attributes for this tile
  Attributes attributes;

  explicit NormalizedTile(RGBA32 transparency) : frames{}, palette{}, hFlip{}, vFlip{}
  {
    palette.size = 1;
    palette.colors[0] = rgbaToBgr(transparency);
    frames.resize(1);
  }

  void copyMetadataFrom(const RGBATile &tile)
  {
    this->type = tile.type;
    this->tileIndex = tile.tileIndex;
    this->layer = tile.layer;
    this->metatileIndex = tile.metatileIndex;
    this->subtile = tile.subtile;
    this->anim = tile.anim;
    this->primer = tile.primer;
    this->attributes = tile.attributes;
  }

  [[nodiscard]] bool transparent() const { return palette.size == 1; }

  void setPixel(std::size_t frame, std::size_t row, std::size_t col, uint8_t value)
  {
    if (frame >= frames.size()) {
      throw std::out_of_range{"internal: NormalizedTile::setPixel frame argument out of bounds (" +
                              std::to_string(frame) + " >= " + std::to_string(frames.size()) + ")"};
    }
    if (row >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: NormalizedTile::setPixel row argument out of bounds (" + std::to_string(row) +
                              ")"};
    }
    if (col >= TILE_SIDE_LENGTH_PIX) {
      throw std::out_of_range{"internal: NormalizedTile::setPixel col argument out of bounds (" + std::to_string(col) +
                              ")"};
    }
    frames.at(frame).colorIndexes[row * TILE_SIDE_LENGTH_PIX + col] = value;
  }

  const NormalizedPixels &keyFrame() const { return frames.at(keyFrameIndex()); }

  static const std::size_t keyFrameIndex() { return 0; }
};
} // namespace porytiles

template <> struct std::hash<porytiles::NormalizedTile> {
  std::size_t operator()(const porytiles::NormalizedTile &tile) const noexcept
  {
    // TODO : better hash function
    std::size_t hashValue = 0;
    for (const auto &layer : tile.frames) {
      hashValue ^= std::hash<porytiles::NormalizedPixels>{}(layer);
    }
    hashValue ^= std::hash<porytiles::NormalizedPalette>{}(tile.palette);
    hashValue ^= std::hash<bool>{}(tile.hFlip);
    hashValue ^= std::hash<bool>{}(tile.vFlip);
    return hashValue;
  }
};

namespace porytiles {

// ---------------------
// |   CONFIG TYPES    |
// ---------------------

enum class TilesOutputPalette { TRUE_COLOR, GREYSCALE };

enum class Subcommand { DECOMPILE_PRIMARY, DECOMPILE_SECONDARY, COMPILE_PRIMARY, COMPILE_SECONDARY };

enum class CompilerMode { PRIMARY, SECONDARY };

enum class AssignAlgorithm { DFS, BFS };

enum class DecompilerMode { PRIMARY, SECONDARY };

/*
 * TODO 1.0.0 : combine CompilerMode and DecompilerMode into a single type: CompilationMode ?
 */

/*
 * TODO 1.0.0 : Remove all checks against Subcommand type in the codebase, prefer explicit CompilationMode parameters
 */

std::string subcommandString(Subcommand subcommand);
std::string compilerModeString(CompilerMode mode);
std::string assignAlgorithmString(AssignAlgorithm algo);
std::string decompilerModeString(DecompilerMode mode);

struct FieldmapConfig {
  // Fieldmap params
  std::size_t numTilesInPrimary;
  std::size_t numTilesTotal;
  std::size_t numMetatilesInPrimary;
  std::size_t numMetatilesTotal;
  std::size_t numPalettesInPrimary;
  std::size_t numPalettesTotal;
  std::size_t numTilesPerMetatile;

  [[nodiscard]] std::size_t numPalettesInSecondary() const { return numPalettesTotal - numPalettesInPrimary; }

  [[nodiscard]] std::size_t numTilesInSecondary() const { return numTilesTotal - numTilesInPrimary; }

  [[nodiscard]] std::size_t numMetatilesInSecondary() const { return numMetatilesTotal - numMetatilesInPrimary; }

  static FieldmapConfig pokeemeraldDefaults()
  {
    FieldmapConfig config;
    config.numTilesInPrimary = 512;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 512;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 6;
    config.numPalettesTotal = 13;
    config.numTilesPerMetatile = 12;
    return config;
  }

  static FieldmapConfig pokefireredDefaults()
  {
    FieldmapConfig config;
    config.numTilesInPrimary = 640;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 640;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 7;
    config.numPalettesTotal = 13;
    config.numTilesPerMetatile = 12;
    return config;
  }

  static FieldmapConfig pokerubyDefaults()
  {
    FieldmapConfig config;
    config.numTilesInPrimary = 512;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 512;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 6;
    config.numPalettesTotal = 12;
    config.numTilesPerMetatile = 12;
    return config;
  }
};

struct CompilerSourcePaths {
  std::string primarySourcePath;
  std::string secondarySourcePath;
  std::string metatileBehaviors;

  std::filesystem::path bottomPrimaryTilesheet() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"bottom.png"};
  }

  std::filesystem::path middlePrimaryTilesheet() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"middle.png"};
  }

  std::filesystem::path topPrimaryTilesheet() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"top.png"};
  }

  std::filesystem::path bottomSecondaryTilesheet() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"bottom.png"};
  }

  std::filesystem::path middleSecondaryTilesheet() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"middle.png"};
  }

  std::filesystem::path topSecondaryTilesheet() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"top.png"};
  }

  std::filesystem::path primaryAttributes() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"attributes.csv"};
  }

  std::filesystem::path secondaryAttributes() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"attributes.csv"};
  }

  std::filesystem::path primaryAnims() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"anim"};
  }

  std::filesystem::path secondaryAnims() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"anim"};
  }

  std::filesystem::path primaryAssignCache() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"assign.cache"};
  }

  std::filesystem::path secondaryAssignCache() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"assign.cache"};
  }

  std::filesystem::path primaryPalettePrimers() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"palette-primers"};
  }

  std::filesystem::path secondaryPalettePrimers() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"palette-primers"};
  }

  std::filesystem::path modeBasedSrcPath(CompilerMode mode) const;
  std::filesystem::path modeBasedBottomTilesheetPath(CompilerMode mode) const;
  std::filesystem::path modeBasedMiddleTilesheetPath(CompilerMode mode) const;
  std::filesystem::path modeBasedTopTilesheetPath(CompilerMode mode) const;
  std::filesystem::path modeBasedAttributePath(CompilerMode mode) const;
  std::filesystem::path modeBasedAnimPath(CompilerMode mode) const;
  std::filesystem::path modeBasedAssignCachePath(CompilerMode mode) const;
  std::filesystem::path modeBasedPalettePrimerPath(CompilerMode mode) const;
};

struct DecompilerSourcePaths {
  std::string primarySourcePath;
  std::string secondarySourcePath;
  std::string metatileBehaviors;

  std::filesystem::path primaryMetatilesBin() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"metatiles.bin"};
  }

  std::filesystem::path primaryAttributesBin() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"metatile_attributes.bin"};
  }

  std::filesystem::path primaryTilesPng() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"tiles.png"};
  }

  std::filesystem::path primaryPalettes() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"palettes"};
  }

  std::filesystem::path primaryAnims() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / std::filesystem::path{"anim"};
  }

  std::filesystem::path secondaryMetatilesBin() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"metatiles.bin"};
  }

  std::filesystem::path secondaryAttributesBin() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"metatile_attributes.bin"};
  }

  std::filesystem::path secondaryTilesPng() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"tiles.png"};
  }

  std::filesystem::path secondaryPalettes() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"palettes"};
  }

  std::filesystem::path secondaryAnims() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / std::filesystem::path{"anim"};
  }

  std::filesystem::path modeBasedSrcPath(DecompilerMode mode) const;
  std::filesystem::path modeBasedTilesPath(DecompilerMode mode) const;
  std::filesystem::path modeBasedMetatilesPath(DecompilerMode mode) const;
  std::filesystem::path modeBasedAttributePath(DecompilerMode mode) const;
  std::filesystem::path modeBasedPalettePath(DecompilerMode mode) const;
  std::filesystem::path modeBasedAnimPath(DecompilerMode mode) const;
};

struct Output {
  TilesOutputPalette paletteMode;
  bool disableMetatileGeneration;
  bool disableAttributeGeneration;
  std::string path;

  Output()
      : paletteMode{TilesOutputPalette::GREYSCALE}, disableMetatileGeneration{false}, disableAttributeGeneration{false},
        path{}
  {
  }
};

struct CompilerConfig {
  RGBA32 transparencyColor;
  bool tripleLayer;
  bool cacheAssign;
  bool forceParamSearchMatrix;
  bool providedAssignCacheOverride;
  bool providedPrimaryAssignCacheOverride;
  std::string defaultBehavior;
  std::string defaultEncounterType;
  std::string defaultTerrainType;

  // Palette assignment algorithm configuration
  AssignAlgorithm primaryAssignAlgorithm;
  std::size_t primaryExploredNodeCutoff;
  std::size_t primaryBestBranches;
  bool primarySmartPrune;
  bool readPrimaryAssignCache;
  AssignAlgorithm secondaryAssignAlgorithm;
  std::size_t secondaryExploredNodeCutoff;
  std::size_t secondaryBestBranches;
  bool secondarySmartPrune;
  bool readSecondaryAssignCache;

  CompilerConfig()
      : transparencyColor{RGBA_MAGENTA}, tripleLayer{true}, cacheAssign{true}, forceParamSearchMatrix{false},
        providedAssignCacheOverride{false}, providedPrimaryAssignCacheOverride{false}, defaultBehavior{"0"},
        defaultEncounterType{"0"}, defaultTerrainType{"0"}, primaryAssignAlgorithm{AssignAlgorithm::DFS},
        primaryExploredNodeCutoff{2'000'000}, primaryBestBranches{SIZE_MAX}, primarySmartPrune{false},
        readPrimaryAssignCache{false}, secondaryAssignAlgorithm{AssignAlgorithm::DFS},
        secondaryExploredNodeCutoff{2'000'000}, secondaryBestBranches{SIZE_MAX}, secondarySmartPrune{false},
        readSecondaryAssignCache{false}
  {
  }
};

struct DecompilerConfig {
  bool normalizeTransparency;
  RGBA32 normalizeTransparencyColor;
  DecompilerConfig() : normalizeTransparency{true}, normalizeTransparencyColor{RGBA_MAGENTA} {}
};

struct CompilerContext {
  std::unique_ptr<CompiledTileset> pairedPrimaryTileset;
  std::unique_ptr<CompiledTileset> resultTileset;
  std::unordered_map<BGR15, std::tuple<RGBA32, RGBATile, std::size_t, std::size_t>> bgrToRgba;
  std::size_t exploredNodeCounter;

  CompilerContext() : pairedPrimaryTileset{nullptr}, resultTileset{nullptr}, bgrToRgba{}, exploredNodeCounter{} {}
};

struct DecompilerContext {
  std::unique_ptr<CompiledTileset> pairedPrimaryTileset;
  std::unique_ptr<DecompiledTileset> resultTileset;

  DecompilerContext() : pairedPrimaryTileset{nullptr}, resultTileset{nullptr} {}
};

} // namespace porytiles

#endif // PORYTILES_TYPES_H
