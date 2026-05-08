# Ideas: Secondary Animation Handling

## Context

When compiling secondary tilesets, there are two broad classes of animations:

1. True secondary animations, defined in the secondary set itself.
2. Secondary tilemap entries that reference into a primary animation tile range.

Case 1 is common, and the existing animation system handles it well for both `FrameLinking` modes.

Case 2 needs more thought. Before this feature, there were two ways it could happen:

- **Implicit workspace fallthrough.** Primary tiles.png (including animation key frame tiles) is pre-loaded
  into the secondary workspace. If a secondary metatile tile produces the same IndexPixel data as a primary
  animation tile, `first_occurrence_of()` returns the primary animation tile index. This is silent and
  uncontrollable. The user gets no notification that their tile will animate at runtime.

- **Manual `primary_references` in `anim.json`.** Users specify explicit override entries mapping individual
  metatile subtiles to primary animation tiles. This works, but is cumbersome for users who just want an
  `automatic`-like experience across both their primary and secondary sets.

Neither approach tells the user when a cross-tileset animation link is created. Neither gives them a simple
way to opt in or out.

## Proposal

Cross-tileset key frame matching: for any paired primary animation that has a key frame, match secondary
RGBA layer sheet tiles against it. If they match, link the secondary tilemap entry to the primary animation
tile and emit a diagnostic. A bool config (`tileset.animations.cross_tileset_linking`, default `true`)
controls whether this automatic matching is enabled.

This is conceptually separate from secondary-owned animations. Putting `primary_references` in `anim.json`
is convenient for manual control, but automatic cross-tileset linking should not require users to think in
terms of animation override entries.

## Implementation

### Config

New bool config value `cross_tileset_anim_linking` at `tileset.animations.cross_tileset_linking`
(CLI: `--cross-tileset-anim-linking` / `--no-cross-tileset-anim-linking`). Default `true`. Lives in
the domain config layer.

### AnimTileMatcher changes

Added an `is_cross_tileset` flag to both `AnimTileMatch` (the result struct) and `KeyframeTileInfo`
(the internal storage struct). The `register_animation` method takes an optional `bool is_cross_tileset`
parameter (default `false`). The flag propagates through `find_match` into the result, letting the tile
assignment code distinguish secondary-owned animation matches from cross-tileset matches.

### Primary animation registration

In `pipeline_helper_register_animations()`, after registering the secondary's own animations, the compiler
now registers primary animation key frames when all three conditions are met:

1. Compiling a secondary tileset (`is_secondary()`)
2. A paired primary is available (`has_paired_primary()`)
3. The config is enabled (`cross_tileset_anim_linking_`)

Secondary animations register first so they win on canonical tile collisions (first-registration-wins
semantics in the lookup map).

Before iterating animations, a `tile_index -> pal_index` map is built from the primary's compiled
`metatiles_bin()`. This is the authoritative source for which palette each primary tile was compiled
against. Using RGBA palette matching to re-derive palette indices is incorrect because two palettes
can cover the same RGBA colors but have them at different slot positions, causing wrong rendering on
GBA hardware where tile pixel data contains indices into a specific palette.

Two subtleties in the map construction:

- **Tile index 0 is skipped.** The GBA transparent/empty tile lives at index 0 and never carries
  meaningful palette data. The loop explicitly `continue`s past it.

- **First-match-wins on duplicate tile indices.** The map is populated with `try_emplace`, so if
  the same tile index appears in multiple metatile entries with different palette banks, only the
  first-encountered palette is recorded. This is consistent with the first-match convention used
  in `pipeline_helper_build_keyframe_data`.

For each primary animation with a key frame:
- Per-subtile palette indices are looked up from the `primary_tile_pal_map`. If a subtile is not
  referenced by any primary metatile entry (unusual), the compiler emits a fatal error. Deriving
  the palette index from secondary palette matching would produce an incorrect palette bank
  (secondary palettes vs. the required primary palette), causing wrong rendering on GBA hardware.
  Transparent subtiles get a dummy palette index since `register_animation` skips them.
