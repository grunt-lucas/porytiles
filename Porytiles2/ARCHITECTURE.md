# Architecture
Outline of the Porytiles2 architecture.

https://matklad.github.io//2021/02/06/ARCHITECTURE.md.html

<!-- TOC -->
* [Architecture](#architecture)
* [Use Cases](#use-cases)
  * [Default Assets](#default-assets)
  * [Create Primary Tileset](#create-primary-tileset)
  * [Create Secondary Tileset](#create-secondary-tileset)
  * [Compile Primary Tileset](#compile-primary-tileset)
  * [Compile Secondary Tileset](#compile-secondary-tileset)
  * [Create Layout](#create-layout)
  * [Compile Layout](#compile-layout)
  * [Import Primary Tileset](#import-primary-tileset)
  * [Import Secondary Tileset](#import-secondary-tileset)
  * [Import Layout](#import-layout)
* [Tileset TOML File](#tileset-toml-file)
* [Code Organization](#code-organization)
<!-- TOC -->

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

This command will:
1. Create the requisite Porytiles files
2. Update the right source and header files
3. Compile the initial tileset to generate the Porymap tileset files

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

## Create Secondary Tileset

## Import Primary Tileset

## Import Secondary Tileset

## Import Layout

## Delete Tileset/Layout
Should we support deleting tilesets/layouts?
If we use Clang, we can somewhat easily remove the various code elements associated with a tileset.

## Compile Primary Tileset

## Compile Secondary Tileset

## Create Layout

## Compile Layout

# Tileset TOML File
TODO

# Code Organization
DDD
