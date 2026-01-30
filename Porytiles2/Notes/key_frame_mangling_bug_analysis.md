# Key Frame Mangling Bug Analysis

**Date:** 2026-01-30
**Status:** Confirmed Bug
**File:** `Porytiles2/lib/domain/services/animation_decompiler.cpp`
**Lines:** 364-372

---

## Summary

The TODO comment at lines 364-372 in `animation_decompiler.cpp` accurately describes a bug in the duplicate tile detection logic. The current implementation only detects duplicates **within** animation key frame tiles, but fails to detect duplicates **between** animation tiles and non-animation tiles in `tiles.png`.

---

## The TODO Comment

```cpp
/*
 * Check for duplicate tiles within the key frame.
 *
 * TODO: this check doesn't even work quite right. E.g. find_duplicate_tile_pairs is only looking within the
 * key_frame_index_tiles. If tiles.png contains duplicate tiles, one of which is in the anim tile range, we won't
 * actually detect that problem, and the mangler won't be invoked. And when we go to recompile, the compiler will
 * link to the first occurrence of the tile, which may be incorrect if we're in patch mode and the anim version of
 * the tile comes after the non-anim duplicate tile.
 */
```

---

## The Problem Visualized

```
┌─────────────────────────────────────────────────────────────────┐
│                        tiles.png                                │
├─────────────────────────────────────────────────────────────────┤
│  [0..49]          │  [50..59]              │  [60..N]           │
│  Regular tiles    │  Animation keyframes   │  More regular      │
│                   │  (tile_offset=50,      │  tiles             │
│  Tile 30: "ABC"   │   tile_count=10)       │                    │
│       ↑           │                        │                    │
│       │           │  Tile 52: "ABC" ←───── Same content!        │
│       │           │       ↑                │                    │
│       └───────────┼───────┘                │                    │
│                   │  NOT DETECTED!         │                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Why Each Claim is Accurate

### 1. `find_duplicate_tile_pairs` Only Looks Within Key Frame Tiles

From `animation_decompiler.cpp:361-373`:

```cpp
std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
    extract_tiles_from_image(tiles_png, tile_offset, tile_count);
// ...
const auto duplicate_pairs = find_duplicate_tile_pairs(key_frame_index_tiles);
```

The function `find_duplicate_tile_pairs` (lines 268-280) does a simple O(n²) self-comparison:

```cpp
std::vector<std::pair<std::size_t, std::size_t>>
find_duplicate_tile_pairs(const std::vector<PixelTile<IndexPixel>> &tiles)
{
    std::vector<std::pair<std::size_t, std::size_t>> duplicates;
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        for (std::size_t j = i + 1; j < tiles.size(); ++j) {
            if (tiles[i] == tiles[j]) {
                duplicates.emplace_back(i, j);
            }
        }
    }
    return duplicates;
}
```

It **never sees tiles outside the animation range**.

### 2. Duplicates Between Anim and Non-Anim Tiles Go Undetected

The `existing_canonical_tiles` set (built at lines 397-407) is used for **collision avoidance during mangling**, not for **duplicate detection**. The detection happens before this set is even built:

```cpp
// Detection happens FIRST (line 373)
const auto duplicate_pairs = find_duplicate_tile_pairs(key_frame_index_tiles);

