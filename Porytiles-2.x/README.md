# Porytiles 2.x

## What Is Porytiles 2.x?
Porytiles 2.x is a from-the-ground-up refactor of Porytiles.

## Porytiles 2.x Goals & Highlights
+ Modern, clean C++20 codebase inspired by concepts from domain-driven design
+ Modern build system and project structure, designed around CMake from the ground up
+ Fixes for many of the pesky bugs listed in the [Issues](https://github.com/grunt-lucas/porytiles/issues) bank
+ Support for completely custom metatile attributes, like Porymap
+ Better support for freestanding builds, i.e. builds that don't need to generate metatiles
+ Modern internal libraries, e.g. `CImg` instead of `png++`
+ Support for map-based builds, i.e. draw a layered map and Porytiles will actually generate your three deduped metatile layer PNGs for you
+ Better palette assignment using more sophisticated bin-packing solutions
+ A GUI client using `Dear ImGui` library
+ Automatic generation of animation C driver code
+ Tons more useful warnings and diagnostics
+ Shell TAB completion for bash, zsh, and fish
+ Animation decompilation support in `decompile-{primary,secondary}` commands
+ And much more!
