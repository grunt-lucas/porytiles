# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Sibling tile support: can now specify cross-palette colors that should share indexes, allowing for tiles that are
  shared across palettes

### Changed

- Better tile indexing logging: logs are super greppable so you can easily figure out where Porytiles put your tile on
  the final sheet
- Output tileset PNG now uses 8bpp indexes to display true colors based on the palettes: `gbagfx` will just truncate
  the top 4 bits so it works as intended

### Fixed

- Properly parse color-related CLI options

## [0.0.1] - 2023-06-12

Preview release.

### Added

- Basic functionality: Porytiles will allocate palettes and create an indexed tilesheet for most input images
- Basic palette priming support: Porytiles allows you to force the palette allocation algorithm to guarantee that
  specified colors will be in the same palette

[Unreleased]: https://github.com/grunt-lucas/porytiles/compare/0.0.1...HEAD

[0.0.1]: https://github.com/grunt-lucas/porytiles/tree/0.0.1