# Use Cases
A summary of the use cases Porytiles2 must support.

## Import Primary Tileset
Import an existing primary tileset to Porytiles for the first time, or update Porytiles assets to match the Porymap assets.

### CLI Invocation
```sh
porytiles2 import-tileset general
```

### Logic Flow
1. Check if the primary tileset exists. If not, abort with error.
2. Import the tileset into a `Tileset` aggregate using the special "import" operation.
3. If `PorymapTilesetComponent` is empty, bail with error.
4. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.
5. Perform a patch compilation with all assets set to `fixed`
6. Persist the `Tileset` (which also caches the checksums).

### Outputs
Importing a tileset for the first time will set `patch.enabled:true` by default with all assets set to `fixed`.

```yaml
# tileset.yaml
patch:
  enabled: true
  tiles: fixed
  pals: fixed
```

## Compile Primary Tileset
Compile a tileset in `data/tilesets/primary`, i.e., update the Porymap assets to match the Porytiles assets.

### CLI Invocation
```sh
porytiles2 compile-tileset MyTileset
```

### Logic Flow
1. Check if the primary tileset exists. If not, abort with error.
2. Load the tileset into a `Tileset` aggregate.
3. If `PorytilesTilesetComponent` is empty, bail with error.
4. If `PorymapTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If any differ, bail with the message "unimported changes present in Porymap asset X."
5. If all `PorytilesTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the message "nothing to do."
6. Compile the `Tileset`, generating a new modified `Tileset`.
7. Persist the `Tileset` (which also caches the checksums).

## Import Secondary Tileset
TODO

## Compile Secondary Tileset
TODO

## Dump Tileset
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
        "tile_index": 12,
        "pal_index": 0,
        "hflip": true,
        "vflip": false
      },
      {
        "tile_index": 2,
        "pal_index": 4,
        "hflip": false,
        "vflip": false
      },
      // ...
    ]
  },
  "metatile_attributes.bin": {
    "checksum": "d2ea66cca67296a861f185fd2e961c5f",
    "attributes_count": 2,
    "attributes": [
      {
        "id": 0,
        "layer_type": "normal",
        "metatile_behavior": "MB_NORMAL"
      },
      {
        "id": 1,
        "layer_type": "split",
        "metatile_behavior": "MB_OCEAN"
      }
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

## Dump Tileset Config
Dump the full source chain of a tileset config value to the console.

This command will be really useful for debugging purposes,
allowing users to inspect the full source chain of a config value.

### CLI Invocation
```sh
porytiles2 dump-tileset-config MyPrimaryTileset num_tiles_in_primary
```

### Outputs
The full source chain of the supplied config value.

## Find Tileset Color
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

## Import Layout
TODO

## Compile Layout
TODO