- The key frame tiles (not composite tiles) are registered in the matcher's RGBA lookup map
  with `is_cross_tileset=true`. This is what secondary layer sheet tiles are matched against,
  since the key frame represents the canonical visual appearance of the animation.
- Composite frames are not used. Palette indices come exclusively from the primary's compiled
  metatile data, which is the only authoritative source for primary palette assignments.

Primary animations without a key frame (manual frame linking) are skipped with a
`remark [cross-tileset-anim-skip-no-keyframe]` diagnostic.

### Stale primary detection

Two staleness checks guard against source/compiled divergence:

- If a primary animation exists in `porytiles_component` (source) but not in `porymap_component`
  (compiled), the primary has uncompiled additions. Fatal error.
- If a primary animation exists in `porymap_component` but not in `porytiles_component`, the primary
  has uncompiled deletions. Fatal error.

Both instruct the user to recompile the primary before compiling the secondary.

### Diagnostics

Four diagnostics cover cross-tileset animation linking scenarios:

1. `remark [cross-tileset-anim-match]` -- Fires during the **tile assignment loop** (not during
   registration) when the RGBA key frame matcher explicitly matches a secondary tile to a primary
   animation subtile. This is the expected path when the config is enabled and the user's layer sheet
   art matches the primary key frame. The remark is gated on `match->is_cross_tileset`, so matches
   against secondary-owned animations are silent by design: the user only sees output when a
   cross-tileset link is created.

2. `remark [cross-tileset-anim-skip-no-keyframe]` -- Fires during primary animation registration when
   a primary animation has no key frame (likely manual frame linking). Cross-tileset key frame matching
   is not possible for these animations; they can still be linked via workspace fallthrough.

3. `warning [cross-tileset-anim-fallthrough]` -- Fires when the config is enabled but the RGBA matcher
   did not catch a tile, yet workspace IndexPixel lookup resolved to a primary animation range. This
   indicates indexed-pixel coincidence: two visually distinct RGBA tiles that produce the same palette
   indices after mapping. Unusual, but possible.

4. `warning [cross-tileset-anim-fallthrough-disabled]` -- Fires when the config is disabled but workspace
   deduplication still linked a tile to a primary animation range. Informs the user that the tile will
   animate at runtime despite opting out of automatic linking, and suggests either restructuring the
   tile art or re-enabling the config.

### Interaction with `primary_references`

Manual `primary_references` overrides always win. They write directly to `metatiles_bin` after the tile
assignment loop, naturally overriding any automatically-linked entries.

### Cross-tileset key frame collision detection

When registering primary animations for cross-tileset linking, each non-transparent primary key frame
subtile is checked against already-registered secondary animations. If a primary subtile has identical
RGBA data to a secondary subtile, the compiler emits a fatal error. This is necessary because the
matcher uses first-registration-wins semantics: without this check, the secondary would silently win
and the primary animation tile would never be linked. Requiring unique key frames across primary and
secondary ensures deterministic, predictable cross-tileset linking behavior.

The check runs in `pipeline_helper_register_animations()` before building `subtile_pal_indices` for each
primary animation and before calling `register_animation`. Collision detection is performed before palette
index resolution so that the error path does not waste work on palette lookups. The check canonicalizes
each primary subtile and calls `find_match`. Any non-cross-tileset match (i.e. a secondary animation)
triggers the error.

### Extrinsic transparency mismatch warning

The use-case layer (`CompileSecondaryTileset::compile()`) now checks whether the secondary and paired
primary tilesets have different `extrinsic_transparency` config values. If they differ, a
`warning [cross-tileset-extrinsic-transparency-mismatch]` is emitted with config source notes for both
values.

