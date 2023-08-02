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
  std::vector<Animation> anims;
};

void importAnimTiles(const std::vector<std::vector<png::image<png::rgba_pixel>>>& rawAnims, DecompiledTileset& tiles) {
  std::vector<Animation> anims;

  // Read all the anims from rawAnims

  tiles.anims = anims;
}

// Compile logic

```

+ Pre-process the animated tiles
  + each corresponding tile in each frame combined into an AnimNormalizedTile
    + anim normalized tile palette contains all the colors from that tile in each frame
    + at the end, push layer 0 of the animated tiles onto the tilesheet first
  + Make GBATiles for frame 0 (i.e. `0.png`), these tiles will go at the top of the tilesheet
  + Make GBATiles for the other frames, these will be saved as indexed PNGs into the output directory
