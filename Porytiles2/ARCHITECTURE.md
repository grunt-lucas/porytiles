# Architecture
Outline of the Porytiles2 architecture.

https://matklad.github.io//2021/02/06/ARCHITECTURE.md.html

# Code Organization
Porytiles2 is organized according to domain-driven design principles.

## Domain
TODO : summarize the core domain types

# Use Cases
A summary of the use cases Porytiles2 must support.

## Default Assets
Default assets are referenced in the use-cases below. They could be simple:
- one row of metatiles with some example tiles: grass, animated flower
- one or two non-default attributes in the CSV file
- a simple animated flower

## Create Primary Tileset
Create a new primary tileset in `data/tilesets/primary` with [default assets.](#default-assets)

### CLI Invocation
```sh
porytiles2 create-tileset MyTileset
```

### Logic Flow
1. Check if the primary tileset already exists. If so, abort with an error message.
2. Initialize a `PorytilesTilesetComponent` with default assets.
3. Compile the `PorytilesTilesetComponent` to generate an initial `PorymapTilesetComponent`.
4. Initialize a new `Tileset` aggregate with the components.
5. Generate checksums and update the source and header files.
6. Persist the `Tileset`.

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
3. If `PorytilesTilesetComponent` is not empty (i.e., a `porytiles` folder exists), and the newest Porytiles asset is newer than the newest Porymap asset, bail with the message "uncompiled changes in Porytiles asset X."
4. Import the Porymap assets into the `PorymapTilesetComponent` and compute checksums for each.
5. If `PorytilesTilesetComponent` is not empty and all checksums match, bail with the message "nothing to do."
6. Perform a complete decompilation.
7. Fill in the `PorytilesTilesetComponent` with the decompiled assets.
8. Perform an incremental compilation and re-store checksums.
9. Persist the `Tileset` aggregate.

### Outputs
Importing a tileset will set `incremental = true` by default.
[See here for more on incremental builds.](#incremental-build-support)

```toml
# general.toml

[tileset]
incremental = true
```

## Import Secondary Tileset
TODO : fill in use-case details

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
3. Compute checksums for each asset in `PorymapTilesetComponent`.
4. Compare with cached checksums in `artifact_checksums.json`, if any differ, bail with the message "unimported changes present in Porymap asset X."
5. If all match, continue.
6. If the newest Porymap asset "modified" timestamp is newer than the newest Porytiles asset "modified" timestamp, bail with "nothing to do."
7. Otherwise, compile the `PorytilesTilesetComponent`, generating a new `PorymapTilesetComponent`.
8. Compute new artifact checksums.
9. Persist the `Tileset` aggregate.

## Compile Secondary Tileset
TODO

## Compile Layout
TODO

# Staging Area For Noteworthy Topics

## Artifact Checksums
TODO : EXPLAIN

## Incremental Build Support
User can specify an incremental tileset build by specifying `--incremental`
or by setting `incremental = true` in the tileset TOML config.

When incremental is set,
compilation will not disturb currently existing Porymap assets.
That is, existing palettes will be treated as "overrides" in the compilation,
and existing tiles will be left undisturbed (but reused if possible).
Incremental builds assume any transparent tile is available, and any `0 0 0`
in a palette can be assumed as a wildcard.

It should be noted:
since incremental builds don't disturb existing assets,
that means they also won't remove output assets that aren't used.
That is, if you remove all instances of a given tile from the metatile sheets,
an incremental build will still leave that tile in `tiles.png`.
This is so that incremental builds can be used as a method for editing tilesets
without disturbing anyone who might depend on that tileset.

## Layout Metatile Generation
Layout compilation runs with default: `--unknown-metatile-policy=reject`.
When the layout compiler encounters a metatile that's not present in the primary or secondary tileset,
it will error out with a diagnostic message.

Users can optionally supply alternatives:
`--unknown-metatile-policy=add-to-primary` and `--unknown-metatile-policy=add-to-secondary`.
When the add-to-primary policy is enabled,
instead of erroring out upon an unknown metatile,
the layout compiler will append the metatile to the end of the primary tileset and continue.
User can specify `--recompile-after=each` or `--recompile-after=all` to control when tileset recompilation happens.
Either after each time the layout compiler updates the tileset, or at the end after all updates have been made.
`--recompile-after=each` is much more CPU intensive, but it can catch issues earlier.

The add-to-secondary policy functions the same way, but appending to the secondary tileset instead.

By combining and recombining these policies through an iterative workflow,
users can build functional layouts and tilesets by simply drawing the maps they want as-is
and generating the necessary metatiles on an as-needed basis.

## Multiple Partner Primary Support
Niche use case, but would be cool. A secondary could specify multiple partner primaries like:
```toml
# my_secondary_tileset.toml

[tileset]
partner_primaries = [ "general_cave_brown", "general_cave_grey" ]
```
The secondary would then supply separate versions of each layer PNG, one for each partner primary.
E.g. `bottom.general_cave_brown.png`, `bottom.general_cave_grey.png`, etc.

The tileset compiler would enforce that
e.g., metatile 8 as seen in the `general_cave_brown` version of the layer PNGs
generates the same metatile data as metatile 8 in `general_cave_grey` version.

This means we must provide some way for users to massage the output ordering of their primary tilesets.
That way these guarantees can be made.
I am not sure if this is something that can be done entirely computationally,
without user intervention.

## Tileset/Layout TOML File
Porytiles2 allows users to configure tileset/layout compilation options using a TOML file.
This will save tons of annoying typing at the CLI.

```toml
# my_secondary_tileset.toml

[tileset]
# This field is required for secondary sets, users won't have to specify paired primary at CLI
partner_primaries = [ "my_cool_primary" ]

[fieldmap_overrides]
num_pals_primary = 7
num_metatiles_primary = 2048
num_metatiles_total = 4096

[palette_assignment]
force_smart_prune = true
```

# Tileset Compilation
TODO : detailed overview of tileset compilation in Porytiles2.

# Tileset Decompilation
TODO : detailed overview of tileset decompilation in Porytiles2.

# Animations
Detailed overview of animation handling.
Since decompilation-recompilation and incremental compilation
are such a big piece of the Porytiles2 flow,
animation handling needs to be transparently symmetrical.
