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