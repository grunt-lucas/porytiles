# Todo List

+ Continue to create more in-file unit tests that maximize coverage
  + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add more `--verbose` logs to stderr

+ swap out png++ library for CImg
  + CImg is much more maintained, it is also header-only and seems to have a clean interface
  + https://github.com/GreycLab/CImg

+ `decompile` command that takes a compiled tileset and turns it back into Porytiles-compatible sources

+ `report` command that prints out various statistics
  + Number of tiles, metatiles, unique colors, etc
  + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
    + calculate this by taking the `Number Unique Colors / Number Slots In Use`
  + Print all animation start tiles
  + dump pal files and tile.png to CLI, see e.g.:
    + https://github.com/eddieantonio/imgcat
    + https://github.com/stefanhaustein/TerminalImageViewer

+ `dump-anim-code` command
  + takes input tiles just like `compile-X` commands
  + instead of outputting all the files, just write C code to the console
  + the C code should be copy-paste-able into `tileset_anims.h/c` and `src/data/tilesets/headers.h`
  + is it possible to generate the code and insert it automatically?

+ `freestanding` mode for compilation
  + freestanding mode would allow input PNG of any dimension, would only generate a tiles.png and pal files
  + might be useful for some people who drew a scene they want to tile-ize
  + low-priority feature

+ Warnings
  + `-Wpalette-alloc-efficiency`
    + warn user if palette allocation was not 100% efficient
  + the entire warning parsing system is a hot flaming dumpster fire mess, fix it somehow?
    + https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
  + warnings-as-errors shouldn't bail until the very end, is there an easy way to do this?

+ Refactor CLI parsing
  + many commands share the same set of options, or slight variations

+ Options changes
  + `-manage-metatiles-with-porymap` skips generation of `metatiles.bin` and `metatile_attributes.bin`
  + `-palette-assign-cutoff-threshold` (or another suitable name) to change the call limit for the palette assign algo
  + force long options to use `--`?
    + rework warning options so `W` is a short option, so `-Wall`, `-Wfoobar`, `-Werror` still work?

+ Set up more CI builds
  + Windows MSVC? MinGW?
  + set up package caches so installs don't have to run every time
    + probably too hard to do with homebrew
  + finish the script targets in package.sh
    + https://releases.llvm.org/16.0.0/projects/libcxx/docs/UsingLibcxx.html
    + https://stackoverflow.com/questions/2579576/i-dir-vs-isystem-dir
    + MacOS:
      CXXFLAGS="-isystem /opt/homebrew/opt/llvm@16/include/c++/v1" LDFLAGS="-L/opt/homebrew/opt/llvm@16/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm@16/lib/c++ -lc++"
    + do we want separate targets for both libc++ and libstdc++?
  + better build system? (cmake, autotools, etc)

+ provide a way to input primer tiles to improve algorithm efficiency

+ Set up auto-generated documentation: doxygen? RTD?

+ support custom masks and shifts for metatile attributes, see how Porymap does this

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)

+ Support .ora files (which are just fancy zip files) since GIMP can export layers as .ora
  + https://github.com/tfussell/miniz-cpp