This matters because cross-tileset operations (animation key frame matching, workspace tile comparison)
use the secondary's extrinsic transparency. If the primary was compiled with a different value, tiles
that should be transparent may not be recognized as such, or vice versa, leading to incorrect linking.

**Guard condition:** If either `extrinsic_transparency()` call fails to resolve, the use-case layer
refuses to proceed with a fatal error (see Gap D closure). Only when both sides resolve successfully
does the check compare the values: if they differ, a non-fatal warning is emitted and compilation
proceeds.

The warning fires in the use-case layer (not the domain compiler) because it is the earliest point where
both tileset names are resolved, and the mismatch affects all cross-tileset RGBA operations, not just
animation linking.

**Ordering note:** The ET mismatch check (step 4b) runs before the primary content staleness check
(step 5) in `CompileSecondaryTileset::compile()`. If the primary is stale, the ET check might fire
against an outdated primary ET value, producing a potentially misleading warning (or suppressing one
that would fire after recompilation). This ordering is necessary because the staleness check requires
checksum infrastructure that is initialized later, but callers should be aware that the ET diagnostic
is not guaranteed to reflect the primary's latest source state.

### Known Gaps

#### Gap A: Unreferenced primary subtile triggers fatal error on secondary compilation — **CLOSED**

**Status:** Closed by `pipeline_helper_validate_primary_anim_subtile_coverage()`, which runs at the
end of `pipeline_step_match_tiles_pals` during primary compilation. Every non-transparent primary
animation subtile is verified to be referenced by at least one metatile entry before a paired
secondary can ever use it.

The secondary-side fatal at `pipeline_helper_register_animations()` remains as a defense-in-depth
assertion, reachable only for primaries compiled by another tool or with a cache that has drifted
past the use-case-layer content-aware staleness check (Gap B closure below).

#### Gap B: Staleness check is name-only and lives in the wrong layer — **PARTIALLY CLOSED**

**Status:** `CompileSecondaryTileset::compile()` now runs `find_unsynced_tileset_artifacts()`
against the paired primary's Porytiles keys, fatal on drift. This catches content edits (modified
pixel art, changed frame counts, etc.) that the name-only domain-layer check cannot see. The new
check only runs when the paired primary has a cached checksum file; a primary compiled externally
(e.g. by porytiles1 or by hand) cannot be content-verified here.

The name-only staleness check inside `pipeline_helper_register_animations()` remains in place as
defense-in-depth. It catches added or removed animations when the use-case-layer content check
cannot (either the primary cache is missing, or the user compiled the primary with a different
tool). Removing it entirely is a possible future cleanup, but only after we are confident the
use-case-layer check is the ground truth for all realistic workflows.

#### Gap C: Animation matcher skipped in patch/locked modes for newly-linked tiles

In `pipeline_helper_assign_tile_via_pal_match()`, patch and locked modes gate the animation matcher
on whether the tile's *previous* tile index was already in an animation range:

```c++
bool should_check_anim_matcher = true;
if (tiles_edit_mode_ != ArtifactEditMode::optimize && flat_index < porymap_tilemap_entries_.size()) {
    const auto original_tile_index = porymap_tilemap_entries_[flat_index].tile_index();
    should_check_anim_matcher = anim_tile_matcher_.is_in_animation_range(original_tile_index);
}
```

If a user enables `cross_tileset_anim_linking` and recompiles in patch or locked mode, tiles that
were previously assigned to regular workspace slots won't be checked against primary animation key
frames. The config change silently has no effect until the user recompiles in optimize mode.

Tracked in #247.

#### Gap D: Extrinsic transparency mismatch has undiagnosed failure modes in subtile processing — **PARTIALLY CLOSED**

**Status (resolvable-ET sub-case, closed):** `CompileSecondaryTileset::compile()` refuses to
proceed when either side's `extrinsic_transparency()` cannot resolve. Previously, an unresolvable
ET on either side caused the check to silently pass, leaving the two failure modes below fully
in play with no user-visible signal. That guard is now a fatal error.

