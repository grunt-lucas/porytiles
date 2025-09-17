# Architecture

The PrimaryTilesetCompiler is a simple service wrapper
that constructs the compilation Pipeline from the set of operations defined in the operations folder.

The Pipeline and all Operations are non-owning for input operators.
They expect that a composition root owns all tileset components provided as input,
thus they can deal with simple pointers.
Outputs are passed using the orchestration OperandBundle system.

The rough operation flow is defined below. 

## Compile Primary

### Tileset Supplier Op

| Inputs | Outputs                               |
|--------|---------------------------------------|
| None   | bottom, middle, top RGBA layer Images |

This operation is constructed with a pointer to the PorytilesTilesetComponent
and supplies pointers to the bottom, middle, and top RGBA layer images.

### Construct RGBA Metatiles Op

| Inputs                                | Outputs                     |
|---------------------------------------|-----------------------------|
| bottom, middle, top RGBA layer Images | `std::vector<RgbaMetatile>` |

### Generate Color Collision Warnings Op

| Inputs                      | Outputs |
|-----------------------------|---------|
| `std::vector<RgbaMetatile>` | None    |

This leaf operation reads the RgbaMetatiles
and generates a warning for Rgba32 colors that will collide after `gbagfx` compression.

### Create Normalized Tiles Op

| Inputs                      | Outputs                       |
|-----------------------------|-------------------------------|
| `std::vector<RgbaMetatile>` | `std::vector<NormalizedTile>` |

### Build Color Index Map Op

| Inputs                        | Outputs                                   |
|-------------------------------|-------------------------------------------|
| `std::vector<NormalizedTile>` | `std::unordered_map<Rgba32, std::size_t>` |

Given a set of normalized tiles,
this operation builds a map from Rgba32 colors to their unique color index.

### Build Reverse Color Index Map Op

| Inputs                        | Outputs                                   |
|-------------------------------|-------------------------------------------|
| `std::vector<NormalizedTile>` | `std::unordered_map<std::size_t, Rgba32>` |

Given a set of normalized tiles, this operation builds the inverse of the above map,
i.e., a mapping from unique color indices back to their corresponding Rgba32 colors.

### Generate ColorSets

| Inputs                                                                   | Outputs                             |
|--------------------------------------------------------------------------|-------------------------------------|
| `std::vector<NormalizedTile>`, `std::unordered_map<Rgba32, std::size_t>` | `std::vector<TaggedNormalizedTile>` |

Given a set of normalized tiles and a map from Rgba32 colors to their unique color index,
this operation generates a set of `TaggedNormalizedTile`s.

`TaggedNormalizedTile` is a struct that contains a `NormalizedTile` as well as some additional metadata.
The metadata:
- raw tile index
- `ColorSet` for the tile
- assigned palette index (which will be selected by the VM packing code)

## Compile Primary Incremental

### Tileset Supplier Op