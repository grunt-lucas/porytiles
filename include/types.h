#ifndef PORYTILES_TYPES_H
#define PORYTILES_TYPES_H

#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <png.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <doctest.h>

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {
constexpr std::size_t TILE_SIDE_LENGTH = 8;
constexpr std::size_t TILE_NUM_PIX = TILE_SIDE_LENGTH * TILE_SIDE_LENGTH;
constexpr std::size_t METATILE_TILE_SIDE_LENGTH = 2;
constexpr std::size_t METATILE_SIDE_LENGTH = TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH;
constexpr std::size_t METATILES_IN_ROW = 8;
constexpr std::size_t PAL_SIZE = 16;
constexpr std::size_t MAX_BG_PALETTES = 16;
constexpr std::uint8_t INVALID_INDEX_PIXEL_VALUE = 255;

// --------------------
// |    DATA TYPES    |
// --------------------

// TODO : For clang, default spaceship for std::array not yet supported by libc++
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
  std::string frame;

  AnimationPng(png::image<T> png, std::string animName, std::string frame) : png{png}, animName{animName}, frame{frame}
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

enum class TileType { FREESTANDING, LAYERED, ANIM };

std::string tileTypeString(TileType type);

enum class TileLayer { BOTTOM, MIDDLE, TOP };

std::string layerString(TileLayer layer);

enum class Subtile { NORTHWEST = 0, NORTHEAST = 1, SOUTHWEST = 2, SOUTHEAST = 3 };

std::string subtileString(Subtile subtile);

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

enum class TerrainType { NORMAL, GRASS, WATER, WATERFALL };
std::uint8_t terrainTypeValue(TerrainType terrainType);
std::string terrainTypeString(TerrainType terrainType);
TerrainType stringToTerrainType(std::string string);

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

  // Metatile attributes for this tile
  Attributes attributes;

  [[nodiscard]] RGBA32 getPixel(size_t row, size_t col) const
  {
    if (row >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: RGBATile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
    }
    if (col >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: RGBATile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
    }
    return pixels.at(row * TILE_SIDE_LENGTH + col);
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
    if (row >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: RGBATile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
    }
    if (col >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: RGBATile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
    }
    pixels.at(row * TILE_SIDE_LENGTH + col) = value;
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
    if (row >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: GBATile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
    }
    if (col >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: GBATile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
    }
    return colorIndexes.at(row * TILE_SIDE_LENGTH + col);
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
struct Assignment {
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
 * The `assignments' vector contains the actual tile palette assignments and flips which can be used to construct the
 * final metatiles.
 */
struct CompiledTileset {
  std::vector<GBATile> tiles;
  std::vector<std::size_t> paletteIndexesOfTile;
  std::vector<GBAPalette> palettes;
  std::vector<Assignment> assignments;
  std::unordered_map<BGR15, std::size_t> colorIndexMap;
  std::unordered_map<GBATile, std::size_t> tileIndexes;
  std::vector<CompiledAnimation> anims;

  CompiledTileset()
      : tiles{}, paletteIndexesOfTile{}, palettes{}, assignments{}, colorIndexMap{}, tileIndexes{}, anims{}
  {
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
    this->attributes = tile.attributes;
  }

  [[nodiscard]] bool transparent() const { return palette.size == 1; }

  void setPixel(std::size_t frame, std::size_t row, std::size_t col, uint8_t value)
  {
    if (frame >= frames.size()) {
      throw std::out_of_range{"internal: NormalizedTile::setPixel frame argument out of bounds (" +
                              std::to_string(frame) + " >= " + std::to_string(frames.size()) + ")"};
    }
    if (row >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: NormalizedTile::setPixel row argument out of bounds (" + std::to_string(row) +
                              ")"};
    }
    if (col >= TILE_SIDE_LENGTH) {
      throw std::out_of_range{"internal: NormalizedTile::setPixel col argument out of bounds (" + std::to_string(col) +
                              ")"};
    }
    frames.at(frame).colorIndexes[row * TILE_SIDE_LENGTH + col] = value;
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

enum class AssignAlgorithm { DEPTH_FIRST, BREADTH_FIRST, TRY_ALL };

enum class DecompilerMode { PRIMARY, SECONDARY };

std::string compilerModeString(CompilerMode mode);

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

struct SourcePaths {
  std::string primarySourcePath;
  std::string secondarySourcePath;

  std::filesystem::path bottomPrimaryTilesheetPath() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / "bottom.png";
  }

  std::filesystem::path middlePrimaryTilesheetPath() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / "middle.png";
  }

  std::filesystem::path topPrimaryTilesheetPath() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / "top.png";
  }

  std::filesystem::path bottomSecondaryTilesheetPath() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / "bottom.png";
  }

  std::filesystem::path middleSecondaryTilesheetPath() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / "middle.png";
  }

  std::filesystem::path topSecondaryTilesheetPath() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / "top.png";
  }

  std::filesystem::path primaryAnimPath() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / "anims";
  }

  std::filesystem::path secondaryAnimPath() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / "anims";
  }

  std::filesystem::path primaryAttributesPath() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / "attributes.csv";
  }

  std::filesystem::path secondaryAttributesPath() const
  {
    std::filesystem::path path{secondarySourcePath};
    return path / "attributes.csv";
  }

  std::filesystem::path primaryMetatileBehaviors() const
  {
    std::filesystem::path path{primarySourcePath};
    return path / "metatile_behaviors.h";
  }

  std::filesystem::path modeBasedSrcPath(CompilerMode mode) const;
  std::filesystem::path modeBasedSrcPath(DecompilerMode mode) const;
};

struct Output {
  TilesOutputPalette paletteMode;
  std::string path;

  Output() : paletteMode{TilesOutputPalette::GREYSCALE}, path{} {}
};

struct CompilerConfig {
  CompilerMode mode;
  RGBA32 transparencyColor;
  bool tripleLayer;

  // Palette assignment algorithm configuration
  AssignAlgorithm assignAlgorithm;
  std::size_t exploredNodeCutoff;

  CompilerConfig()
      : mode{}, transparencyColor{RGBA_MAGENTA}, tripleLayer{true}, assignAlgorithm{AssignAlgorithm::DEPTH_FIRST},
        exploredNodeCutoff{4'000'000}
  {
  }
};

struct DecompilerConfig {
  DecompilerMode mode;

  DecompilerConfig() : mode{} {}
};

struct CompilerContext {
  std::unique_ptr<CompiledTileset> pairedPrimaryTileset;
  std::unique_ptr<CompiledTileset> resultTileset;
  std::unordered_map<BGR15, std::tuple<RGBA32, RGBATile, std::size_t, std::size_t>> bgrToRgba;
  std::size_t exploredNodeCounter;

  CompilerContext() : pairedPrimaryTileset{nullptr}, resultTileset{nullptr}, bgrToRgba{}, exploredNodeCounter{} {}
};

} // namespace porytiles

#endif // PORYTILES_TYPES_H
