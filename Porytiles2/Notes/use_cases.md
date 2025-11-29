# Use Cases
A summary of the use cases Porytiles2 must support.


## Create Primary Tileset
Create a new primary tileset in `data/tilesets/primary` with [default assets.](#default-assets)

### CLI Invocation
```sh
porytiles2 create-tileset MyTileset
```

### Logic Flow
1. Check if the primary tileset already exists. If so, abort with an error message.
2. Initialize a `PorytilesTilesetComponent` with default assets.
3. Initialize a blank `PorymapTilesetComponent`, to be filled later.
4. Initialize a `Tileset` aggregate with the components.
5. Compile the `Tileset`, generating a new modified `Tileset`.
6. Update the source and header files.
7. Persist the new `Tileset` (which also caches the checksums).

### Outputs
The resulting tileset directory tree:
```
data/tilesets/primary/
├─ my_tileset/
│  ├─ metatile_attributes.bin
│  ├─ metatiles.bin
│  ├─ tiles.png
│  ├─ anim/
│  │  ├─ flower/
│  │  |  ├─ 00.png
│  │  |  ├─ ...
│  ├─ palettes/
│  │  ├─ 00.pal
│  │  ├─ ...
│  ├─ porytiles/
│  │  ├─ bottom.png
│  │  ├─ middle.png
│  │  ├─ top.png
│  │  ├─ attributes.csv
│  │  ├─ my_tileset.toml
│  │  ├─ artifact_checksums.json
│  |  ├─ anim/
│  │  |  ├─ flower/
│  │  |  |  ├─ key.png
│  │  |  |  ├─ 00.png
│  │  |  |  ├─ ...
```

`src/data/tilesets/graphics.h` diff:
```C
const u16 gTilesetPalettes_MyTileset[][16] =
{
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/00.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/01.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/02.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/03.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/04.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/05.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/06.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/07.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/08.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/09.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/10.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/11.gbapal"),
    INCBIN_U16("data/tilesets/primary/my_tileset/palettes/12.gbapal"),
};

const u32 gTilesetTiles_MyTileset[] = INCBIN_U32("data/tilesets/primary/my_tileset/tiles.4bpp.lz");
```

`src/data/tilesets/headers.h` diff:
```C
const struct Tileset gTileset_MyTileset =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_MyTileset,
    .palettes = gTilesetPalettes_MyTileset,
    .metatiles = gMetatiles_MyTileset,
    .metatileAttributes = gMetatileAttributes_MyTileset,
    .callback = NULL,
};
```

`src/data/tilesets/metatiles.h` diff:
```C
const u16 gMetatiles_MyTileset[] = INCBIN_U16("data/tilesets/primary/my_tileset/metatiles.bin");
const u16 gMetatileAttributes_MyTileset[] = INCBIN_U16("data/tilesets/primary/my_tileset/metatile_attributes.bin");
```

### Default Assets
They could be simple:
- one row of metatiles with some example tiles: grass, animated flower
- one or two non-default attributes in the CSV file
- a simple animated flower


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



## Create Secondary Tileset
Create a new secondary tileset in `data/tilesets/secondary` with [default assets.](#default-assets)

### CLI Invocation
```sh
porytiles2 create-tileset MySecondaryTileset --partner-primaries MyTileset
```

```sh
# This special case allows the user to create a secondary tileset that isn't paired with any particular primary
porytiles2 create-tileset MySecondaryTileset --any-partner-primary
```

### Logic Flow
1. Check if the secondary tileset already exists. If so, abort with an error message.
2. Initialize a new `Tileset` aggregate.
3. Fill in the `PorytilesTilesetComponent` with default assets.
4. Compile the `PorytilesTilesetComponent` to generate an initial `PorymapTilesetComponent` and initial artifact checksums.
5. Persist the `Tileset` aggregate (including updating the right source and header files, i.e. `graphics.h`, `headers.h`, and `metatiles.h`).

### Outputs
`my_secondary_tileset.toml` contents:
```toml
# my_secondary_tileset.toml

[tileset]
partner_primaries = [ "my_tileset" ]
```

For the `--any-partner-primary` case:
```toml
# my_secondary_tileset.toml

[tileset]
partner_primaries = [ "*" ]
```


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
    "tiles_count": 512,
    "non_transparent_tiles_count": 322,
    "tiles": [
      {
        "index": 0,
        "transparent": true
      },
      {
        "index": 1,
        "transparent": false
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
    // middle.png, top.png
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


## Create Layout
TODO


## Import Layout
TODO


## Compile Layout
TODO


## Decompile Layout
TODO