// existing_canonical_tiles is only built LATER, inside the mangle branch (lines 397-407)
if (!duplicate_pairs.empty()) {
    switch (key_frame_strategy.value()) {
    case AnimKeyFrameResolutionStrategy::mangle: {
        // NOW we build existing_canonical_tiles...
```

### 3. The Mangler Won't Be Invoked

Since `duplicate_pairs` will be empty when the duplicate is cross-range (animation vs non-animation), the code takes the happy path and skips the entire mangle block.

### 4. The Compiler Links to the First Occurrence

From `TilesPngWorkspace`:

```cpp
// first_occurrence_of() returns the FIRST tile found
std::optional<std::size_t> first_occurrence_of(const CanonicalPixelTile<IndexPixel> &tile) const;
```

The `canonical_forms_` map stores `std::vector<std::size_t>` for each tile pattern, but matching code explicitly calls `first_occurrence_of()`.

### 5. Patch Mode Creates Incorrect Linking

In patch mode, if:
- Non-anim tile at index 30 has pattern "ABC"
- Anim tile at index 52 also has pattern "ABC" (undetected duplicate)

Then during recompilation, any metatile referencing what *should* be the animation tile (52) may incorrectly link to tile 30 because it appears first. The animation won't work correctly.

---

## Code Flow Analysis

### Current Flow (Buggy)

```
1. Extract key frame tiles from animation range
   └── key_frame_index_tiles = extract_tiles_from_image(tiles_png, tile_offset, tile_count)

2. Check for duplicates ONLY within animation tiles
   └── duplicate_pairs = find_duplicate_tile_pairs(key_frame_index_tiles)
   └── MISSES: duplicates between animation tiles and external tiles

3. If duplicates found (only intra-animation):
   └── Build existing_canonical_tiles (external tiles)
   └── Invoke mangler
   └── Backport mangles to tiles.png

4. If NO duplicates found:
   └── Skip mangling entirely
   └── BUG: Cross-range duplicates are not mangled!
```

### Correct Flow (Proposed Fix)

```
1. Extract key frame tiles from animation range
   └── key_frame_index_tiles = extract_tiles_from_image(tiles_png, tile_offset, tile_count)

2. Build canonical set for ALL tiles outside animation range FIRST
   └── external_canonical_tiles = build_external_canonical_set(tiles_png, tile_offset, tile_count)

3. Check for duplicates:
   a. Animation vs external tiles (NEW)
      └── For each tile in key_frame_index_tiles:
          └── If external_canonical_tiles.contains(CanonicalPixelTile{tile}): needs_mangling = true
   b. Intra-animation duplicates (existing)
      └── duplicate_pairs = find_duplicate_tile_pairs(key_frame_index_tiles)

4. If any duplicates found (either type):
   └── Invoke mangler with external_canonical_tiles for collision avoidance
   └── Backport mangles to tiles.png
```

---

## Additional Gap: Canonical Form Checking

There's a subtle additional gap in the current detection:

- **Mangler** uses `CanonicalPixelTile` which accounts for flip-equivalence
- **`find_duplicate_tile_pairs`** uses raw `PixelTile` comparison

This means flip-equivalent tiles within the animation range may also go undetected.

---

## Detection Coverage Matrix

| Scenario | Current Behavior | Correct Behavior |
|----------|------------------|------------------|
| Intra-animation duplicates (exact match) | ✅ Detected | ✅ Detected |
| Intra-animation duplicates (flip-equivalent) | ❌ Not detected | Should be detected |
| Animation vs external (exact match) | ❌ Not detected | Should be detected |
| Animation vs external (flip-equivalent) | ❌ Not detected | Should be detected |

---

## Impact

### When This Bug Manifests

1. User has a tileset with duplicate tiles (common in vanilla games)
2. One duplicate is in the animation tile range, one is outside
3. Decompilation succeeds (no error, no warning)
4. Recompilation in patch mode links to wrong tile
5. Animation appears broken in-game

### Severity

**Medium** - The bug causes silent corruption in patch mode when specific conditions are met. It won't crash, but will produce incorrect output.

---

## Full System Context

### PrimaryTilesetDecompiler Orchestration

The decompilation flow in `PrimaryTilesetDecompiler::decompile()` is **critically ordered**:

```
Input: Tileset (Porymap: IndexPixel)
    ↓
[1] Triple-layerize Porymap component
    └─ Expand dual-layer (8 entries) → triple-layer (12 entries) metatiles
    ↓
[2] Decompile animations (IndexPixel → Rgba32)  ← BUG IS HERE
    └─ Extract animation key frame tiles from tiles_png
    └─ Detect/mangle duplicate key frame tiles
    └─ Backport mangled tiles to Porymap component
    └─ Convert IndexPixel animations to Rgba32 format
    ↓
[3] Decompile metatiles (IndexPixel → Rgba32)
    └─ For each metatile entry, convert IndexPixel tiles to Rgba32
    └─ Apply flips and palette lookup
    ↓
[4] Demetatileize (Metatile<Rgba32> → LayerImages)
    └─ Convert metatiles back to layer pixel images
    ↓
Output: Tileset (Porytiles: Rgba32 + animations)
```

**Critical ordering**: Animation decompilation MUST happen before metatile decompilation so that any mangled tiles are used when decompiling metatiles.

### Tile Type Conversion Points

The IndexPixel → Rgba32 conversion happens at three distinct points:

| Location | Conversion | When |
|----------|------------|------|
| AnimationDecompiler (key frames) | `color_tile_from_index_tile()` | Per animation, after mangling |
| AnimationDecompiler (frames) | `color_tile_from_index_tile()` | Per frame per animation |
| MetatileDecompiler | `color_tile_from_index_tile()` | 12× per metatile |

The conversion order is **correct** - mangling must operate on `IndexPixel` (needs palette index bits), final output needs `Rgba32` (Porytiles format).

### AnimKeyFrameMangler Internals

The mangler correctly uses canonical forms throughout its logic:

**Pixel Modification Priority** (from `anim_key_frame_mangler.cpp:34-104`):
1. Corners first (indices 0, 7, 56, 63) - least visible
2. Edges (top, left, right, bottom)
3. Interior last - most visible

**Mangling Algorithm** (`try_mangle_tile`, lines 176-225):
1. For each pixel in priority order:
   - Skip transparent pixels
   - Extract `color_index` (lower 4 bits) and `palette_index` (upper 4 bits)
   - Find most similar color via RGB distance in palette
   - Create candidate tile with pixel swapped to new color
   - **Canonicalize candidate** to check against existing tiles
   - If unique in canonical form, success
2. Return first successful mangle or `std::nullopt` if all pixels fail

**Key invariant**: The mangler preserves the `palette_index` (upper 4 bits) when swapping colors:
```cpp
const std::size_t new_index = (original_palette_index << 4) | alternative_color.value();
```

### TilesPngWorkspace Tile Linking

**Core data structures**:
- `tiles_`: Vector of `CanonicalPixelTile<IndexPixel>`
- `canonical_forms_`: Map from `PixelTile<IndexPixel>` → `std::vector<std::size_t>`
- `anim_end_offset_`: Boundary between animation slots and regular tiles

**Linking methods**:
- `first_occurrence_of()`: O(1) lookup, returns first index of tile
- `first_occurrence_of_by_color()`: O(n) scan, compares by rendered color (for patch/locked modes)

**Animation tile handling**:
- `reserve_anim_slots()`: Reserves indices 1 through `anim_tile_count` at workspace start
- `place_anim_tile()`: Places tiles in reserved animation region
- Animation tiles are in a contiguous block after tile 0 (transparent)

---

## Related TODOs Found During Analysis

### Animation Decompiler: Per-Subtile Palette Support

**Location**: `animation_decompiler.cpp`, lines 243-246 and 337-340

```cpp
/*
 * TODO: ANIM: adapt this code so that it computes a separate pal index for each subtile of the key frame.
 * Technically, advanced users could make animations where different subtiles use different palettes. None of
 * the vanilla game animations work this way, but it's possible and thus a use-case I want to support.
 */
```

Currently the code assumes a single palette for the entire animation and panics if tiles use multiple palettes.

### AnimKeyFrameMangler: Incomplete Bug Comment

**Location**: `anim_key_frame_mangler.cpp`, lines 247-254

```cpp
/*
 * TODO: I think there is *STILL* actually a potential bug with the mangling process. Here, we are tracking
 * "existing" tiles using the canonical index version. The 'existing_canonical_tiles' variable is set to contain all
 * the tiles from tiles.png, which is supposed to prevent us from creating a mangled tile that matches one of the
 * existing tiles.
 *
 * TODO: what am I saying here?
 */
```

The concern appears to be about whether canonical tile tracking is sufficient for all cases, particularly with true-color mode (upper 4 bits of IndexPixel).

### TilesPngWorkspace: Patch Mode Fragmentation

**Location**: `tiles_png_workspace.cpp`, lines 295-301

```cpp
/*
 * TODO: these methods below work great when tiles are in ArtifactEditMode::optimize.
 * But what about the ArtifactEditMode::patch case? Here, we might have "fragmentation",
 * i.e. free spaces that are too small to fit the contiguous key frame. We won't necessarily
 * be able to reserve space at the start of the workspace, since that space is probably already
 * taken. It's also possible that the tiles we need are already present, this is a patch build
 * after all. We should provide some kind of functionality that scans the workspace for the
 * requisite free space and uses it. This means our anim_end_offset_ variable is probably
 * an over-simplification.
 */
```

### PrimaryTilesetCompiler: Mode Conflict Bug

**Location**: `primary_tileset_compiler.cpp`, lines 185-196

```cpp
/*
 * TODO: we have a bug. If pals_edit_mode::optimize and tiles_edit_mode::locked,
 * on the first compile pass after making no changes, it will optimize the pals,
 * but emit identical metatile entries (since the Porytiles and Porymap metatiles
 * will match). Then it will emit the optimized pals but identical tilemap entries.
 * The tileset becomes corrupted and subsequent compilations crash out with a
 * bazillion errors.
 */
```

---

## Proposed Fix Implementation

### New Helper Functions

Add to anonymous namespace in `animation_decompiler.cpp`:

```cpp
/**
 * @brief Categorizes duplicates by type for detailed error reporting.
 */
struct DuplicateInfo {
    std::vector<std::size_t> cross_range_indices;  // Anim tiles matching external
    std::vector<std::pair<std::size_t, std::size_t>> intra_animation_pairs;
};

/**
 * @brief Detects both cross-range and intra-animation duplicates using canonical forms.
 *
 * @details
 * This function performs two kinds of duplicate detection:
 * 1. Cross-range: Checks if any animation tile's canonical form exists in the external tiles set
 * 2. Intra-animation: Checks if any two animation tiles have the same canonical form
 *
 * Both checks use CanonicalPixelTile to ensure flip-equivalent tiles are treated as duplicates.
 *
 * @param key_frame_tiles The animation key frame tiles to check
 * @param external_canonical_tiles Set of canonical tiles from outside the animation range
 * @return True if any duplicates were found (either type), false otherwise
 */
bool has_any_duplicates(
    const std::vector<PixelTile<IndexPixel>> &key_frame_tiles,
    const std::set<PixelTile<IndexPixel>> &external_canonical_tiles)
{
    std::set<PixelTile<IndexPixel>> seen_canonical_tiles;

    for (const auto &tile : key_frame_tiles) {
        const CanonicalPixelTile canonical{tile};
        const PixelTile<IndexPixel> &base = canonical;

        // Check for cross-range duplicate
        if (external_canonical_tiles.contains(base)) {
            return true;
        }

        // Check for intra-animation duplicate (flip-aware)
        if (seen_canonical_tiles.contains(base)) {
            return true;
        }

        seen_canonical_tiles.insert(base);
    }

    return false;
}

/**
 * @brief Categorizes all duplicates for detailed error messaging.
 */
DuplicateInfo categorize_duplicates(
    const std::vector<PixelTile<IndexPixel>> &key_frame_tiles,
    const std::set<PixelTile<IndexPixel>> &external_canonical_tiles)
{
    DuplicateInfo info;
    std::map<PixelTile<IndexPixel>, std::size_t> canonical_first_occurrence;

    for (std::size_t i = 0; i < key_frame_tiles.size(); ++i) {
        const CanonicalPixelTile canonical{key_frame_tiles[i]};
        const PixelTile<IndexPixel> &base = canonical;

        // Check for cross-range duplicate
        if (external_canonical_tiles.contains(base)) {
            info.cross_range_indices.push_back(i);
        }

        // Check for intra-animation duplicate
        auto it = canonical_first_occurrence.find(base);
        if (it != canonical_first_occurrence.end()) {
            info.intra_animation_pairs.emplace_back(it->second, i);
        } else {
            canonical_first_occurrence[base] = i;
        }
    }

    return info;
}
```

### Refactored Control Flow

Replace lines 361-434 in `decompile_animation()`:

```cpp
// 1. Extract key frame tiles (unchanged)
std::vector<PixelTile<IndexPixel>> key_frame_index_tiles =
    extract_tiles_from_image(tiles_png, tile_offset, tile_count);

// 2. Build external canonical tiles FIRST (moved from inside mangle branch)
const std::size_t total_tiles =
    (tiles_png.height() / tile::side_length_pix) * (tiles_png.width() / tile::side_length_pix);
std::set<PixelTile<IndexPixel>> external_canonical_tiles;
for (std::size_t i = 0; i < total_tiles; ++i) {
    if (i >= tile_offset && i < tile_offset + tile_count) {
        continue;  // Skip animation range
    }
    const CanonicalPixelTile canonical{extract_single_tile(tiles_png, i)};
    const PixelTile<IndexPixel> &base = canonical;
    external_canonical_tiles.insert(base);
}

// 3. Check for ANY duplicates (cross-range OR intra-animation, using canonical forms)
const bool needs_resolution = has_any_duplicates(key_frame_index_tiles, external_canonical_tiles);

// 4. Handle duplicates if found
if (needs_resolution) {
    switch (key_frame_strategy.value()) {
    case AnimKeyFrameResolutionStrategy::error: {
        const auto dup_info = categorize_duplicates(key_frame_index_tiles, external_canonical_tiles);
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag_->formatter().format(
            "Animation '{}' has duplicate key frame tiles:", FormatParam{anim.name(), Style::bold}));

        // Report cross-range duplicates
        for (const auto idx : dup_info.cross_range_indices) {
            err_msg.emplace_back(diag_->formatter().format(
                "  - tile {} duplicates an existing tile outside the animation range",
                FormatParam{idx, Style::bold}));
        }

        // Report intra-animation duplicates
        for (const auto &[i, j] : dup_info.intra_animation_pairs) {
            err_msg.emplace_back(diag_->formatter().format(
                "  - tile {} and tile {} are identical (or flip-equivalent)",
                FormatParam{i, Style::bold}, FormatParam{j, Style::bold}));
        }

        err_msg.emplace_back("");
        err_msg.emplace_back("Consider using 'mangle' strategy to auto-resolve.");
        std::ranges::copy(
            format_config_note_with_separator(diag_->formatter(), key_frame_strategy), std::back_inserter(err_msg));
        return FormattableError{err_msg};
    }

    case AnimKeyFrameResolutionStrategy::mangle: {
        // external_canonical_tiles already built, use it directly
        AnimKeyFrameMangler mangler{diag_, tile_printer_};
        PT_TRY_ASSIGN_CHAIN_ERR(
            mangle_result,
            mangler.mangle_duplicates(
                anim.name(),
                std::move(key_frame_index_tiles),
                pal,
                extrinsic_transparency.value(),
                external_canonical_tiles),
            diag_->formatter().format(
                "Failed to mangle duplicate key frame tiles for animation '{}'.",
                FormatParam{anim.name(), Style::bold}),
            Animation<Rgba32>);
        key_frame_index_tiles = std::move(mangle_result.tiles);

        // Backport changes to tiles.png
        if (porymap_component != nullptr && !mangle_result.mangle_records.empty()) {
            backport_mangles_to_tiles_png(porymap_component, tile_offset, mangle_result.mangle_records);
        }
        break;
    }

    default:
        panic("unhandled AnimKeyFrameResolutionStrategy value");
    }
}
```

---

## Related Files

- `Porytiles2/lib/domain/services/animation_decompiler.cpp` - Main bug location
- `Porytiles2/lib/domain/services/anim_key_frame_mangler.cpp` - Mangling logic
- `Porytiles2/lib/domain/workspace/tiles_png_workspace.cpp` - Tile linking logic
- `Porytiles2/lib/domain/compiler/primary_tileset_compiler.cpp` - Compilation flow
- `Porytiles2/include/porytiles2/domain/models/canonical_pixel_tile.hpp` - Flip-equivalence logic

---

## Test Cases Needed

1. **Cross-range exact duplicate**: Non-anim tile at index 30 matches anim tile at index 52
2. **Cross-range flip duplicate**: Non-anim tile is h-flip of anim tile
3. **Intra-animation flip duplicate**: Two anim tiles are flip-equivalent
4. **No duplicates**: Baseline case, ensure no false positives
5. **Patch mode recompilation**: Verify correct tile linking after fix

---

## Implementation Notes

### Backward Compatibility

This fix **changes behavior** - previously undetected duplicates will now be flagged/mangled. This is the correct behavior but could surprise users with tilesets that "worked" before (silently corrupting in patch mode).

### Performance

Building the external tile set adds an O(n) scan of all tiles outside the animation range. This is acceptable:
- The scan was already done in the mangle branch (just moved earlier)
- `tiles.png` is typically small (under 1000 tiles)
- This is a one-time cost per animation during decompilation

### Why Not Simplify the Conversion Flow?

Analysis confirmed the IndexPixel → Rgba32 conversion order is **correct and necessary**:
- Mangling needs `IndexPixel` (must preserve and manipulate palette index bits)
- Final Porytiles output needs `Rgba32` (the format Porytiles uses)
- No simplification opportunity exists here
