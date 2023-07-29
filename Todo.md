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

+ Refactor commands / options
  + Use `-f` prefix for frontend configuration
    + e.g. `-fnum-pals-primary`, `-fruby`, `-ftiles-png-pal-mode`
  + Delete `compile-raw`, use `-ffreestanding` instead
  + `compile-primary` and `compile-secondary` should be separate commands

+ Set up more CI builds
  + Windows MSVC?
  + set up package caches so installs don't have to run every time
  + universal MacOS binary?
    + https://stackoverflow.com/questions/67945226/how-to-build-an-intel-binary-on-an-m1-mac-from-the-command-line-with-the-standar

+ Setup some kind of clang tidy script:
```
/opt/homebrew/opt/llvm/bin/clang-tidy -checks='cert-*' -header-filter='.*' --warnings-as-errors='*' src/*.cpp -- --std=c++20 -Iinclude $(pkg-config --cflags libpng) -Idoctest-2.4.11 -Ipng++-0.2.9
```

+ Setup some kind of clang format script

+ Add `--verbose` logs to stderr

+ Support for animated tiles, i.e. stable placement in `tiles.png`

+ Decompile compiled tilesets back into three layer PNGs + animated sets

+ Detect and exploit opportunities for tile-sharing to reduce size of `tiles.png`
  + hide this behind an optimization flag, `-Otile-sharing` (will make it easier to test)

+ Proper support for dual layer tiles (requires modifying metatile attributes file)

+ Support .ora files (which are just fancy zip files) since GIMP can export layers as .ora
  + https://github.com/tfussell/miniz-cpp
