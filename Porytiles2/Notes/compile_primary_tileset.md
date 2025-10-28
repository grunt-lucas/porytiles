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

Tile workspace can be a collection of CanonicalPixelTile<IndexPixel>

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

Convert layer images into `vector<RgbaMetatile>`

Leaf step to throw error if there are too many metatiles.

Decompose `vector<RgbaMetatile>` into `vector<RgbaTile>` (we have a compute_metatile function which allows us to reconstruct the original metatile params from a raw tile index)
 
Leaf step to throw errors if:
- any tiles contain an invalid alpha value
- any tiles have more than 15+1 colors
- generate precision loss warnings if some colors collapse to the same 5-bit color

Create color index map from `vector<RgbaTile>`

Generate `vector<CanonicalShapeTile<ColorIndex>>` using color index map and `vector<RgbaTile>`

Create `vector<PackSet>` for VM packing (definition TBD)

Optional: via `vector<CanonicalShapeTile>` compute color isomorphism cliques to pass to VM packer

Create `vector<PackBin>` to pass to VM packer (`PackBin` is the hardware pal type?)

Run VM packing

Convert `vector<PackBin>` to `vector<RgbaPal>`

Convert `vector<CanonicalShapeTile<ColorIndex>>` -> `vector<CanonicalShapeTile<Rgba32>>`

Use each elem of `vector<CanonicalShapeTile<Rgba32>>` plus `vector<RgbaPal>` to create `vector<CanonicalPixelTile<IndexPixel>>`

Init blank TileWorkspace, add in override tiles from `porytiles/tiles_override.png`

Use the `vector<CanonicalPixelTile<IndexPixel>>` and `vector<PackSet>` to fill up TileWorkspace and generate TilemapEntries

We have three parallel tile vectors, each entry aligned to correspond to the same tile:
- vector<RgbaTile>: the original PixelTile with color data
- vector<CanonicalShapeTile>: the canonicalized ShapeTile version of the tile, mapped to ColorIndex
- vector<PackSet>: this tile's PackSet, which stores the tile ColorSet and final pal assignment

### Compile Primary Incremental -- Fixed
Convert `PorytilesTilesetComponent` layer images into `vector<RgbaMetatile> porytiles`

Use `MetatileDecompiler` to decompile the `PorymapTilesetComponent` into `vector<RgbaMetatile> porymap`

Leaf step to throw error if there are too many metatiles.

Decompose `vector<RgbaMetatile> porytiles` into `vector<RgbaTile> porytiles`
(we have a compute_metatile function which allows us to reconstruct the original metatile params from a raw tile index)

Decompose `vector<RgbaMetatile> porymap` into `vector<RgbaTile> porymap`

Leaf step to throw errors if any `vector<RgbaTile> porytiles` have more than 15+1 colors.

Leaf step to generate precision loss warnings if some `vector<RgbaTile> porytiles` colors collapse to the same 5-bit color.

Create color index map from `vector<RgbaTile> porytiles`

Generate `vector<CanonicalShapeTile<ColorIndex>>` using color index map and `vector<RgbaTile> porytiles`

Convert `vector<CanonicalShapeTile<ColorIndex>>` -> `vector<CanonicalShapeTile<Rgba32>>`

Init a `vector<size_t> pal_indexes`.
Use each elem of `vector<CanonicalShapeTile<Rgba32>>` plus Porymap `vector<RgbaPal>` to create `vector<CanonicalPixelTile<IndexPixel>>`
Note: we need to make sure to only check the pals relevant to the tileset, i.e. if this is primary, don't check pals 7-15.
If no pal matches, emit an error and continue until the end of the vector.
Otherwise, push back the matching pal index to `pal_indexes`.

Init blank TileWorkspace, add in override tiles from Porymap `tiles.png` (warn that `porytiles/tiles_override.png` will be ignored)

Iterate over `vector<CanonicalPixelTile<IndexPixel>>` and both `vector<RgbaTile>`.
If current `porytiles RgbaTile` equals the `porymap RgbaTile`, then we don't need to compute anything, it's unchanged.
Just grab the original tilemap entry and re-emit.
Note: it's possible the `porytiles RgbaTile` and `porymap RgbaTile` might have different transparencies.
E.g. the porytiles one might be using alpha channel,
while the porymap one will be using the extrinsic transparency color (see MetatileDecompiler).
So we need to check equality while ignoring different representations of transparent pixels.

If the tiles differ, then we have a genuine update.
Find the current `CanonicalPixelTile<IndexPixel>` in the TileWorkspace.
If it doesn't exist, emit an error and continue until the end of the vector.
Otherwise, check the corresponding pal index in `pal_indexes`.
We now have the tile index, pal index, and flip bits. Emit a tilemap entry.

```c++
// This is a rough outline, consider it pseudocode
ChainableResult<std::unique_ptr<Tileset>> PrimaryTilesetCompiler::compile_incremental(const Tileset &tileset)
{
    
}
```
