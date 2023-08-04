# Todo List

+ Continue to create more in-file unit tests that maximize coverage
  + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add a `report` command that prints out various statistics
  + Number of tiles, metatiles, unique colors, etc
  + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
    + calculate this by taking the `Number Unique Colors / Number Slots In Use`
  + Print all animation start tiles
  + dump pal files and tile.png to CLI, see e.g.:
    + https://github.com/eddieantonio/imgcat
    + https://github.com/stefanhaustein/TerminalImageViewer
    + 

+ Warnings
  + `-Wcolor-precision-loss` / `-Wno-color-precision-loss`
    + warn user on two RGB colors that will collapse into a single BGR color
  + `-Wpalette-alloc-efficiency` / `-Wno-palette-alloc-efficiency`
    + warn user if palette allocation was not 100% efficient
  + `-Wall`: enable all warnings
  + `-Werror`: warnings become errors
    + https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
    + `-Werror=color-precision-loss`

+ Refactor commands / options
  + Use `-f` prefix for frontend configuration
    + e.g. `-fnum-pals-primary`, `-fruby`, `-ftiles-png-pal-mode`
  + `compile` command has three modes
    + `compile freestanding`: takes a raw tilesheet (no layers), generates `tiles.png` and freestanding palette files
      + freestanding palette files aren't in a `palettes` folder, aren't named according to 00.pal
      + so it would just make pal1.txt, pal2.txt, pal3.txt, ... , empty.txt where empty.txt is an empty palette for convenience
    + `compile primary`: compiles a primary tileset
    + `compile secondary`: compiles a secondary tileset, takes path to primary folder
  + `-fskip-metatile-generation` skips generation of `metatiles.bin`
  + compile should take path to folder as input, folder must contain a `bottom,middle,top.png`, `anim` folder with
    frames in expected format, and metatile_attributes.csv file, etc

+ directory input
  + metatile_attributes.csv which contains attributes for each metatile
    + this will allow us to support dual layers
    + will also be better because it means all files in a given tileset folder will now be managed by porytiles,
      which will prevent possible corruption from porymap

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

+ `anim_flower_white` and `anim_flower_yellow` in `res/tests` do not have true magenta backgrounds, fix this

+ Add `--verbose` logs to stderr

+ Set up auto-generated documentation: doxygen? RTD?

+ Support for animated tiles, i.e. stable placement in `tiles.png`

+ Decompile compiled tilesets back into three layer PNGs + animated sets

+ Proper support for dual layer tiles (requires modifying metatile attributes file)
  + input to compile can include `metatile_attributes.csv`, which defines attributes for each metatile
  + then just generate `metatile_attributes.bin` from this CSV
  + will probably need to do some basic C parsing to allow for macro expansion

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)

+ Support .ora files (which are just fancy zip files) since GIMP can export layers as .ora
  + https://github.com/tfussell/miniz-cpp