**Status (differing-ET sub-case, still open):** When both sides resolve but disagree, the use-case
layer still emits only `warning [cross-tileset-extrinsic-transparency-mismatch]`. Compilation
proceeds, and both failure modes below are still reachable.

Both the palette index lookup loop and the collision detection loop in
`pipeline_helper_register_animations()` use the **secondary's** extrinsic transparency to classify
**primary** subtiles as transparent or not. When the values differ, two failure modes arise:

1. **Subtile transparent under primary ET but opaque under secondary ET.** The subtile is processed
   normally. Palette index lookup in `primary_tile_pal_map` may succeed (giving a meaningless palette
   for a tile that was meant to be transparent) or fail (triggering the Gap A unreferenced-subtile
   fatal error for a subtile that was never supposed to be referenced).

2. **Subtile opaque under primary ET but transparent under secondary ET.** The subtile is skipped
   with dummy palette index 0 and never registered in the matcher. A legitimate cross-tileset link
   is silently lost.

The same misclassification affects the collision detection loop, where a subtile that should be
checked may be skipped (or vice versa).

Whether to promote the mismatch warning to a hard fatal is a policy question. The plan that closed
the unresolvable-ET sub-case deferred it: legitimate workflows sometimes involve paired tilesets
with different tastes in transparency color, even if the current implementation cannot handle them
safely.

#### Gap E: No integration tests for cross-tileset paths

The unit tests in `anim_tile_matcher_test.cpp` cover the matcher service well (flag propagation,
first-registration-wins, flip matching, transparent tile handling, mixed secondary/primary
registration order, etc.). But there are no integration tests for:

- The full registration orchestration in `pipeline_helper_register_animations` (staleness checks,
  palette index map construction, collision detection, the skip-no-keyframe path).
- The fallthrough diagnostic paths in `pipeline_helper_assign_tile_via_pal_match` (both enabled
  and disabled variants).
- The extrinsic transparency mismatch warning and the new ET-unresolvable fatal in
  `CompileSecondaryTileset::compile()`.
- The new primary-side checksum staleness check.
- The new primary-side unreferenced-subtile fatal at primary compile time.

Integration testing these paths would require constructing a full `CompilerTask` with paired
primary data. Scoped as a follow-up PR that also builds a reusable paired-primary fixture harness.

#### Gap F: `nothing-to-do` early exit swallows config-only changes

The checksum cache hashes file contents only. `ProjectArtifactChecksumProvider::compute_tileset_artifact_checksums`
reads each artifact key as a file under `project_root_` and digests its bytes; no domain config
value contributes to the hash. `TilesetArtifactKeyProvider::get_porytiles_artifact_keys` only
enumerates file-based artifacts (layer PNGs, attributes CSV, pal files, anim frames, anim params
JSON). The cache JSON at `porytiles/tilesets/<name>/tileset.cache.json` is a simple `key -> checksum`
map with no config fingerprint.

Consequence: if a user flips `tileset.animations.cross_tileset_linking` (or any other domain config
value) between compilation runs **without** touching any source file, the `all_checksums_tileset_match`
branch at `CompileSecondaryTileset::compile()` returns true, and the `warning [nothing-to-do]`
fires. The config change silently has no effect on output.

This is a second flavor of Gap C, but cross-cutting: it affects **every** domain-config-gated
feature in the compiler, not just cross-tileset linking. A proper fix lives in the checksum layer
(e.g. hash a deterministic serialization of the effective config into the tileset cache), and
would touch every caller of the cache, so it is out of scope for this cross-tileset feature.

Tracked in #230.

### Intentional invariants

These are behaviors that look like bugs or accidents but are load-bearing for correctness.
Document them here so future refactors do not "fix" them into regressions.

