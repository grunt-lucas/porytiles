# Todo List

+ Continue to create more in-file unit tests that maximize coverage
  + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add a `--report` option that prints out various statistics
  + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
    + calculate this by taking the `Number Unique Colors / Number Slots In Use`
  + dump pal files and tile.png to CLI using: https://github.com/eddieantonio/imgcat

+ Warnings
  + `-Wcolor-precision-loss` / `-Wno-color-precision-loss`
    + warn user on two RGB colors that will collapse into a single BGR color
  + `-Wpalette-alloc-efficiency` / `-Wno-palette-alloc-efficiency`
    + warn user if palette allocation was not 100% efficient
  + `-Wall`: enable all warnings
  + `-Werror`: warnings become errors
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

+ Set up more CI builds
  + Windows MSVC?
  + set up package caches so installs don't have to run every time
  + universal MacOS binary?
    + https://stackoverflow.com/questions/67945226/how-to-build-an-intel-binary-on-an-m1-mac-from-the-command-line-with-the-standar

+ Add `--verbose` logs to stderr

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
