# CompilerTask Complexity Analysis & Refactoring Suggestions

## Executive Summary

After analyzing `PrimaryTilesetCompiler::CompilerTask`, I conclude that **the complexity is largely inherent to the problem domain**, but there are organizational improvements that can make the code more maintainable without fighting the essential complexity.

---

## Current State Analysis

### Scale
- **~1200 lines** in `primary_tileset_compiler.cpp`
- **6 pipeline steps** (`pipeline_step_*` functions)
- **9 helper functions** (`pipeline_helper_*` functions)
- **~25 member variables** across 5 categories

### Member Variable Categories
1. **Dependencies (8)**: Injected services (format, diag, printers, config)
2. **Config values (8)**: Unwrapped from DomainConfig at runtime
3. **Porytiles state (3)**: metatiles, pixel tiles, canonical tiles
4. **Porymap state (4)**: tilemap entries, metatiles, pixel tiles, canonical tiles
5. **Working data (4)**: palettes array, workspace, matcher, output component

### Complexity Hotspots
1. **`pipeline_helper_assign_tile_via_pal_match()`** (~80 lines) - Core tile assignment kernel
2. **`pipeline_helper_register_animations()`** (~110 lines) - Multi-phase animation setup
3. **`pipeline_helper_run_pal_packing()`** (~110 lines) - Palette packing orchestration
4. **Mode branching** - 3 edit modes create conditionals throughout

---

## Why This Complexity Is Inherent

| Factor | Why It's Unavoidable |
|--------|---------------------|
| **9 mode combinations** | Users genuinely need optimize/patch/locked for both tiles and palettes |
| **Bidirectional reconciliation** | Must match new Porytiles art against existing Porymap data |
| **Palette packing** | NP-hard bin-packing problem; delegation helps but orchestration remains |
| **Animation system** | Requires reservation → building → matching → compilation phases |
| **Rich diagnostics** | Good UX demands tracking context for detailed error messages |

**Verdict**: You cannot eliminate this complexity—only organize it better.

---

## Refactoring Options

### Option 1: Strategy Pattern for Edit Modes

```cpp
class TileEditStrategy {
    virtual void setup_workspace(...) = 0;
    virtual std::optional<TilemapEntry> try_reuse_tile(...) = 0;
    virtual TileAssignmentResult assign_tile(...) = 0;
};

class OptimizeTileStrategy : public TileEditStrategy { ... };
class PatchTileStrategy : public TileEditStrategy { ... };
class LockedTileStrategy : public TileEditStrategy { ... };
```

| Pros | Cons |
|------|------|
| Eliminates mode switches | Adds class hierarchy |
| Each mode's logic is cohesive | Strategies need lots of shared state |
| Extensible for new modes | May need friend access or parameter explosion |

**Verdict**: Overkill unless mode branching becomes truly unmanageable.

---

### Option 2: Extract Phase Objects (Pipeline Pattern)

```cpp
class PorytilesInputProcessor {
    ChainableResult<PorytilesData> process(const Tileset &tileset);
};

class TilePaletteMatcher {
    ChainableResult<MatchResult> match(const PorytilesData &, ...);
};
```

| Pros | Cons |
|------|------|
| Each phase independently testable | Creates many new classes |
| Clear data contracts between phases | No shared member state (explicit passing) |
| CompilerTask becomes thin orchestrator | Large parameter lists or intermediate structs |

**Verdict**: Significant architectural change with high testing payoff, but high effort.

---

### Option 3: Extract Helper Classes (Recommended)

Keep everything in the anonymous namespace but group related functions + state:

```cpp
// In anonymous namespace of primary_tileset_compiler.cpp

class AnimationProcessor {
    const DomainConfig &config_;
    TilesPngWorkspace &workspace_;
    AnimTileMatcher &matcher_;
public:
    ChainableResult<void> register_animations(...);
    AnimKeyframeData build_keyframe_data(...);
    std::vector<Animation> compile_animations(...);
};

class TileAssigner {
    TilesPngWorkspace &workspace_;
    const std::array<Palette<Rgba32, 16>, 16> &palettes_;
public:
    std::optional<TilemapEntry> try_reuse_porymap_tile(...);
    TileAssignmentResult assign_tile_via_pal_match(...);
};

class CompileErrorEmitter {
    const UserDiagnostics &diag_;
    const TilePrinter &tile_printer_;
    const PalettePrinter &pal_printer_;
public:
    void emit_no_matching_tile_error(...);
    void emit_no_matching_pal_error(...);
    void emit_tile_limit_error(...);
};
```

| Pros | Cons |
|------|------|
| Incremental (one group at a time) | Still need dependency passing |
| Groups related state + behavior | Helpers may share some state |
| No public API changes | |
| Stays in anonymous namespace | |

**Verdict**: Best balance of effort vs. improvement. Can be done incrementally.

---

### Option 4: Explicit Data Structs

Replace member variable categories with named structs:

```cpp
struct PorytilesInputData {
    std::vector<Metatile<Rgba32>> metatiles;
    std::vector<PixelTile<Rgba32>> pixel_tiles;
    std::vector<CanonicalPixelTile<Rgba32>> canonical_tiles;
};

struct PorymapInputData { /* similar */ };

struct WorkingState {
    std::array<Palette<Rgba32, 16>, 16> palettes;
    std::unique_ptr<TilesPngWorkspace> workspace;
    AnimTileMatcher matcher;
    std::unique_ptr<PorymapTilesetComponent> output;
};
```

| Pros | Cons |
|------|------|
| Makes data flow explicit | Significant restructuring |
| Self-documenting | May increase copying |
| Enables potential parallelization | Breaks incremental accumulation |

**Verdict**: Good complement to Option 3; can be done together or separately.

---

## Recommended Path

### Short-Term (Low Risk, High Value)

1. **Extract `CompileErrorEmitter`** - Completely orthogonal to compilation logic. Easy win.

2. **Extract `AnimationProcessor`** - Animation logic spans 4 functions (`register_animations`, `build_keyframe_data`, `compile_animations`, plus parts of setup). Relatively self-contained.

### Medium-Term

3. **Extract `TileAssigner`** - Core matching/assignment kernel. Clarifies the main loop.

4. **Introduce data structs** - Group the 5 categories of members into 3-4 named structs.

### Long-Term (Only If Needed)

5. **Strategy pattern for modes** - Only if mode branching becomes unmanageable after the above.

---

## What NOT To Do

- ❌ **Don't split into separate compilation units** - Anonymous namespace benefits (internal linkage, no header pollution) are valuable
- ❌ **Don't create deep inheritance hierarchies** - Adds indirection without proportional benefit
- ❌ **Don't over-abstract** - Some complexity is irreducible; fighting it creates worse code
- ❌ **Don't refactor during active feature development** - Wait for a stabilization period

---

## Key Insight

`CompilerTask` is architecturally sound as a task object pattern. The improvement opportunity is **grouping its ~25 members and ~15 methods into logical clusters** (helper classes + data structs) rather than changing the fundamental approach.

The `pipeline_step_*` / `pipeline_helper_*` naming convention you've established is good—the next step is promoting related helpers into cohesive mini-classes within the same file.
