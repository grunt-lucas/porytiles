# Porytiles 1.x

## What Is Porytiles 1.x?
This folder is the home of legacy Porytiles 1.x. Currently, the [Porytiles Releases Tab](https://github.com/grunt-lucas/porytiles/releases) contains builds from the code here in the Porytiles 1.x folder. This will be the supported version of Porytiles for the forseeable future. It's very usable in its current state, and there is significant documentation [over at the wiki](https://github.com/grunt-lucas/porytiles/wiki) to get you started. There will be occasional tweaks and bugfixes, which should show up in the [Porytiles Releases Tab](https://github.com/grunt-lucas/porytiles/releases) as nightlies. Check back occasionally and always download the latest version. However, most large new features (and some bugs too) will not be fixed in this version. Rather, they will be fixed in an upcoming Porytiles 2.x redux. More on that below.

## Why?
Instead of releasing a 1.0.0 Porytiles, I have instead decided to start working on a from-the-ground-up refactor of Porytiles, which will be known as Porytiles 2.x until it officially releases. The reason for this: the process of rapid iterative development on Porytiles 1.x has accrued significant technical debt. At the moment, the Porytiles 1.x code is so messy that I am having trouble adding features or fixing bugs without introducing further issues and gotchas. Additionally, since starting Porytiles in 2023 as a C++ exploratory project, I have learned much about C++ best practices as well as open source app development. As such, building Porytiles 2.x "from scratch" will give it the best foundation for a bright future. Porytiles 2.x will begin as a feature-for-feature port. Once it has parity with Porytiles 1.x functionality, I will begin adding some of the new features discussed below.

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

## Tracking 2.x Progress
In order to avoid triggering tons of irrelevant nightly builds, 2.x development will begin on a new branch called [`2.x/develop`](https://github.com/grunt-lucas/porytiles/tree/2.x/develop). Feel free to track progress and try out 2.x as it develops by pulling from this branch and building the project. I will keep this 2.x branch in sync with regular [`develop`]((https://github.com/grunt-lucas/porytiles/tree/develop)), and I may at certain points create preview 2.x releases. Please star and watch the repo if you want regular updates!
