# Porytiles 2.x

## What Is Porytiles 2.x?
Porytiles 2.x is a from-the-ground-up refactor of Porytiles, now written in Go.

## What Happened To Porytiles 2.x in C++?
<sub><sup>I'm borderline ADHD and bored of C++ for the moment...</sup></sub>

Nothing! I just decided that working in Go will be better for the long-term viability of the project. [Poryscript](https://github.com/huderlem/poryscript), another popular community tool, is written in Go, and I'd like to foster around Porytiles a community of contribution much like Poryscript's. I figure that writing Porytiles in a non-arcane language the community is already using may help that effort. Also, since Go is overall an easier language to use, it will be easier for me to maintain Porytiles long-term. I have found it is difficult to dive back into a complex C++ codebase when I've been on hiatus (which I often am). The actual goals of Porytiles 2.x have not changed, and are listed below.

## Porytiles 2.x Goals & Highlights
+ Modern, clean Go codebase
+ Fixes for many of the pesky bugs listed in the [Issues](https://github.com/grunt-lucas/porytiles/issues) bank
+ Support for completely custom metatile attributes, like Porymap
+ Better support for freestanding builds, i.e. builds that don't need to generate metatiles
+ Support for map-based builds, i.e. draw a layered map and Porytiles will actually generate your three deduped metatile layer PNGs for you
+ Smarter palette assignment using more sophisticated bin-packing solutions
+ An eventual GUI client
+ Automatic generation of animation C driver code
+ Tons more useful warnings and diagnostics
+ Shell TAB completion for bash, zsh, and fish
+ Animation decompilation support
+ And much more!
