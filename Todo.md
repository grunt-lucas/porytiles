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
  + `-Werror` should take an optional argument of specific warnings to activate
    + https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
    + e.g. `-Werror=color-precision-loss`
  + `-Wno-X` support: should warnings support an off switch syntax?
    + would be helpful if someone wants all warnings EXCEPT one

+ Refactor commands / options
  + `-skip-metatile-generation` skips generation of `metatiles.bin`
  + `-max-recurse-count` (or another suitable name) to change the call limit for the palette assign algo

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
    + MacOS:
      CXXFLAGS="-isystem /opt/homebrew/opt/llvm@16/include/c++/v1" LDFLAGS="-L/opt/homebrew/opt/llvm@16/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@16/lib/c++ -lc++ -lc++abi"
  + better build system? (cmake, autotools, etc)

+ provide a way to input primer tiles to improve algorithm efficiency

+ Set up auto-generated documentation: doxygen? RTD?

+ Decompile compiled tilesets back into three layer PNGs + animated sets

+ Proper support for dual layer tiles (requires modifying metatile attributes file)
  + input to compile can include `metatile_attributes.csv`, which defines attributes for each metatile
  + then just generate `metatile_attributes.bin` from this CSV
  + will probably need to do some basic C parsing to allow for macro expansion
  + the file should be sparse, that is, any unspecified metatile just receives default values
  + CSV parsing header-only: https://github.com/ben-strasser/fast-cpp-csv-parser

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)

+ Support .ora files (which are just fancy zip files) since GIMP can export layers as .ora
  + https://github.com/tfussell/miniz-cpp

+ Wiki page ideas
  + How to integrate into the pokeemerald Makefile
