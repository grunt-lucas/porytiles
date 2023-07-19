# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/grunt-lucas/porytiles/compare/0.0.2...HEAD

[0.0.2]: https://github.com/grunt-lucas/porytiles/compare/0.0.1...0.0.2

[0.0.1]: https://github.com/grunt-lucas/porytiles/tree/0.0.1