# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `decompile-secondary` command to decompile secondary tilesets

- `-normalize-transparency` option for the decompile commands

- `-Wtile-index-out-of-range` for use with the decompile commands

- `-Wpalette-index-out-of-range` for use with the decompile commands

- Options to disable generation of `metatiles.bin` and `metatile_attributes.bin`
  - `-disable-metatile-generation` and `-disable-attribute-generation`

- Added support for `-best-branches=smart` pal assignment mode, which prunes `populated + 1` number of branches per vertex
   - assign config search matrix now tries smart prune before trying a constant prune

### Changed

- Fixed bug from issue [Secondary tileset attributes aren't generated #1](https://github.com/grunt-lucas/porytiles/issues/1)

- `-Wkey-frame-missing-assignment` now called `-Wkey-frame-no-matching-tile`

- Subcommand help menus are much more refined, they now only show info relevant to the given subcommand

## [0.0.7] - 2024-01-07

### Added

- Palette assignment parameter search matrix: users no longer have to fiddle with a bunch of annoying options to find
  something that builds. It should work automatically in the background. Some additional options related to this
  functionality have been added.

- Added a `default-behavior` option to specify default metatile behavior for missing `attributes.csv` entries

- Also added `default-terrain-type` and `default-encounter-type` options

- Manual palette assignment options now have a `primary-` version for use with `compile-secondary`

### Changed

- Updated libfmt dependency

- Changed `prune-branches` option to `best-branches`, flag now works in reverse. I.e. `best-branches` tells the palette
  assignment search algorithm to only save the N best branches at each node.

## [0.0.6] - 2023-09-23

### Added

- Can now change default transparency color with `-transparency-color` option

- Customizable warning system using GNU-like warning flags

- Basic decompile mode for primary tilesets (secondary tilesets and animation decompilation still a WIP)

- Multiple palette assignment backends controllable via command line options (this system is still a WIP)

### Changed

- Lots of improved error messages

- Behavior header is now mandatorily supplied on the command line

## [0.0.5] - 2023-08-18

### Added

- Support for generating `metatile_attributes.bin`. Input path may contain an `attributes.csv` file that defines
  non-default values for any metatiles that should have non-default attributes. Both Emerald/Ruby and Firered style
  attributes are supported. Additionally, the input path should contain a `metatile_behaviors.h` file (I recommend you
  simply symlink your actual behavior header here) so that the attribute parser can understand the behavior macro names
  instead of working with hardcoded behavior values.

- Support for dual-layer tilesets. Supply the `-dual-layer` option, and Porytiles will automatically infer the layer
  type from your layer PNGs, and put that type into the generate attributes bin file.

- More warnings and errors, better printouts

### Changed

- Fieldmap configuration is a little different. Now, you can specify a target base game with the `-target-base-game`
  option (defaults to `pokeemerald` if not specified). Then, if your game uses adjusted fieldmap parameters, you can
  alter those with the fieldmap override options. E.g. to change the number of primary set palettes, use the
  `-pals-primary-override` option.

- Animation generation now makes use of *key frames*. The *key frame* is the frame of tiles that you will use to
  reference an animated tile from within your layer PNGs. The key frame can be anything you want, as long as it is
  unique (note that each key frame tile must share a final hardware palette with the corresponding tiles in other
  frames, so you have a limitation there). You specify a key frame by putting `key.png` in your input anims folder. The
  key frame will not be compiled into the final anims folder, but you will see it present in your `tiles.png`.

## [0.0.4] - 2023-08-11

### Added

- Support for generating compiled animated tiles. If input path contains an `anim` folder with properly formatted RGBA
  anim assets, it will generate an `anim` folder in the output location with all anim frames correctly indexed. At the
  moment, the user must still manually enter the C code in `tileset_anims.{c,h}` to drive the actual animation.

- Colored output, more descriptive error messages

- `-Wall` and `-Werror` flags to turn on some helpful compiler warnings

### Changed

- Inputs are now provided in a given directory instead of as individual files

### Removed

- Raw compilation mode

## [0.0.3] - 2023-07-23

### Added

- Support for secondary tilesets using the `--secondary` flag. Secondary tilesets must consume the layer PNGs for their
  paired primary set, but then the compiler can use that information to take advantage of all the typical secondary
  tilset optimizations (like sharing palettes and tiles)

## [0.0.2] - 2023-07-19

### Added

- Generate `metatiles.bin` in addition to `tiles.png` and pal files (for primary tilesets only)

### Changed

- Complete algorithm overhaul from the `0.0.1` release
- CLI has been updated significantly, please see the `--help` option for info

## [0.0.1] - 2023-06-12

Preview release.

### Added

- Basic functionality: Porytiles will allocate palettes and create an indexed tilesheet for most input images
- Basic palette priming support: Porytiles allows you to force the palette allocation algorithm to guarantee that
  specified colors will be in the same palette

[Unreleased]: https://github.com/grunt-lucas/porytiles/compare/0.0.7...HEAD

[0.0.7]: https://github.com/grunt-lucas/porytiles/compare/0.0.6...0.0.7

[0.0.6]: https://github.com/grunt-lucas/porytiles/compare/0.0.5...0.0.6

[0.0.5]: https://github.com/grunt-lucas/porytiles/compare/0.0.4...0.0.5

[0.0.4]: https://github.com/grunt-lucas/porytiles/compare/0.0.3...0.0.4

[0.0.3]: https://github.com/grunt-lucas/porytiles/compare/0.0.2...0.0.3

[0.0.2]: https://github.com/grunt-lucas/porytiles/compare/0.0.1...0.0.2

[0.0.1]: https://github.com/grunt-lucas/porytiles/tree/0.0.1