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

## Create Layout
TODO

## Import Primary Tileset
Import an existing primary tileset to Porytiles.
This is what legacy Porytiles called "decompilation."

### CLI Invocation
```sh
porytiles2 import-tileset general
```

### Logic Flow
1. Check if the primary tileset exists. If not, abort with error.
2. Load the tileset into a `Tileset` aggregate.
3. If `PorymapTilesetComponent` is empty, bail with error.
4. If `PorytilesTilesetComponent` is not empty, compare with cached checksums in `artifact_checksums.json`. If any differ, bail with the message "uncompiled changes present in Porytiles asset X."
5. If all `PorymapTilesetComponent` checksums match those cached in `artifact_checksums.json`, bail with the message "nothing to do."
6. Decompile the `PorymapTilesetComponent`, generating a new `PorytilesTilesetComponent`.
7. Perform an incremental compilation.
8. Persist the `Tileset` (which also caches the checksums).

TODO: review the timestamp and checksum logic here to make sure it actually catches uncompiled changes

### Outputs
Importing a tileset for the first time will set `incremental = true` by default.
[See here for more on incremental builds.](#incremental-build-support)

```toml
# general.toml

[tileset]
incremental = true
```

## Import Secondary Tileset
TODO: fill in use-case details

## Import Layout
TODO

## Delete Tileset/Layout
Should we support deleting tilesets/layouts?
If we use Clang, we can somewhat easily remove the various code elements associated with a tileset.

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

## Compile Secondary Tileset
TODO

## Compile Layout
TODO
