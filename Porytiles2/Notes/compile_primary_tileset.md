# Compile Primary Tileset

## Color Handling
Porytiles2 will use full RGBA all the way through.
We have to do this to support complete symmetric import/compilation,
since vanilla tilesets use 8-bit colors in their pals.

## Normalization
Now handling normalization with our new PixelTile/ShapeTile distinction.
CanonicalShapeTile can easily detect flip and color isomorphisms
in a single representation.

## Tile Workspace
`tiles.png` can be represented by a TileWorkspace,
which is basically a searchable collection of canonical tiles.
For the incremental compilation case,
we can prefill the workspace with the existing tiles.
That way, when we get to the tile assignment step,
we can just look up the tile in the workspace.
We will of course need to calculate the correct flip bits for the final TilemapEntry.

## Example Mermaid Diagrams

### Example Data Flow Diagram

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

### Example Pipeline Execution Flow

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

### Compile Primary (Using New CanonicalShapeTile)

1. Convert layer images into vector<RgbaMetatile>
2. Leaf step to throw error if there are too many metatiles.
3. Decompose vector<RgbaMetatile> into vector<RgbaTile> (we have a compute_metatile function which allows us to reconstruct the original metatile params from a raw tile index)
4. Leaf step to throw errors if any tiles have more than 15+1 colors.
5. Leaf step to generate precision loss warnings if some colors collapse to the same 5-bit color.
6. Create color index map from vector<RgbaTile>
7. Generate vector<CanonicalShapeTile> using color index map and vector<RgbaTile>
8. Create vector of VM packing set type (definition TBD)
9. Optional: via vector<CanonicalShapeTile> compute color isomorphism cliques to pass to VM packer
10. Create vector of hardware palettes to pass to VM packer
11. Run VM packing on vector of pack set types
12. TODO: think through how to actually generate tiles.png and metatiles.bin data

### Compile Primary Incremental
