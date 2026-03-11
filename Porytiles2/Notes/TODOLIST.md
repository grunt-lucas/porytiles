# Tasks

## Refactor and finish TilesetRepo and project impl
- Make .pla files a first class type in Porymap component (need a domain type for .pla)
- JSON Impl
  - Start working on a JSON impl that can read/write tilesets from a standardized JSON format 

## `pokefirered` import flow broken, `const u16` is incorrect type for FireRed `metatile_attributes.bin` (it should be u32)
- this should be configurable, we can have something to auto-detect, like the HeaderDefineProvider but for this type

## Palette Packing
- Implement multiplicity-based dfs and bfs
  - it will basically be BestFusion but with backtracking
  - Do we need this at this point? Diminishing returns with all these strategies. Most assignment failures are still user-driven, when they provide clearly unassignable assets.
- Implement a "shotgun approach" strategy that tries different sub-strategies until success
  - the paper has some ideas on which algorithm to use depending on problem set's "multiplicity"
  - let's make the shotgun approach smart based on the character of the input
  - the shotgun approach should use a **threadpool** to run sub-strategies in parallel across multiple cores
    - feasibility analysis confirmed this is viable: see `Notes/parallel_packing_strategy_analysis.md`
    - iterations are embarrassingly parallel (stateless strategies, immutable input, no shared mutable state)
    - cooperative cancellation via `std::atomic<bool>` checked alongside existing `node_cutoff` in DFS/BFS
    - also opens the door to "best solution" semantics (compare results by quality) instead of first-success

## Project Structure Refactor
- Domain layer is getting way too crowded
  - break it up into `config`, `core`, `tileset`, `layout`, `packing`
  - each of these folders can have subfolders `algorithms`, `models`, `repos`, `services`
- Make each layer a completely isolated library so that the compiler enforces dependency rules

## Validation Refactor
- Move compilation (and decompilation) validation into a completely separate class that runs first
  - That way, compiler/decompiler can just panic on failed preconditions, codepaths are simpler
