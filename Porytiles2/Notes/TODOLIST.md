# Tasks

## Refactor and finish TilesetRepo and project impl
- Make Porytiles yaml config a first class type (vector<string> file lines is fine)
- Make PLA files a first class type in Porymap component (need a domain type for PLA)
- `TilesetRepo::write` should do a clean wipe of the current tileset folder before writing new artifacts?
  - See various TODOs about stale artifacts and "possibly incomplete" components
  - Switch to the original "atomic move" idea
  - Or figure out how to "delete" irrelevant assets, maybe we can move them to a .bak directory or something
    - It should be easy, the only assets we need to actually delete are anims
    - we can figure out which ones should exist by looking at the "target component" of the operation and also the ArtifactEditMode
    - (will need to think through how anim handling will work in locked, patch, optimize)
- Project Impl
  - Finish handling for unfinished types (attr.csv, anim frames, etc)
- JSON Impl
  - Start working on a JSON impl that can read/write tilesets from a standardized JSON format 

## Palette Packing
- Implement classic dfs and bfs
- Implement multiplicity-based dfs and bfs
  - it will basically be BestFusion but with backtracking
- Implement a "shotgun approach" strategy that tries different sub-strategies until success
  - the paper has some ideas on which algorithm to use depending on problem set's "multiplicity"
  - let's make the shotgun approach smart based on the character of the input

## Project Structure Refactor
- Domain layer is getting way too crowded
  - break it up into `config`, `core`, `tileset`, `layout`, `packing`
  - each of these folders can have subfolders `algorithms`, `models`, `repos`, `services`
- Make each layer a completely isolated library so that the compiler enforces dependency rules

## Start building animation system
- See initial attempt in 2/anim1
- change to `anim.json` instead of `anim.yaml`
  - even though it's technical a user config file, since Porytiles overwrites it every time, it's unintuitive for users
  - JSON is better since comments are disallowed by default and syntax is simpler, users won't be confused when their idiosyncracies get clobbered

## Start working on secondary tileset compilation

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

## Start designing layout import/compile

## Identify unit/integration testing gaps and fill them in

## Figure out how to remove fmtlib from random places all over the codebase
- Ideally it should be isolated so we could easily swap to a different string formatting library

## Miscellaneous Cleanup
- Palette hints should be validated entirely within the config system, remove validation from the PaletteValidator
- Can I use std::span in more places?
- Introduce "remark" category of diagnostics
- Fix up `Scripts` directory
  - we should split it up by `Porytiles1` and `Porytiles2` for better usability
- Provide a configuration that allows users to request ascii-only output
  - e.g. in the file highlighting, the → would become ->

## Clean up TODOs in codebase: `rg -e TODO Porytiles2/`
