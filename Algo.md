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
+ Pre-compile the animated tiles
  + For each anim group, iterate over the available palettes and assign all colors in the group to the same palette
  + Make GBATiles for frame 0 (i.e. `0.png`), these tiles will go at the top of the tilesheet
  + Make GBATiles for the other frames, these will be saved as indexed PNGs into the output directory
