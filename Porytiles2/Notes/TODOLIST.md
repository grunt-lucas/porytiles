# Tasks

## Refactor and finish TilesetRepo and project impl
- Make Porytiles yaml config a first class type (vector<string> file lines is fine)
- Make PLA files a first class type in Porymap component (need a domain type for PLA)
- `TilesetRepo::write` should do a clean wipe of the current tileset folder before writing new artifacts
  - See various TODOs about stale artifacts and "possibly incomplete" components
  - Switch to the original "atomic move" idea
- Project Impl
  - Finish handling for unfinished types (attr.csv, anim frames, etc)
- JSON Impl
  - Start working on a JSON impl that can read/write tilesets from a standardized JSON format 

## Start implementing basic palette packing code
- need basic data structures
- PalettePacker domain service
  - we'll need some basic interface to communicate which pal slots are fixed, which are available, etc
  - maybe a PackablePal type?
  - input: list of PackablePals built from tileset pals and supplied overrides, list of PaletteHints
  - output: list of packed palettes
  - client code of PalettePacker should never see nor deal with internal types, it should be pals in and pals out

## Start building animation system

## Start working on secondary tileset compilation

## Implement `create-tileset` command

## Design and Implement `verify-tileset` command
- new idea: `diff-tileset`
- create a JsonTilesetArtifactReader/Writer (can be used by `dump-tileset` as well)
- ArtifactChecksumProvider will also dump full json of tileset to `artifact_checksums.json`
- Can compare that to current state to display a rich diff
- This poses the question:
  - should our anti-clobber mechanism compute checksum based on tileset binary data or json representation?
  - this would provide the advantage that changes to on-disk artifacts which don't result in a logical tileset diff wouldn't block a build
    - e.g. a PNG metadata change, changing line-ending format of .pal file, etc
    - disadvantage: it's way more complex than just checksumming the binary data
    - we have StreamDigest, if we give the Tileset constituent types a to_string function, we can MD5 it for a logical checksum

## Implement `dump-tileset-config` command

## Implement `dump-tileset` command

## Start designing layout create/import/compile

## Clean up TODOs in codebase: `rg -e TODO Porytiles2/`
