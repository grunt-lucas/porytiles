# Compile Primary Tileset

## Color Handling
Porytiles2 will use full RGBA all the way through.
We have to do this to support complete symmetric import/compilation,
since vanilla tilesets use 8-bit colors in their pals.

## Normalization
Implement normalization solution outlined here: https://github.com/grunt-lucas/porytiles/issues/118
This normalization that includes the palette will be called full normalization.
Alternatively, we can provide the normalization of Porytiles1 called "partial normalization".
We'll want both, because for incremental compilation,
we need to be able to detect when a sibling is in use.
And for that, we'll need partial normalization as well.
Let's think about this more.

## Tile Workspace
`tiles.png` can be represented by a TileWorkspace,
which is basically a searchable collection of NormalizedTiles.
For the incremental compilation case,
we can prefill the workspace with the existing tiles.
That way, when we get to the tile assignment step,
we can just look up the tile in the workspace.
We will of course need to calculate the correct flip bits for the final TilemapEntry.

## Example Mermaid Diagrams

### Data Flow Diagram

```mermaid
graph TD
    A[PorytilesTilesetComponent] --> B[TileNormalizationOperation]
    B --> |normalized_tiles| C[ColorExtractionOperation]
    B --> |tile_mappings| G[MetatileCompilationOperation]
    
    C --> |unique_colors| D[PaletteGenerationOperation]
    C --> |color_index_map| D
    
    D --> |color_sets| E[PaletteAssignmentOperation]
    
    E --> |palette_assignments| F[TileIndexingOperation]
    E --> |hardware_palettes| F
    E --> |palette_assignments| H[AnimationCompilationOperation]
    E --> |hardware_palettes| H
    
    F --> |vram_tiles| G[MetatileCompilationOperation]
    F --> |tile_index_map| G
    
    A --> H[AnimationCompilationOperation]
    A --> G
    
    G --> |vram_metatiles| I[OutputAssemblyOperation]
    H --> |vram_animations| I
    E --> |hardware_palettes| I
    
    I --> J[PorymapTilesetComponent]
```

### Pipeline Execution Flow

```mermaid
sequenceDiagram
    participant Client
    participant TilesetCompilerPipeline
    participant Pipeline
    participant Operations
    participant OperandBundle
    
    Client->>TilesetCompilerPipeline: compile_primary(porytiles_component)
    TilesetCompilerPipeline->>Pipeline: create with operations
    TilesetCompilerPipeline->>OperandBundle: add initial inputs
    TilesetCompilerPipeline->>Pipeline: run()
    
    loop For each operation
        Pipeline->>Operations: execute(bundle, diag)
        Operations->>OperandBundle: get inputs
        Operations->>Operations: process data
        Operations->>OperandBundle: add outputs
        Operations-->>Pipeline: Result<OperandBundle>
    end
    
    Pipeline-->>TilesetCompilerPipeline: Result<void>
    TilesetCompilerPipeline->>OperandBundle: get("porymap_component")
    TilesetCompilerPipeline-->>Client: Result<PorymapTilesetComponent>
```

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

### Validate Colors Op

| Inputs                      | Outputs |
|-----------------------------|---------|
| `std::vector<RgbaMetatile>` | None    |

This leaf operation reads the RgbaMetatiles and:
- generates a warning for Rgba32 colors that will collide after `gbagfx` compression
- generates a warning (or error?) for Rgba32 colors with invalid alpha values (must be either 0 or 255)
    - alpha 0 is treated as transparent, alpha anything else is opaque, but warn user if not 255 since partial opacity isn't a thing and probably isn't what the user intended

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

## Compile Primary (Using New Isomorphism Types)

1. Convert layer images into std::vector<RgbaMetatile>
2. Leaf step to generate precision loss warnings if some colors collapse to same 5 bit color. We do this first so that we have context about where the offending colors are in the input metatiles.
3. Generate color index map from metatile vector
4. Using the index map and metatiles, generate std::vector<PackSet> (assignable tiles)
5. Run pal assignment bin packing (Run pal alignment based on IsoColorTiles?) to generate std::vector<HardwarePalette>
6. Convert metatiles to std::vector<IsoFlipTile>
7. std::vector<IsoFlipTile> and std::vector<PackSet> are aligned, do a zip-loop and generate std::vector<Tile<IndexPixel>> (tiles.png) and std::vector<TilemapEntry>


## Compile Primary Incremental

### Tileset Supplier Op