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

+ attributes / behavior changes
  + allow user to specify default behavior value using CLI option `-default-behavior`
    + this option takes a string defined in the behavior header
  + also have `-default-terrain-type` and `-default-encounter-type`

+ More assign algorithms?
  + Maybe some kind of A-star?
    + https://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
    + https://realtimecollisiondetection.net/blog/?p=56
  + More heuristics to prune unpromising branches
  + Assignment config discovery
    + default behavior: look for `assign.cfg` in the input folder, use those settings
      + format of file: key=value lines, e.g.
      ```
      assign-explore-cutoff=3
      best-branches=2
      assign-algo=bfs
      ```
    + if assign.cfg does not exist, warn the user and run the full assign param search matrix
      + this will be a default-on warning, `-Wassign-config-not-found`
    + user can supply --cache-assign-config option to force porytiles to:
      1. Ignore the values in `assign.cfg`, if present
      2. Run the full assign param search matrix
      3. If a valid param setting is found, overwrite `assign.cfg` with these new settings

+ `report` command that prints out various statistics
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
  + `-Wno-transparency-present`
    + warn user if the tileset did not have any of the selected transparency color
    + this is a common mistake if user decompiles a vanilla tileset that uses a different transparency color
      + user may forget to set `-transparency-color` which will break compilation
  + the entire warning parsing system is a hot flaming dumpster fire mess, fix it somehow?
    + https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
  + warnings-as-errors shouldn't bail until the very end, is there an easy way to do this?

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)
  + canonical example: Pokémart and Pokécenter roofs are identical tiles with different colorings
    + ideally, two palettes will be color-aligned such that we only have to save a single indexed roof tile

+ Improve CI build system
  + Add Windows MSVC? MinGW?
  + set up package caches so installs don't have to run every time
    + probably too hard to do with homebrew, apt
  + finish the script targets in package.sh
    + https://releases.llvm.org/16.0.0/projects/libcxx/docs/UsingLibcxx.html
    + https://stackoverflow.com/questions/2579576/i-dir-vs-isystem-dir
    + Github CI should call these scripts
  + static link all libs for the Linux version, do not provide a dynamically linked release binary
    + if users want one, they can build from source
  + static link libc++ and libpng on mac?
    + https://stackoverflow.com/questions/844819/how-to-static-link-on-os-x
    + probably too brittle (MacOS syscall interface is not stable), instead provide detailed install instructions
  + Mac universal binary?
    + https://stackoverflow.com/questions/67945226/how-to-build-an-intel-binary-on-an-m1-mac-from-the-command-line-with-the-standar
  + better build system? (cmake, autotools, etc)
  + static analysis: https://nrk.neocities.org/articles/c-static-analyzers

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
