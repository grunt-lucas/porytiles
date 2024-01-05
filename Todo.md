# Todo List

+ in-code TODOs, FIXMEs, FEATUREs, use todo.sh to find them

+ Reported test case failure with clang 14.0.0 on Ubuntu
  + https://discord.com/channels/@me/1148327134343995453/1149142257744748636

+ Continue to create more in-file unit tests that maximize coverage
  + https://github.com/doctest/doctest/tree/master (progress on this is going well)

+ Add more `--verbose` logs to stderr

+ swap out png++ library for CImg
  + CImg is much more maintained, it is also header-only and seems to have a clean interface
  + https://github.com/GreycLab/CImg

+ `decompile` command that takes a compiled tileset and turns it back into Porytiles-compatible sources
  + get secondary tileset decompilation working
  + get anim decompilation working
    + will require key frame detection / key frame hints at CLI

+ More assign algorithms?
  + Maybe some kind of A-star?
    + https://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
    + https://realtimecollisiondetection.net/blog/?p=56
  + More heuristics to prune unpromising branches
  + Assignment config discovery
    ~~~
    + default behavior: look for `assign.cfg` in the input folder, use those settings
      + format of file: key=value lines, e.g.
      ```
      assign-explore-cutoff=3
      best-branches=2
      assign-algo=bfs
      ```
    ~~~ this feature is now impl'd
    + if those settings fail, warn the user (non-toggleable warning) and re-run the param search matrix
    + if assign.cfg does not exist, warn the user and run the full assign param search matrix
      + this will be a default-on warning, `-Wassign-config-not-found`
      + if the search matrix fails, error out and print a link to a helpful wiki page
    + How to handle the manual override options?
      + Providing any manual override will force Porytiles to work in manual mode. It will ignore `assign.cfg` entirely,
        and use the manual CLI values, or defaults if a value is not supplied.
      + I want to move away from the idea that users should be manually tweaking the assignment config settings
      + If the param search matrix fails, users can try manual options to see if something works. If something does,
        it will automatically save off an `assign.cfg`. Basically, Porytiles should always automatically write out an
        `assign.cfg` any time a successful compilation occurs. In most cases, it will write out an identical file so
        nothing will change.

+ provide a way to "prime" palette assignment to improve algorithm efficiency?
  + idea: two different palette override modes
  + `palette-overrides` folder in the input folder
    + in this folder, numbered JASC PAL files containing exactly 16 colors are copied directly into the final palette
      + e.g. 1.pal will become palette 1
      + fail build if we can't assign all tiles given the override
  + `palette-primers`
    + in this folder, named JASC PAL files that contain an arbitrary number of colors
      + e.g. grass.pal
      + turn each palette file here into a dummy normalized tile so each pal file is guaranteed to appear in the same
        hardware palette, gives users a way to guarantee that certain colors are always together
      + when done correctly, this will help the algorithm find a more optimal solution by "leeching" intelligence from
        human intervention

+ Warnings
  + `-Wpalette-alloc-efficiency`
    + warn user if palette allocation was not 100% efficient
  + `-Wno-transparency-present`
    + warn user if the tileset did not have any of the selected transparency color
    + this is a common mistake if user decompiles a vanilla tileset that uses a different transparency color
      + user may forget to set `-transparency-color` which will break compilation
  + the entire warning parsing system is a hot flaming dumpster fire mess, fix it somehow?
    + https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
  + warnings-as-errors shouldn't bail until the very end, is there an easy way to do this?

+ Use imgui to create a basic GUI?
  + https://github.com/ocornut/imgui

+ `report` command that prints out various statistics
  + Number of tiles, metatiles, unique colors, etc
  + Palette efficiency in colors-per-palette-slot: a value of 1 means we did a perfect allocation
    + calculate this by taking the `Number Unique Colors / Number Slots In Use`
  + Print all animation start tiles
  + dump pal files and tile.png to CLI, see e.g.:
    + https://github.com/eddieantonio/imgcat
    + https://github.com/stefanhaustein/TerminalImageViewer

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
  + test if the actual scripted release process works properly
  + static link libc++ and libpng on mac?
    + https://stackoverflow.com/questions/844819/how-to-static-link-on-os-x
    + probably too brittle (MacOS syscall interface is not stable), instead provide detailed install instructions
  + Mac universal binary?
    + https://stackoverflow.com/questions/67945226/how-to-build-an-intel-binary-on-an-m1-mac-from-the-command-line-with-the-standar
  + better build system? (cmake, autotools, etc)
  + static analysis: https://nrk.neocities.org/articles/c-static-analyzers

+ `dump-anim-code` command
  + takes input tiles just like `compile-X` commands
  + instead of outputting all the files, just write C code to the console
  + the C code should be copy-paste-able into `tileset_anims.h/c` and `src/data/tilesets/headers.h`
  + is it possible to generate the code and insert it automatically?

+ `freestanding` mode for compilation
  + freestanding mode would allow input PNG of any dimension, would only generate a tiles.png and pal files
  + might be useful for some people who drew a scene they want to tile-ize
  + low-priority feature
  + `-skip-metatile-generation` skips generation of `metatiles.bin`
  + `-skip-attribute-generation` skips generation of `metatile_attributes.bin`

+ Set up auto-generated documentation: doxygen? RTD?

+ support custom masks and shifts for metatile attributes, see how Porymap does this

+ Refactor CLI parsing, it's a mess
  + CXXOpts lib may be helpful here

+ Support .ora files (which are just fancy zip files) since GIMP can export layers as .ora
  + https://github.com/tfussell/miniz-cpp
