# Tile Sharing: The Indirect Link System

## Overview

This document explains how Porytiles's tile sharing system works, with a focus on
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
    std::size_t source_group_index;           // Shape group origin (for diagnostics)
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

The full flow lives in `palette_packer.cpp`'s `pack_tiles()` method. The method uses
**step-numbered comments** for the packing pipeline and **phase-numbered comments** for
the three-phase sharing diagnostics. When `tile_sharing_alignment_` is set to `greedy`,
the Indirect link system activates.

### Steps 1–4: Input Conversion and Packing

- **Steps 1–3** (`build_packing_inputs()`): Convert regular tiles and animations to
  `PackableTile` vectors, convert hint palettes, and convert prefilled palettes to
  `PrefilledPalette` constraints.
- **Pre-packing (biased mode only)**: When `tile_sharing_packing_` is `biased`, shape
  groups are computed **before** the pack call so the strategy receives sharing-aware
  metadata:

```c++
if (params.tile_sharing_packing_ == TileSharingPacking::biased) {
    combined_for_biased = build_combined_tiles(params, anim_keyframe_tiles);
    shape_groups_for_biased = analyze_shape_groups(combined_for_biased.tiles, params.extrinsic_transparency_);

    if (!shape_groups_for_biased.empty()) {
        auto metadata = build_shape_group_metadata(shape_groups_for_biased, combined_for_biased.index_to_id);
        packing_input.shape_group_metadata_ = std::move(metadata);
    }
}
```

- **Step 4**: Call `strategy_->pack(packing_input)` to execute the low-level packing
  strategy.

### Step 5a: Tile-to-Palette Assignment

`populate_tile_to_pal()` extracts tile→palette mappings from `PackingOutput`. Only
`RegularId` tiles are recorded in the final output; `AnimId`, `HintId`, and prefilled
entries are skipped.

### Step 5b: Shape Groups, Diagnostics, and Indirect Link Pipeline

Shape groups are always computed for diagnostics (reusing pre-pack results when packing
is biased). The three diagnostic phases and indirect link generation all live within
this step:

**Diagnostic Phase 1** — Detect sharing opportunities: identifies shape groups with
potential for tile sharing.

**Diagnostic Phase 2** — Compute partition groups: filters shape groups to "eligible"
ones (2+ members in 2+ distinct palettes after packing).

**Build Indirect Links (Greedy Alignment Only)**:

The `tile_pal_assignments` map is built from the authoritative packing output — it maps
each combined tile index to its hardware palette index. When `greedy` alignment is
active and shape groups exist, base palettes are built first, then links are generated:

```c++
if (params.tile_sharing_alignment_ == TileSharingAlignment::greedy && !shape_groups.empty()) {
    auto base_pals = build_all_output_palettes(
        packing_output.pals_,
        params.prefilled_pals_,
        params.color_map_,
        params.extrinsic_transparency_,
        {} /* empty links */);

    indirect_links = build_indirect_links(
        shape_groups, tile_pal_assignments, base_pals, params.prefilled_pals_);
}
```

The base palettes are "draft" palettes — colors land wherever sequential fill puts
them. They exist so the conflict-minimization heuristic in the link builder can look up
each ShapeMask's slot position in the reference member's base palette.

**Build Final Palettes**:

```c++
AlignmentFailureCounts failure_counts{};
auto final_pals = build_all_output_palettes(
    packing_output.pals_,
    params.prefilled_pals_,
    params.color_map_,
    params.extrinsic_transparency_,
    indirect_links,
    !indirect_links.empty() ? &failure_counts : nullptr);
```