- **`primary_tile_pal_map` uses `try_emplace`.** If the same tile index is referenced by multiple
  metatile entries with different palette banks, the first-encountered palette wins. This matches
  the first-match convention used by `pipeline_helper_build_keyframe_data`, so the two code paths
  cannot drift semantically. Changing either side to last-match-wins or to raise on conflict
  must be done to both, or cross-tileset animation tiles will be assigned palettes that disagree
  with how their reused siblings were compiled.

- **Collision detection uses `CanonicalPixelTile<Rgba32>` with `find_match`.** This mirrors the
  exact mechanism by which secondary tiles are later matched against primary animation key
  frames during the tile assignment loop. Using a different comparator or a looser match would
  let the collision check pass for subtiles that will later collide at match time, reintroducing
  the first-registration-wins silent-lose bug that the collision check exists to prevent.

- **Secondary animations register before primary animations in the matcher.** Secondary-owned
  animations are registered first so that when a secondary and primary animation have overlapping
  RGBA key frames, the secondary wins the `find_match` lookup. This intentionally defers to
  secondary-owned art as the source of truth; the collision detection loop then fires to refuse
  compilation rather than silently losing the primary link.

- **The cross-tileset match remark is gated on `match->is_cross_tileset`.** Secondary-owned
  animation matches are silent by design. The remark exists to surface **new** runtime behavior
  (a tile that now animates because it links into primary art), not to narrate every secondary
  tile that happens to match a secondary animation.

- **Primary-vs-primary subtile collisions are not checked.** The collision detection loop at
  line 1557 checks `!match->is_cross_tileset`, meaning it only catches primary-vs-secondary
  collisions. If two different primary animations share a canonical subtile, the second one
  silently loses in the matcher's first-registration-wins lookup with no diagnostic. This is
  intentional: two primary animations sharing subtile art would be a primary-side authoring
  mistake, and the primary compiler is responsible for its own validation. The secondary
  collision check exists only to protect the cross-tileset boundary.

- **`total_keyframe_tiles()` counts cross-tileset registrations.** `AnimTileMatcher::register_animation`
  increments `total_tiles_` for every call, including `is_cross_tileset=true` registrations. The
  accessor returns the combined count across both secondary-owned and cross-tileset animations.
  Currently nothing in the compiler uses this accessor (the local variable `total_keyframe_tiles` at
  `tileset_compiler.cpp:1336` is a separate thing), so no bug exists. If a future caller uses the
  accessor assuming it counts only the current tileset's animation tiles, it will get a wrong number.

### Notes

#### Fallthrough animation range check is O(primary_anims) per workspace-matched tile

`tileset_compiler.cpp` (the `pipeline_helper_assign_tile_via_pal_match` fallthrough check) iterates
all primary porymap animations for every workspace-matched tile. `AnimTileMatcher` already has
`is_in_animation_range()`, but it cannot replace this loop for two reasons:

1. The fallthrough check runs even when `cross_tileset_anim_linking` is **disabled**, in which case
   primary animations are not registered in the matcher. The loop reads from `primary_porymap_anims`
   which is always available.

2. `is_in_animation_range()` checks **all** registered animations (both secondary and primary), but
   the fallthrough diagnostic specifically targets primary animation ranges. It also returns only a
   bool, while the diagnostic needs the animation name.

A proper optimization would require either a precomputed interval structure or a new matcher method
that returns the animation name for a given tile index. Not a correctness issue.

### Decompilation

Tracked as a `@todo` on `DecompilePrimaryTileset` in
`Porytiles2/include/porytiles2/app/use_cases/decompile_primary_tileset.hpp`. The follow-up work is
to build a `DecompileSecondaryTileset` use case that mirrors the forward cross-tileset path in
`pipeline_helper_register_animations`, reconstructing the RGBA key frame tile on the secondary
layer sheet from animation tile offsets. Decompiling a secondary against a different primary than
it was compiled against will produce incorrect results, since the tile offsets are primary-specific.
