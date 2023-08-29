# 1.0.0 Notes

## Key Frame System
+ animation key frame system: see note in `res/tests/anim_metatiles_shared_tiles/note.txt`
  + a better way to do representative tiles
  + may allow to properly decompile vanilla tilesets
  + will also solve problem of some animated tiles sharing content
    + anim_yellow_flower and anim_white_flower share some representative tiles, which means you can't have both in
      the same tileset
  + basically, each anim has a `key.png` key frame, this key frame must contain unique tiles, the key frame is used in
    the layer PNGs to tell Porytiles that we want an anim tile at that location
  + the nice thing about a dedicated key frame is that it can literally be anything the user wants, since it is never
    actually displayed in-game, obviously the most natural choice is just to have it match `0.png`
  + the decompile command can then take a sequence of args like 'yellow_flower=12' where 12 is the tile index for the
    start of the key frame sequence for 'yellow_flower' anim, this solves the problem of vanilla tilesets using key
    frames that don't actually appear anywhere in the anim, and decompile can warn the user if it decomps an animation
    that doesn't have any frames present in the tiles (which indicates the user will need to manually provide the
    location of the keyframes for that anim, e.g. the water tiles in the vanilla tilesets)

## -Otile-sharing
+ how do we impl this?