Same function as the base palette build, but with non-empty `indirect_links` (or empty
links when alignment is `off`). When links are present, the optional
`AlignmentFailureCounts` pointer enables failure detail recording. This runs the full
six-phase algorithm described in the
[Six-Phase Palette Builder](#the-six-phase-palette-builder) section below.

**Diagnostic Phase 3** — Verify sharing alignment: checks each eligible shape group's
members against the **final** palettes via `verify_sharing_alignment()`. For each
member, it indexes the tile against its assigned final palette and canonicalizes the
result. Members whose canonical indexed tiles match the reference member's are confirmed
as sharing-aligned. Groups are categorized as fully aligned, partially aligned (some
members diverged), or unaligned.

**Sharing Summary** — Emitted only when `biased` packing or `greedy` alignment is
active. Includes detected → eligible → aligned breakdown, per-group failure details, and
aggregate failure counts by category.

## IndirectLink Generation

(`domain/packing/algorithms/indirect_link_builder.cpp`)

For each shape group:

### 1. Resolve Each Member's Palette

Each member's palette is determined from `tile_pal_assignments` — a map built from the
authoritative packing output that maps each tile index to its hardware palette index.
This is more reliable than re-matching against base palettes because it uses the actual
packing decisions.

```c++
for (std::size_t m = 0; m < group.members.size(); ++m) {
    const auto &member = group.members.at(m);
    if (!tile_pal_assignments.contains(member.tile_index)) {
        continue;  // tile wasn't packed — skip
    }
    std::size_t hw_index = tile_pal_assignments.at(member.tile_index);
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

## Alignment Failure Tracking

(`domain/packing/algorithms/palette_builder.hpp`)

The six-phase palette builder optionally populates an `AlignmentFailureCounts` struct
that records exactly why alignment failed for specific colors. This enables the sharing
summary to provide actionable diagnostics.

```c++
struct AlignmentFailureCounts {
    std::vector<PrefilledDestinationConflictDetail> prefilled_destination_conflict_details;
    std::vector<PrefilledSourceConflictDetail> prefilled_source_conflict_details;
    std::vector<FirstWriterWinsDetail> first_writer_wins_details;
    std::vector<PostResolutionMismatchDetail> post_resolution_mismatch_details;

    [[nodiscard]] std::size_t total() const;
};
```

The four failure types:

- **`PrefilledDestinationConflictDetail`** (Phase 4): An Indirect color resolved to a
  slot that is prefilled (locked) in its own palette. The color cannot be placed at the
  target slot, so alignment fails for that link. Records the blocked color, the locked
  color occupying the slot, and the target slot index.

- **`PrefilledSourceConflictDetail`** (Phase 2): The source color of an IndirectLink is
  itself prefilled (locked to an `AbsolutePosition` in Phase 1). The link cannot
  override a hardware-locked position. However, if the reference color is also prefilled
  at the **same** slot, alignment is naturally satisfied and no failure is recorded.

- **`FirstWriterWinsDetail`** (Phase 2): Two shape groups both try to set Indirect
  links on the same source color with **incompatible** references (different ref_pal or
  ref_color). The first link wins; the second is dropped. Records both the winning and
  losing reference info.

- **`PostResolutionMismatchDetail`** (post-Phase 5): After all phases complete, a
  successfully applied Indirect link's source and reference colors ended up at
  **different** final slots. This can happen when a later eviction displaces a
  previously placed color. Records both final slot values for diagnosis.

## The Six-Phase Palette Builder

(`domain/packing/algorithms/palette_builder.cpp`)

This is the heart of the system. It takes packed palettes + IndirectLinks and
produces final palettes with sharing alignment. The full signature:

```c++
[[nodiscard]] std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals>
build_all_output_palettes(
    const std::vector<PackedPalette> &packed_pals,
    const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &prefilled_pals,
    const ColorIndexMap<Rgba32> &color_map,
    const Rgba32 &default_slot_zero,
    const std::vector<IndirectLink> &indirect_links,
    AlignmentFailureCounts *failure_counts = nullptr);
```

The optional `failure_counts` pointer enables detailed failure recording when non-null.
An internal `AppliedIndirect` struct tracks which links were successfully applied, used
later for post-resolution verification.

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

For each IndirectLink, attempt to convert the source color to `IndirectPosition`. The
source color's current state determines the outcome — there are three branches:

```c++
for (const auto &link : indirect_links) {
    auto &position = state.color_positions.at(link.source_color);

    if (std::holds_alternative<UndeterminedPosition>(position)) {
        // Normal case: set to Indirect
        position = IndirectPosition{link.ref_pal, link.ref_color, link.source_group_index};
    }
    else if (std::holds_alternative<IndirectPosition>(position)) {
        const auto &existing = std::get<IndirectPosition>(position);
        if (existing.ref_pal_index == link.ref_pal && existing.ref_color == link.ref_color) {
            // Compatible duplicate: same reference — record for post-verification
            applied_indirects.push_back(/* ... */);
        }
        else {
            // Incompatible: first writer wins — record FirstWriterWinsDetail failure
        }
    }
    else if (std::holds_alternative<AbsolutePosition>(position)) {
        // Source is prefilled (locked). Check if ref color is also at the same slot.
        // If so, alignment is naturally satisfied — no failure recorded.
        // Otherwise, record PrefilledSourceConflictDetail failure.
    }
}
```

The three branches handle:

1. **`UndeterminedPosition`** — Normal case. The color becomes `IndirectPosition`,
   pointing at the reference color in the reference palette.

2. **`IndirectPosition`** (already linked) — Two sub-cases:
   - **Compatible**: The existing link targets the same ref_pal and ref_color. No
     conflict — the link is recorded in `applied_indirects` for post-verification.
   - **Incompatible**: Different reference. First-writer-wins: the existing link stays,
     the new link is dropped, and a `FirstWriterWinsDetail` failure is recorded.

3. **`AbsolutePosition`** (prefilled) — The source color is hardware-locked from Phase
   1. The link can't override a prefilled position. However, if the reference color in
   the reference palette is *also* `AbsolutePosition` at the **same slot**, alignment
   is naturally satisfied and no failure is recorded. Otherwise, a
   `PrefilledSourceConflictDetail` failure is recorded.

First-writer-wins is the cycle prevention mechanism. Since the link builder may emit
links from multiple shape groups targeting the same source color, only the first link
takes effect.

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
for (const auto &[indirect_color, target_slot, source_group_index, res_ref_pal, res_ref_color] :
     resolutions) {
    // Check if the target slot is occupied by a sequential-fill color
    for (auto &[color, position] : state.color_positions) {
        if (color == indirect_color) {
            continue;
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
        // Rebuild all_used set and find next free slot for evicted color
        // Panics if no free slot is available (invariant: Phase 3 guarantees space)
        state.color_positions.at(evicted_color) = AbsolutePosition{free_slot};
    }

    // Place the Indirect color at the target slot
    state.color_positions.at(indirect_color) = AbsolutePosition{target_slot};

    // Record successfully resolved link for post-resolution verification
    applied_indirects.push_back(
        AppliedIndirect{pal_index, indirect_color, res_ref_pal, res_ref_color, source_group_index});
}
```

There is one skip condition:

- **Prefilled target slot**: Can't evict hardware-locked colors. The Indirect color is
  left in `IndirectPosition` state (Phase 5 will assign it a fallback slot). A
  `PrefilledDestinationConflictDetail` failure is recorded, capturing the blocked color
  and the locked color occupying the slot.

Note: broken chains (where `try_resolve_indirect` returns `nullopt`) are **not** a
best-effort skip — they trigger a panic. By design, all reference colors have stable
`AbsolutePosition` assignments from Phase 3, so resolution should never fail. A
`nullopt` return indicates an internal invariant violation.

### Phase 5 — Fallback Fill for Unresolved Indirects

Phase 4 may leave colors in `IndirectPosition` if resolution failed (prefilled
destination conflict). These colors still need
placement in the final palette, so assign them sequential free slots — identical to
Phase 3's logic but targeting `IndirectPosition` instead of `UndeterminedPosition`.
This ensures all colors get placed even if Indirect alignment fails.

### Post-Resolution Verification

After Phase 5, the builder checks all successfully applied Indirect links to detect
**eviction displacement** — cases where a later eviction in Phase 4 moved a previously
placed color, breaking an earlier link's alignment:

```c++
for (const auto &ai : applied_indirects) {
    const auto source_slot = std::get<AbsolutePosition>(source_pos).slot;
    const auto ref_slot = std::get<AbsolutePosition>(ref_pos).slot;

    if (source_slot != ref_slot) {
        failure_counts->post_resolution_mismatch_details.push_back(
            PostResolutionMismatchDetail{/* ... source and ref slots, colors, groups ... */});
    }
}
```

After verification, all four failure detail vectors are **deduplicated** (sorted then
`std::ranges::unique`) to eliminate duplicate records that can arise when multiple
IndirectLinks for the same color pair (from different group members) produce identical
detail entries.

### Phase 6 — Build Final Palettes

Straightforward materialization with three placement passes:

```c++
// 1. Slot 0: use prefilled value if available, otherwise default_slot_zero
if (prefilled_ptr != nullptr && !prefilled_ptr->is_wildcard(0)) {
    output.set(0, prefilled_ptr->at(0));
}
else {
    output.set(0, default_slot_zero);
}

// 2. Place all prefilled slots (hardware-locked)
if (prefilled_ptr != nullptr) {
    for (std::size_t i = 1; i < pal::max_size; ++i) {
        if (!prefilled_ptr->is_wildcard(i)) {
            output.set(i, prefilled_ptr->at(i));
        }
    }
}

// 3. Place all Absolute-position colors (skip prefilled — already placed above)
for (const auto &[color, position] : state.color_positions) {
    if (std::holds_alternative<AbsolutePosition>(position)) {
        const std::size_t slot = std::get<AbsolutePosition>(position).slot;
        if (!state.prefilled_slots.contains(slot)) {
            output.set(slot, color);
        }
    }
}
```

## Configuration: Packing and Alignment Modes

Tile sharing is controlled by two independent config enums:

**`TileSharingPacking`** (`domain/config/tile_sharing_packing.hpp`): Controls whether
the packing strategy is shape-group-aware.

- **`off`** (default): Packing ignores shape group membership entirely.
- **`biased`**: Before packing, compute `ShapeGroupMetadata` mapping tiles to their
  shape groups. The packing strategy (backtracking, etc.) uses this metadata to
  **penalize placing sibling tiles in the same palette**, maximizing the chance that
  shape group members end up in different palettes — creating more sharing
  opportunities for the Indirect system to capitalize on.
- **`optimal`**: Not yet implemented. Would reject sibling co-placement outright.

**`TileSharingAlignment`** (`domain/config/tile_sharing_alignment.hpp`): Controls
palette slot alignment for sharing deduplication.

- **`off`** (default): Pure sequential fill with no sharing alignment.
- **`greedy`**: Build Indirect links and apply the six-phase palette builder to align
  palette slot indices for color-isomorphic tiles.
- **`optimal`**: Not yet implemented. Would use a CSP-based solver for globally optimal
  alignment.

Both dimensions are independent — you can use `biased` packing with `off` alignment
(shape-aware packing without slot alignment), or `off` packing with `greedy` alignment
(post-hoc alignment on whatever the normal packer produces). Both converge at the same
link-building and diagnostic flow in `palette_packer.cpp`.

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