- Add validation for (what was I going to type here? I can't remember...)

## Config system improvements
- find a way to support config "shortcut" CLI options
  - e.g. --disable-warnings as a shortcut for --diagnostic-warnings-exclude='.*'
  - making the option is easy, the tricky part here is cleanly integrating it into our completion system

## Implement `LayoutDataProvider`
Once we complete the `ProjectTilesetArtifactKeyProvider` refactor, create a `LayoutDataProvider` which parses `layouts.json`.
We can then create a `TilesetPairProvider` that reads the layout data and provides mappings between primary/secondary.
```c++
std::set<std::string> paired_tilesets = pair_provider.get_paired_tilesets("gTileset_General");
// paired_tilesets: std::set{"gTileset_Petalburg", "gTileset_Slateport", ...}
```

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

## Implement `dump-tileset` command
Dump the contents of a tileset to a file in the current working directory.

This command will be really useful for debugging purposes,
allowing users to inspect the state of their tilesets using a human-readable text format.

### CLI Invocation
```sh
porytiles2 dump-tileset MyPrimaryTileset

porytiles2 dump-tileset MyPrimaryTileset --format=yaml --component=metatiles.bin,attributes.csv
```

### Outputs
Outputs a single file called `<TILESET-NAME>.<FORMAT>` (e.g. `MyPrimaryTileset.json`) to the CWD.
Users can use `--format` to control the output format.
Default is JSON, but we could also support YAML, CSV, etc.
Users can use `--component=<comma-separate-list>` to select which tileset components they want.
Default is all.

JSON example:
```json
{
  "name": "MyPrimaryTileset",
  "layer_mode": "triple",
  "assets_with_diffs": [
    "metatiles.bin",
    "porytiles/attributes.csv"
  ],
  "metatiles.bin": {
    "checksum": "d2ea66cca67296a861f185fd2e961c5f",
    "entries_count": 24,
    "entries": [
      {
        "index": 0,
        "metatile_id": 0,
        "entry": {
          "tile_index": 12,
          "pal_index": 0,
          "hflip": true,
          "vflip": false
        }
      },
      {
        "index": 1,
        "metatile_id": 0,
        "entry": {
          "tile_index": 204,
          "pal_index": 4,
          "hflip": false,
          "vflip": false
        }
      },
      // ...
    ]
  },
  "metatile_attributes.bin": {
    "checksum": "d2ea66cca67296a861f185fd2e961c5f",
    "attributes_count": 2,
    "attributes": [
      {
        "metatile_id": 0,
        "attribute":
        {
          "layer_type": "normal",
          "metatile_behavior": "MB_NORMAL"  
        }
      },
      // ...
    ]
  },
  "tiles.png": {
    "checksum": "d2ea66cca67296a861f185fd2e961c5f",
    "tiles_count": 512,
    "non_transparent_tiles_count": 322,
    "tiles": [
      {
        "index": 0,
        "is_transparent": true,
        "values": [
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0],
          [0, 0, 0, 0, 0, 0, 0, 0]
        ]
      },
      {
        "index": 1,
        "is_transparent": false,
        "values": [
          [1, 0, 0, 2, 2, 0, 0, 1],
          [0, 0, 0, 2, 2, 0, 0, 0],
          [0, 0, 0, 2, 2, 0, 0, 0],
          [0, 0, 0, 4, 4, 0, 0, 0],
          [0, 0, 0, 4, 4, 0, 0, 0],
          [0, 0, 0, 2, 2, 0, 0, 0],
          [0, 0, 0, 2, 2, 0, 0, 0],
          [1, 0, 0, 2, 2, 0, 0, 1]
        ]
      },
      // ...
    ]
  },
  "palettes": {
    "00.pal": [
      "255 0 255",
      "8 0 8",
      // ...
    ],
    "01.pal": [
      "255 0 255",
      "128 128 12",
      // ...
    ],
    // ...
  },
  "porytiles": {
    "bottom.png": {
      // TODO: what to put here?
    },
    "middle.png": {
      // TODO: what to put here?
    },
    "top.png": {
      // TODO: what to put here?
    },
    "attributes.csv": {
      "attributes_count": 2,
      "attributes": [
        {
          "id": 0,
          "layer_type": "normal",
          "metatile_behavior": "MB_OCEAN"
        },
        {
          "id": 1,
          "layer_type": "split",
          "metatile_behavior": "MB_OCEAN"
        }
      ]
    },
    "tiles.override.png": {
      "tiles_count": 512,
      "non_transparent_tiles_count": 3,
      "tiles": [
        {
          "index": 0,
          "is_transparent": true,
          "values": [
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0]
          ]
        },
        {
          "index": 1,
          "is_transparent": false,
          "values": [
            [1, 0, 0, 2, 2, 0, 0, 1],
            [0, 0, 0, 2, 2, 0, 0, 0],
            [0, 0, 0, 2, 2, 0, 0, 0],
            [0, 0, 0, 4, 4, 0, 0, 0],
            [0, 0, 0, 4, 4, 0, 0, 0],
            [0, 0, 0, 2, 2, 0, 0, 0],
            [0, 0, 0, 2, 2, 0, 0, 0],
            [1, 0, 0, 2, 2, 0, 0, 1]
          ]
        },
        // ...
      ]
    }
  }
}
```

## Implement `find-tileset-color` command
Given a color, print all locations of that color in a tileset.

This command will be really useful for debugging purposes,
allowing users to quickly locate stray pixels or iterate on compilation error messages.

### CLI Invocation
```sh
porytiles2 find-tileset-color MyPrimaryTileset 234,21,97
```

### Outputs
Print out fancy metatile ASCII art with X-marks-the-spot on all locations of the given color.
Porytiles will do one printout for each metatile containing the color.
Limit the output to 10 metatiles, configurable.

## Start designing layout import/compile

## Identify unit/integration testing gaps and fill them in

## Miscellaneous Cleanup
- Can I use std::span in more places?
- Fix up `Scripts` directory
  - we should split it up by `Porytiles1` and `Porytiles2` for better usability
- Provide a configuration that allows users to request ascii-only output
  - e.g. in the file highlighting, the → would become ->
- Figure out how to cleanup tileset name handling
  - We have code to scrub `gTileset_` prefixes littered all over the place
  - some logic needs full tileset name, other logic needs scrubbed name, it's a mess
  - Right now, we require users to type full name, e.g. `gTileset_SecretBase`
    - Could we create a little helper that tries to decode fuzzed names:
    - e.g. `gTileset_SecretBase`, `secret_base`, `SecretBase`, `secretBase` all would compile the same tileset

## Clean up TODOs in codebase: `rg -e TODO Porytiles2/`
