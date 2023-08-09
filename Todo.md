# Todo List

+ Continue to create more in-file unit tests that maximize coverage
  + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add more `--verbose` logs to stderr

+ swap out png++ library for CImg
  + CImg is much more maintained, it is also header-only and seems to have a clean interface
  + https://github.com/GreycLab/CImg

+ Add a `report` command that prints out various statistics
  + Number of tiles, metatiles, unique colors, etc
  + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
    + calculate this by taking the `Number Unique Colors / Number Slots In Use`
  + Print all animation start tiles
  + dump pal files and tile.png to CLI, see e.g.:
    + https://github.com/eddieantonio/imgcat
    + https://github.com/stefanhaustein/TerminalImageViewer

+ Warnings
  + `-Wpalette-alloc-efficiency`
    + warn user if palette allocation was not 100% efficient
  + `-Wtrue-color-mode`
    + warn on true-color output mode usage since Porymap doesn't support it yet
  + `-Werror` should take an optional argument of specific warnings to activate
    + https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
    + e.g. `-Werror=color-precision-loss`
  + `-Wno-X` support: should warnings support an off switch syntax?
    + would be helpful if someone wants all warnings EXCEPT one

+ Refactor commands / options
  + Use `-f` prefix for frontend configuration
    + e.g. `-fnum-pals-primary`, `-fruby`, `-ftiles-png-pal-mode`
  + `-fskip-metatile-generation` skips generation of `metatiles.bin`
  + `compile-freestanding` mode
    + `takes a raw tilesheet (no layers), generates `tiles.png` and freestanding palette files
    + freestanding palette files aren't in a `palettes` folder, aren't named according to 00.pal
    + so it would just make pal1.txt, pal2.txt, pal3.txt, ... , empty.txt where empty.txt is an empty palette for convenience

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

+ `dump-anim-code` command
  + takes input tiles just like `compile-X` commands
  + instead of outputting all the files, just write C code to the console
  + the C code should be copy-paste-able into `tileset_anims.h/c` and `src/data/tilesets/headers.h`
  + is it possible to generate the code and insert it automatically?

+ Set up more CI builds
  + Windows MSVC?
  + set up package caches so installs don't have to run every time
  + universal MacOS binary?
    + https://stackoverflow.com/questions/67945226/how-to-build-an-intel-binary-on-an-m1-mac-from-the-command-line-with-the-standar
  + dist scripts that setup a folder for easy github upload, auto set compiler flags as necessary
    + https://releases.llvm.org/16.0.0/projects/libcxx/docs/UsingLibcxx.html
    + https://stackoverflow.com/questions/2579576/i-dir-vs-isystem-dir

+ provide a way to input primer tiles to improve algorithm efficiency

+ Set up auto-generated documentation: doxygen? RTD?

+ Decompile compiled tilesets back into three layer PNGs + animated sets

+ Proper support for dual layer tiles (requires modifying metatile attributes file)
  + input to compile can include `metatile_attributes.csv`, which defines attributes for each metatile
  + then just generate `metatile_attributes.bin` from this CSV
  + will probably need to do some basic C parsing to allow for macro expansion
  + maybe a `generate-attributes-template` command to generate a template CSV based on layer inputs?

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)

+ Support .ora files (which are just fancy zip files) since GIMP can export layers as .ora
  + https://github.com/tfussell/miniz-cpp
