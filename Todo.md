# Todo List

## 1.0.0 Release Checklist

+ Check all in-code TODOs, FIXMEs, FEATUREs, use todo.sh to find them
  + Some may need to be addressed before 1.0.0

+ Continue to create more in-file unit tests that maximize coverage

+ get secondary tileset `decompile` command working

+ Warnings
  + `-Wpalette-alloc-efficiency`
    + warn user if palette allocation was not 100% efficient
  + `-Wno-transparency-present`
    + warn user if the tileset did not have any of the selected transparency color
    + this is a common mistake if user decompiles a vanilla tileset that uses a different transparency color
      + user may forget to set `-transparency-color` which will break compilation

+ `-disable-metatile-generation` skips generation of `metatiles.bin`
+ `-disable-attribute-generation` skips generation of `metatile_attributes.bin`

+ `-print-report` option that prints out various statistics
  + Number of tiles, metatiles, unique colors, etc
  + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
    + calculate this by taking the `Number Unique Colors / Number Slots In Use`
  + Print all animation start tiles
  + dump pal files and tile.png to CLI, see e.g.:
    + https://github.com/eddieantonio/imgcat
    + https://github.com/stefanhaustein/TerminalImageViewer

+ Finish the wiki pages

+ Create a basic YouTube tutorial series

## Post 1.0.0 Ideas

+ in-code TODOs, FIXMEs, FEATUREs, use todo.sh to find them
  + Some of these can wait for post 1.0.0

+ Reported test case failure with clang 14.0.0 on Ubuntu
  + https://discord.com/channels/@me/1148327134343995453/1149142257744748636

+ Continue to create more in-file unit tests that maximize coverage
  + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add more `--verbose` logs to stderr

+ swap out png++ library for CImg
  + CImg is much more maintained, it is also header-only and seems to have a clean interface
  + https://github.com/GreycLab/CImg

+ get `decompile` command to successfully decompile animations
  + will require key frame detection / key frame hints at CLI?

+ More assign algorithms?
  + Maybe some kind of A-star?
    + https://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
    + https://realtimecollisiondetection.net/blog/?p=56
  + More heuristics to prune unpromising branches

+ `palette-overrides` folder in the input folder
  + in this folder, numbered JASC PAL files containing exactly 16 colors are copied directly into the final palette
    + e.g. 1.pal will become palette 1
    + fail build if we can't assign all tiles given the override
    + user can put a `-` on a line to indicate that this could be any color
    + the primary use case for this would be so that users could:
      + e.g. fix a given color slot in a given palette, sometimes this is nice for certain DNS systems, etc

+ Warnings
  + the entire warning parsing system is a hot flaming dumpster fire mess, fix it somehow?
    + https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
  + warnings-as-errors shouldn't bail until the very end, is there an easy way to do this?
  + clang/gcc will show "And 100 similar warnings..." if there are too many warnings, something like this

+ Use imgui to create a basic GUI?
  + https://github.com/ocornut/imgui

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)
  + canonical example: Pokémart and Pokécenter roofs are identical tiles with different colorings
    + ideally, two palettes will be color-aligned such that we only have to save a single indexed roof tile

+ Improve CI build system
  + Add Windows MSVC? MinGW?
    + Apparently it is possible to build a cross-compiled version with minGW on a macbook:
    ```
    brew install mingw-w64
    cd porytiles
    make clean && CC="x86_64-w64-mingw32-gcc" CXX="x86_64-w64-mingw32-g++" make
    ```
    + you can extract this and steal the libpng.a file https://packages.msys2.org/package/mingw-w64-x86_64-libpng
  + set up package caches so installs don't have to run every time
    + probably too hard to do with homebrew, apt
  + static link libc++ and libpng on mac?
    + https://stackoverflow.com/questions/844819/how-to-static-link-on-os-x
    + probably too brittle (MacOS syscall interface is not stable), instead provide detailed install instructions
  + Mac universal binary?
    + https://stackoverflow.com/questions/67945226/how-to-build-an-intel-binary-on-an-m1-mac-from-the-command-line-with-the-standar
  + better build system? (cmake, autotools, etc)
  + static analysis: https://nrk.neocities.org/articles/c-static-analyzers

+ `-dump-anim-code` option
  + instead of outputting all the files, just write C code to the console
  + the C code should be copy-paste-able into `tileset_anims.h/c` and `src/data/tilesets/headers.h`
  + is it possible to generate the code and insert it automatically? This would be even better. Would need some kind of
    syntax tree parsing.

+ `freestanding` mode for compilation
  + freestanding mode would allow input PNG of any dimension, would only generate a tiles.png and pal files
  + might be useful for some people who drew a scene they want to tile-ize
  + low-priority feature

+ `generate-metailes` mode
  + input PNGs of any dimension, e.g. users can "draw" the map the way they want it to look
  + instead of outputting files for Porymap, output a top, middle, bottom PNG that can then be fed thru `compile-primary`
    to get the Porymap files

+ Set up auto-generated documentation: doxygen? RTD?

+ support custom masks and shifts for metatile attributes, see how Porymap does this

+ Refactor CLI parsing, it's a mess
  + CXXOpts lib may be helpful here
