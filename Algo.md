# 1.0.0 Notes

## Animated Tile Support

+ Input format: directory structure:
+ `anims`
  + `flower`
    + `0.png`
    + `1.png`
    + `2.png`
    + etc.
  + `sand`
    + `0.png`
    + `1.png`
    + `2.png`
    + etc.
  + etc.

```C++
struct AnimFrame {
  std::vector<RGBATile> tiles;
}

struct Animation {
  std::vector<AnimFrame> frames;

   AnimFrame representativeFrame() const {
    return frames.at(0);
   }
}

struct DecompiledTileset {
  std::vector<RGBATile> tiles;

  // new field: anims
  // this field holds the decompiled anim data from the optionally supplied
  // anims folder. Each anim is essentially a vector of frames, where each frame is a vector of RGBATiles representing
  // that frame. The compiler will ultimately copy frame 0 into the start of VRAM. Users can "use" an animated tile
  // by simply painting a frame 0 tile onto the RGBA metatile sheet. The compiler will automatically link it to one of
  // the anim tiles at the start of tiles.png
  std::vector<Animation> anims;
};

struct CompiledTileset {
  std::vector<GBATile> tiles;
  std::vector<std::size_t> paletteIndexesOfTile;
  std::vector<GBAPalette> palettes;
  std::vector<Assignment> assignments;
  std::unordered_map<BGR15, std::size_t> colorIndexMap;
  std::unordered_map<GBATile, std::size_t> tileIndexes;

  // new field:
  
};

void importAnimTiles(const std::vector<std::vector<png::image<png::rgba_pixel>>>& rawAnims, DecompiledTileset& tiles) {
  std::vector<Animation> anims;

  // Read all the anims from rawAnims
  // HOW TO READ SORTED FILES
  // std::vector<std::filesystem::path> files_in_directory;
  // std::copy(std::filesystem::directory_iterator(myFolder), std::filesystem::directory_iterator(), std::back_inserter(files_in_directory));
  // std::sort(files_in_directory.begin(), files_in_directory.end());

  // for (const std::string& filename : files_in_directory) {
  //     std::cout << filename << std::endl; // printed in alphabetical order
  // }

  tiles.anims = anims;
}

// Compile logic
// DecompiledIndex to become a custom type, will see why in a second
struct DecompiledIndex {
  bool animated;
  std::size_t animIndex;
  std::size_t frameIndex;
  std::size_t tileIndex;
}
using IndexedNormTile = std::pair<DecompiledIndex, porytiles::NormalizedTile>;

struct NormalizedTile {
  /*
   * Vector here to represent layers. Animated tiles can have multiple layers, with each layer corresponding to
   * a frame in the animation. Regular tiles will have a size 1 vector, since they don't have layers.
   */
  std::vector<NormalizedPixels> pixels;
  /*
   * This palette is the combined palette for all layers. We want to handle it this way, since for anim tiles, each
   * frame in the animation must be placed in the same hardware palette.
   */
  NormalizedPalette palette;
  bool hFlip;
  bool vFlip;
}

void compile () {
  // other setup ...

  /*
   * Modify this function so it first reads the Animations from the DecompTileset. Regular tiles will just set the index
   * field to their index in the iteration order, but with animated set to false. Animated tiles will set animated to
   * true, and set the animIndex and frameIndex fields accordingly. For animated tiles, the tileIndex field refers to
   * the index in that particular frame.
   */
  std::vector<IndexedNormTile> indexedNormTiles =
      normalizeDecompTiles(ctx.compilerConfig.transparencyColor, decompiledTileset);

  // logic for buildColorIndexMaps is identical, it does not care about the DecompiledIndex field

  // logic for matchNormalizedWithColorSets is also the same

  // palette assignment and hardware palette construction are also the same

  // the assignTiles logic will need to be updated to handle the animated tiles correctly
  // A first loop should iterate over just the animated=true tiles
  // makeTile() on each tile: frame 0 tiles get pushed to the tiles map so the regular assignments can reference
  // all frames get pushed to the animations field in the compiled set so the emitter can emit the frames as GBATiles
}
```

+ Pre-process the animated tiles
  + each corresponding tile in each frame combined into an AnimNormalizedTile
    + anim normalized tile palette contains all the colors from that tile in each frame
    + at the end, push layer 0 of the animated tiles onto the tilesheet first
  + Make GBATiles for frame 0 (i.e. `0.png`), these tiles will go at the top of the tilesheet
  + Make GBATiles for the other frames, these will be saved as indexed PNGs into the output directory
