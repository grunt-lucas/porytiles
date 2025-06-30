# Architecture
Outline of the Porytiles2 architecture.

https://matklad.github.io//2021/02/06/ARCHITECTURE.md.html

<!-- TOC -->
* [Architecture](#architecture)
* [Tileset/Layout TOML File](#tilesetlayout-toml-file)
* [Use Cases](#use-cases)
  * [Default Assets](#default-assets)
  * [Create Primary Tileset](#create-primary-tileset)
  * [Create Secondary Tileset](#create-secondary-tileset)
  * [Import Primary Tileset](#import-primary-tileset)
  * [Import Secondary Tileset](#import-secondary-tileset)
  * [Import Layout](#import-layout)
  * [Delete Tileset/Layout](#delete-tilesetlayout)
  * [Compile Primary Tileset](#compile-primary-tileset)
  * [Compile Secondary Tileset](#compile-secondary-tileset)
  * [Create Layout](#create-layout)
  * [Compile Layout](#compile-layout)
* [Incremental Build Support](#incremental-build-support)
* [Layout Metatile Generation](#layout-metatile-generation)
* [Multiple Partner Primary Support](#multiple-partner-primary-support)
* [Code Organization](#code-organization)
<!-- TOC -->

# Tileset/Layout TOML File
Porytiles2 allows users to configure tileset/layout compilation options using a TOML file.
This will save tons of annoying typing at the CLI.

```toml
# my_secondary_tileset.toml

[tileset]
# This field is required for secondary sets, users won't have to specify paired primary at CLI
partner_primaries = [ "my_cool_primary" ]

[fieldmap-overrides]
num_pals_primary = 7
num_metatiles_primary = 2048
num_metatiles_total = 4096

[palette-assignment]
force_smart_prune = true
```


# Use Cases
A summary of the CLI-driver-based use cases Porytiles2 must support.

## Default Assets
Default assets are referenced in use-cases below. They could be simple:
- one row of metatiles with some example tiles: grass, animated flower
- one or two non-default attributes in the CSV file
- a simple animated flower

## Create Primary Tileset
Create a new primary tileset in `data/tilesets/primary` with [default assets.](#default-assets)

```sh
porytiles2 create-tileset MyTileset
```

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

### Logic Flow
1. Create the requisite Porytiles files
2. Update the right source and header files
3. Compile the initial tileset to generate the Porymap tileset files

## Create Secondary Tileset
Create a new secondary tileset in `data/tilesets/secondary` with [default assets.](#default-assets)

```sh
porytiles2 create-tileset MySecondaryTileset --partner-primaries MyTileset
```

This command will:
1. Create the requisite Porytiles files
2. Update the right source and header files
3. Compile the initial tileset to generate the Porymap tileset files

`my_secondary_tileset.toml` contents:
```toml
# my_secondary_tileset.toml

[tileset]
partner_primaries = [ "my_tileset" ]
```

## Import Primary Tileset
Import an existing primary tileset to Porytiles.
This is what legacy Porytiles called "decompilation."

```sh
porytiles2 import-tileset general
```

Importing a tileset will set `incremental = true` by default.
[See here for more on incremental builds.](#incremental-build-support)

```toml
# general.toml

[tileset]
incremental = true
```

### Logic Flow
1. Perform a complete decompilation.
2. Create the requisite Porytiles files.
3. Perform a compilation to confirm everything works correctly.

## Import Secondary Tileset
TODO

## Import Layout
TODO

## Delete Tileset/Layout
Should we support deleting tilesets/layouts?
If we use Clang, we can somewhat easily remove the various code elements associated with a tileset.

## Compile Primary Tileset
Compile a tileset in `data/tilesets/primary`, i.e. update the Porymap assets to match the Porytiles assets.

```sh
porytiles2 compile-tileset MyTileset
```

### Logic Flow
1. Import the Porymap assets and compute hashes for each.
2. Import cached LastHash from the Porytiles assets.
3. If any don't match, bail with message "unimported changes present in Porymap asset X"
4. If all match, continue.
5. If newest Porymap asset "modified" timestamp is newer than newest Porytiles asset "modified" timestamp, exit with "nothing to do."
6. Otherwise, continue with compilation.
7. Emit compilation result if successful.
8. Compute hash for each emitted asset and store in Porytiles asset LastHash cache.

## Compile Secondary Tileset
TODO

## Create Layout
TODO

## Compile Layout
TODO

# Incremental Build Support
User can specify an incremental tileset build by specifying `--incremental`
or by setting `incremental = true` in the tileset TOML config.

# Layout Metatile Generation
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

# Multiple Partner Primary Support
Niche use case, but would be cool. A secondary could specify multiple partner primaries like:
```toml
# my_secondary_tileset.toml

[tileset]
partner_primaries = [ "general_cave_brown", "general_cave_grey" ]
```
The secondary would then supply separate versions of each layer PNG, one for each partner primary.
E.g. `bottom.general_cave_brown.png`, `bottom.general_cave_grey.png`, etc.

The tileset compiler would enforce that
e.g. metatile 8 as seen in the `general_cave_brown` version of the layer PNGs
generates the same metatile data as metatile 8 in `general_cave_grey` version.

This means we must provide some way for users to massage the output ordering of their primary tilesets.
That way these guarantees can be made.
I am not sure if this is something that can be done entirely computationally,
without user intervention.

# Code Organization
domain-driven design
