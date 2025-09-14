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

This operation is constructed with a pointer to the PorytilesTilesetComponent and supplies pointers to the bottom, middle, and top RGBA layer images.

### Construct RGBA Metatiles Op

| Inputs                                | Outputs                   |
|---------------------------------------|---------------------------|
| bottom, middle, top RGBA layer Images | std::vector<RgbaMetatile> |

### Generate Color Collision Warnings Op

| Inputs                    | Outputs |
|---------------------------|---------|
| std::vector<RgbaMetatile> | None    |

This leaf operation reads the RgbaMetatiles and generates a warning for rgba32 colors that will collide after gbagfx compression.

### Create Normalized Tiles Op

| Inputs                    | Outputs                     |
|---------------------------|-----------------------------|
| std::vector<RgbaMetatile> | std::vector<NormalizedTile> |

## Compile Primary Incremental

### Tileset Supplier Op