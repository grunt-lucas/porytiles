# Tile Sharing: The Indirect Link System

## Overview

This document explains how Porytiles2's tile sharing system works, with a focus on
the Indirect link mechanism that enables palette slot alignment across palettes.

## The Problem: Why Tile Sharing Matters

On the GBA, each 8x8 tile in `tiles.png` is a **4-bit indexed image** — each pixel
is a slot index (0–15) into one of the hardware palettes. A single tile image can be
reused across multiple metatile positions by assigning it different palettes at
runtime. The GBA just swaps which palette the indices point into.

But this only works if the **colors line up at the same slot indices** across
palettes. Consider two metatile subtiles that have identical shapes (same pixel
pattern) but different colors:

```
Tile A (uses palette 0):        Tile B (uses palette 1):
  pixel pattern: ██░░██░░        pixel pattern: ██░░██░░  (same shape!)
  color: red                     color: blue
```

If red is at **slot 3** in palette 0, and blue is also at **slot 3** in palette 1,
then a single indexed tile (with pixels = `3,3,0,0,3,3,0,0`) renders correctly with
either palette. That's tile sharing — one tile image, two palettes, correct rendering
for both.

If blue ends up at **slot 7** instead, you'd need two separate tile images. That
wastes tile slots (there's a hard cap of ~512 for primary tilesets).

## Shape Groups

The system starts by identifying **shape groups** — sets of tiles that have the same
pixel shape but different colors. This is done by `analyze_shape_groups()`.

### Data Structures

**`ShapeGroup`** (`domain/models/shape_group.hpp`): Groups tiles with the same
canonical shape but different colors.

```c++
template <typename PixelType>
struct ShapeGroup {
    ShapeTile<PixelType> canonical_shape;              // The shared pixel pattern
    std::vector<ShapeGroupMember<PixelType>> members;  // Tiles sharing this shape
};
```

**`ShapeGroupMember`** (`domain/models/shape_group.hpp`): One tile in a shape group,
with its color mapping.

```c++
template <typename PixelType>
struct ShapeGroupMember {
    std::size_t tile_index;
    std::map<ShapeMask, PixelType> colors;  // ShapeMask → color for each region
    bool h_flip;
    bool v_flip;
};
```

The `ShapeMask` is the canonical identifier for "which pixels in the tile are this
color." Two members of the same group have the **same set of ShapeMask keys** but
potentially different color values. That correspondence is what the system needs to
preserve across palettes.

## The Indirect Link System

The system needs to ensure that corresponding colors across palettes land at the same
slot index, **without knowing the final slot positions in advance** (because colors
haven't been assigned to slots yet when links are created).

### Core Data Structures

**`ColorPosition`** (`domain/packing/models/color_position.hpp`): A variant tracking
a color's state during palette construction.

```c++
using ColorPosition = std::variant<UndeterminedPosition, AbsolutePosition, IndirectPosition>;
```

The three states:

```c++
struct UndeterminedPosition {};               // Not yet assigned a slot

struct AbsolutePosition {                      // Locked to a specific slot
    std::size_t slot;
};

struct IndirectPosition {                      // "My slot is wherever ref_color is in ref_pal"
    std::size_t ref_pal_index;
    Rgba32 ref_color;
};
```

**`IndirectLink`** (`domain/packing/models/color_position.hpp`): The instruction that
drives sharing alignment.

```c++
struct IndirectLink {
    std::size_t source_pal;         // "in this palette..."
    Rgba32 source_color;            // "...this color..."
    std::size_t ref_pal;            // "...should be at the same slot as..."
    Rgba32 ref_color;               // "...this color in this other palette"
    std::size_t source_group_index; // (for diagnostics)
};
```

The critical design choice: **links reference colors, not slots.** As the Doxygen
comment says: *"by referencing colors rather than absolute slots, links remain valid
even when sequential fill places non-shared colors at different positions than the
base palettes."*

## The Orchestration Flow

The full flow lives in `palette_packer.cpp` (see the comment block in the packer's
`pack_tiles()` method):

```c++
/*
 * The Indirect approach replaces the old two-pass absolute-slot constraint system:
 *  - Step 1: Build base palettes (sequential fill, no links) — needed for
 *            match_or_best matching
 *  - Step 2: Generate Indirect links (color-to-color references, not absolute
 *            slot targets)
 *  - Step 3: Build final palettes with Indirect links applied
 *  - Step 4: Build sharing diagnostics against final palettes (verified alignment)
 *
 * Key architectural insight: Indirect links reference colors, not slots. When
 * the final palettes are built, sequential fill may place non-shared colors at
 * different slots than the base palettes — but this doesn't matter because
 * Indirect links say "my slot is wherever that color ends up," and that color
 * gets a stable Absolute position during the final palette's own sequential fill.
 */
```

### Step 1: Build Base Palettes

(`palette_packer.cpp` — the `build_all_output_palettes` call with empty links)

```c++
auto base_pals = build_all_output_palettes(
    packing_output.pals_,
    params.prefilled_pals_,
    params.color_map_,
    params.extrinsic_transparency_,
    {} /* empty links */);
```

These are "draft" palettes — colors land wherever sequential fill puts them. We need
them so `match_or_best()` can determine which tile ended up in which palette.

### Step 2: Generate IndirectLinks

(`palette_packer.cpp`)

```c++
indirect_links = build_indirect_links(
    shape_groups, combined.tiles, base_pals,
    params.prefilled_pals_, params.extrinsic_transparency_);
```

See the [IndirectLink Generation](#indirectlink-generation) section below for details.

### Step 3: Build Final Palettes

(`palette_packer.cpp`)

```c++
auto final_pals = build_all_output_palettes(
    packing_output.pals_,
    params.prefilled_pals_,
    params.color_map_,
    params.extrinsic_transparency_,
    indirect_links);  // <-- this time with links!
```

Same function as Step 1, but with non-empty `indirect_links`. This runs the full
five-phase algorithm described below.

### Step 4: Build Sharing Diagnostics

(`palette_packer.cpp`)

Builds `SharingResult` objects by verifying each shape group's members against the
**final** palettes. For each member, it indexes the tile against its matched final
palette and canonicalizes the result. Members whose canonical indexed tiles match the
reference member's are confirmed as sharing-aligned and added to the result. Groups
with 2+ verified members spanning 2+ palettes are recorded in
`packing.sharing_results_`.

## IndirectLink Generation

(`domain/packing/algorithms/indirect_link_builder.cpp`)

For each shape group:

### 1. Resolve Each Member's Palette

```c++
for (std::size_t m = 0; m < group.members.size(); ++m) {
    const auto &member = group.members.at(m);
    const auto &tile = tiles.at(member.tile_index);

    auto matches = match_or_best(tile, flat_pals, extrinsic, 1);

    if (matches.empty() || !matches.at(0).is_covered) {
        continue;  // tile doesn't fully match any palette — skip
    }

    std::size_t hw_index = flat_to_hw_index.at(matches.at(0).pal_index);
    resolved.push_back(ResolvedMember{m, hw_index, member.colors});
}
```

### 2. Skip Trivial Groups

Groups with <2 resolved members or all members in the same palette have no sharing
opportunity:

```c++
if (resolved.size() < 2) {
    continue;
}
// ...
if (distinct_pals.size() < 2) {
    continue;  // All in same palette — sharing is trivially satisfied
}
```

### 3. Pick the Best Reference Member (Conflict-Minimization Heuristic)

For each candidate reference, count how many of its links would target prefilled
(locked) slots in other palettes. Pick the candidate with fewest conflicts:

```c++
std::size_t best_ref_index = 0;
std::size_t best_ref_conflicts = std::numeric_limits<std::size_t>::max();

for (std::size_t candidate_ref = 0; candidate_ref < resolved.size(); ++candidate_ref) {
    // ...build candidate_mask_to_slot from base palette...

    std::size_t conflicts = 0;
    for (std::size_t other = 0; other < resolved.size(); ++other) {
        // ...for each other member in a different palette with prefilled slots...
        std::size_t target_slot = candidate_mask_to_slot.at(mask);
        if (!prefilled.is_wildcard(target_slot)) {
            ++conflicts;  // This link would hit a locked slot
        }
    }

    if (conflicts < best_ref_conflicts) {
        best_ref_conflicts = conflicts;
        best_ref_index = candidate_ref;
    }
}
```

Why this matters: the reference member's slot layout becomes the "truth" that all
other members align to. Choosing a reference whose slots conflict with prefilled
constraints in other palettes would create unresolvable alignment failures.

### 4. Emit IndirectLinks

For each non-reference member in a different palette, create one link per ShapeMask:

```c++
for (const auto &[mask, other_color] : other.colors) {
    if (!ref.colors.contains(mask)) {
        continue;
    }
    const auto &ref_color = ref.colors.at(mask);

    links.push_back(
        IndirectLink{
            .source_pal = other.hw_pal_index,
            .source_color = other_color,
            .ref_pal = ref.hw_pal_index,
            .ref_color = ref_color,
            .source_group_index = group_idx,
        });
}
```

The correspondence is through the `ShapeMask`: same mask key → same pixel region →
colors must share a slot index.

## The Five-Phase Palette Builder

(`domain/packing/algorithms/palette_builder.cpp`)

This is the heart of the system. It takes packed palettes + IndirectLinks and
produces final palettes with sharing alignment.

### Phase 1 — Initialize Position Maps

For each palette, set up the initial `ColorPosition` state:

```c++
// Prefilled colors → AbsolutePosition (locked)
if (!prefilled_ptr->is_wildcard(i)) {
    already_placed_colors.insert(prefilled_ptr->at(i));
    state.color_positions[prefilled_ptr->at(i)] = AbsolutePosition{i};
}

// Packed palette colors → UndeterminedPosition (to be assigned)
if (!already_placed_colors.contains(color)) {
    state.color_positions[color] = UndeterminedPosition{};
}
```

After Phase 1, every color is either `AbsolutePosition` (prefilled) or
`UndeterminedPosition` (needs a slot).

### Phase 2 — Apply Indirect Links

For each IndirectLink, convert the source color from `Undetermined` to
`IndirectPosition`:

```c++
for (const auto &link : indirect_links) {
    // ...
    auto &position = state.color_positions.at(link.source_color);
    // First-writer-wins: only set Indirect on Undetermined positions (prevents cycles)
    if (std::holds_alternative<UndeterminedPosition>(position)) {
        position = IndirectPosition{link.ref_pal, link.ref_color};
    }
}
```

First-writer-wins is the cycle prevention mechanism. If two shape groups both try to
link the same color, only the first link takes effect.

After Phase 2, colors are in one of three states:

- `AbsolutePosition` — prefilled, slot is final
- `IndirectPosition` — "follow this other color"
- `UndeterminedPosition` — not linked, needs sequential fill

### Phase 3 — Sequential Fill (Skip Indirect)

Assign `AbsolutePosition` to every remaining `Undetermined` color. **Crucially,
Indirect colors are skipped**:

```c++
for (auto &[color, position] : state.color_positions) {
    if (!std::holds_alternative<UndeterminedPosition>(position)) {
        continue;  // skips both Absolute AND Indirect
    }
    // ...find next free slot...
    position = AbsolutePosition{next_slot};
}
```

This is the key ordering insight described in the Phase 3 comment:

```c++
/*
 * Every Undetermined color gets an Absolute slot. Indirect colors are left
 * untouched — they'll be resolved in Phase 4. After this phase, all reference
 * colors (which are Undetermined, not Indirect) have stable Absolute positions,
 * enabling Indirect chain resolution.
 */
```

Reference colors are always `Undetermined` (the link builder never creates links FROM
the reference), so they get stable `AbsolutePosition` assignments here. This is what
makes Phase 4 work — by the time we resolve Indirect chains, there's always an
`AbsolutePosition` at the end of the chain.

### Phase 4 — Resolve Indirect Chains with Eviction

First, resolve each `IndirectPosition` to its target slot via
`try_resolve_indirect()`:

```c++
[[nodiscard]] std::optional<std::size_t> try_resolve_indirect(
    const IndirectPosition &start,
    const std::array<std::optional<PaletteBuildState>, pal::num_pals> &states)
{
    IndirectPosition current = start;
    for (std::size_t iter = 0; iter < pal::num_pals; ++iter) {
        if (!states.at(current.ref_pal_index).has_value()) {
            return std::nullopt;
        }
        const auto &ref_state = states.at(current.ref_pal_index).value();
        if (!ref_state.color_positions.contains(current.ref_color)) {
            return std::nullopt;
        }
        const auto &ref_position = ref_state.color_positions.at(current.ref_color);

        if (std::holds_alternative<AbsolutePosition>(ref_position)) {
            return std::get<AbsolutePosition>(ref_position).slot;  // found it!
        }
        else if (std::holds_alternative<IndirectPosition>(ref_position)) {
            current = std::get<IndirectPosition>(ref_position);  // follow the chain
        }
        else {
            return std::nullopt;  // Undetermined — reference not yet filled
        }
    }
    panic("Indirect chain resolution exceeded maximum iterations (cycle detected)");
}
```

Chains can be multi-hop: color A in pal 1 → color B in pal 2 → color C in pal 0 →
`AbsolutePosition{slot=3}`. The loop cap at `pal::num_pals` iterations detects
cycles.

Then, apply resolutions with eviction. If the target slot is occupied by a
non-prefilled color from sequential fill, evict it:

```c++
for (const auto &[indirect_color, target_slot] : resolutions) {
    // Check if target slot is occupied
    for (auto &[color, position] : state.color_positions) {
        if (color == indirect_color) {
            continue;  // Don't evict ourselves
        }
        if (std::holds_alternative<AbsolutePosition>(position) &&
            std::get<AbsolutePosition>(position).slot == target_slot &&
            !state.prefilled_slots.contains(target_slot)) {
            evicted_color = color;
            needs_eviction = true;
            break;
        }
    }

    if (needs_eviction) {
        // Move the evicted color to the next free slot
        state.color_positions.at(evicted_color) = AbsolutePosition{free_slot};
    }

    // Place the Indirect color at the target slot
    state.color_positions.at(indirect_color) = AbsolutePosition{target_slot};
}
```

Two best-effort skip conditions:

- **Prefilled target slot**: Can't evict hardware-locked colors. The Indirect color
  stays wherever sequential fill would have put it.
- **Broken chain**: Reference color not found or not yet resolved. Skipped silently.

### Phase 5 — Fallback Fill for Unresolved Indirects

Phase 4 may leave colors in `IndirectPosition` if resolution failed (broken chain,
prefilled destination conflict, or no free slot for eviction). These colors still need
placement in the final palette, so assign them sequential free slots — identical to
Phase 3's logic but targeting `IndirectPosition` instead of `UndeterminedPosition`.
This ensures all colors get placed even if Indirect alignment fails.

### Phase 6 — Build Final Palettes

Straightforward materialization — place every `AbsolutePosition` color at its
resolved slot:

```c++
for (const auto &[color, position] : state.color_positions) {
    if (std::holds_alternative<AbsolutePosition>(position)) {
        const std::size_t slot = std::get<AbsolutePosition>(position).slot;
        if (!state.prefilled_slots.contains(slot)) {
            output.set(slot, color);
        }
    }
}
```

## Aggressive vs. Opportunistic Mode

Both modes use the same Indirect link pipeline. The difference is **before packing**:

- **Opportunistic**: Pack tiles normally, then analyze shape groups and apply Indirect
  links post-hoc. Whatever sharing opportunities fall out naturally get aligned.

- **Aggressive**: Before packing, compute `ShapeGroupMetadata` mapping tiles to their
  shape groups. The packing strategy (backtracking, etc.) uses this metadata to
  **penalize placing sibling tiles in the same palette**, maximizing the chance that
  shape group members end up in different palettes — creating more sharing
  opportunities for the Indirect system to capitalize on.

Both converge at the same link-building and diagnostic flow in `palette_packer.cpp`.

## Key Design Insight

The Indirect system solves a subtle **circular dependency** problem. You can't know
final slot positions until you build the palette, but you need sharing constraints to
build the palette correctly.

The solution is **indirection through colors**: links say "these two colors must share
a slot" without specifying *which* slot. Phase 3 assigns slots to reference colors
first (they're `Undetermined`, not `Indirect`). Phase 4 then follows the Indirect
chains to find those stable slots and places the dependent colors there, evicting
sequential-fill occupants as needed.

Prefilled slots are the only hard constraint that can prevent alignment — and the
conflict-minimization heuristic in the link builder specifically tries to avoid them.
