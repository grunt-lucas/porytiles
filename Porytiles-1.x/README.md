# Porytiles 1.x

## What Is Porytiles 1.x?
This folder is the home of legacy Porytiles 1.x. Currently, the [Porytiles Releases Tab](https://github.com/grunt-lucas/porytiles/releases) contains builds from the code here in the Porytiles 1.x folder. This will be the supported version of Porytiles for the forseeable future. However, most new features (and some bugs too) will not be fixed in this version. Rather, they will be fixed in an upcoming Porytiles 2.x redux. More on that below.

## Why?
Instead of releasing a 1.0.0 Porytiles, I have instead decided to start working on a from-the-ground-up refactor of Porytiles, which will be known as Porytiles 2.x until it officially releases. The reason for this: the process of rapid iterative development on Porytiles 1.x has accrued significant technical debt. At the moment, the Porytiles 1.x code is so messy that I am having trouble adding features or fixing bugs without introducing further issues and gotchas. Additionally, since starting Porytiles in 2023 as a C++ exploratory project, I have learned much about C++ best practices as well as open source app development. As such, building Porytiles 2.x "from scratch" will give it the best foundation for a bright future.

## Porytiles 2.x Goals & Highlights
+ Modern, clean C++20 codebase based around domain-driven design
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
