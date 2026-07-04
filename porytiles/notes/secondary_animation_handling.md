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

Scoping caveat: the compiler reads this value from the *secondary's* tileset scope only (it is
consulted only when `is_secondary()`), so setting it on a primary tileset to opt out of being
linked into is silently ignored. The schema description in `config_schema.yaml` does not mention
this scoping yet; it should get a sentence when the schema is next touched.

### AnimTileMatcher changes

Added an `is_cross_tileset` flag to both `AnimTileMatch` (the result struct) and `KeyframeTileInfo`
(the internal storage struct). The `register_animation` method takes an optional `bool is_cross_tileset`
parameter (default `false`). The flag propagates through `find_match` into the result, letting the tile
assignment code distinguish secondary-owned animation matches from cross-tileset matches.

The lookup map was later reworked to handle mismatched extrinsic transparency (ET) values:
each map key (`KeyframeKey`) stores the ET of the tileset it was registered from,
and the comparator (`KeyframeKeyCompare`) classifies each side's pixels under its own ET
via `PixelTile<Rgba32>::cross_et_compare`.
Registrations and lookups therefore match correctly across tilesets configured with different ETs.

### Primary animation registration

In `pipeline_helper_register_animations()`, after registering the secondary's own animations, the compiler
now registers primary animation key frames when all three conditions are met:

1. Compiling a secondary tileset (`is_secondary()`)
2. A paired primary is available (`has_paired_primary()`)
3. The config is enabled (`cross_tileset_anim_linking_`)

Secondary animations register first so they win on canonical tile collisions (first-registration-wins
semantics in the lookup map).

Before iterating animations, a `tile_index -> pal_index` map is built from the primary's compiled
`metatiles_bin()`.
This is the authoritative source for which palette each primary tile was compiled against.
For tiles referenced by a primary metatile, RGBA palette matching would be the wrong tool:
two palettes can cover the same RGBA colors at different slot positions,
so a color-level match can pick a palette whose indices disagree with the compiled tile data
on GBA hardware, where tile pixel data contains indices into a specific palette.

Two subtleties in the map construction:

- **Tile index 0 is skipped.** The GBA transparent/empty tile lives at index 0 and never carries
  meaningful palette data. The loop explicitly `continue`s past it.

- **First-match-wins on duplicate tile indices.** The map is populated with `try_emplace`, so if
  the same tile index appears in multiple metatile entries with different palette banks, only the
  first-encountered palette is recorded. This is consistent with the first-match convention used
  in `pipeline_helper_build_keyframe_data`.

For each primary animation with a key frame:
- Per-subtile palette indices resolve through a cascade.
  The `primary_tile_pal_map` lookup is authoritative when the subtile is referenced by a primary metatile.
  Unreferenced subtiles fall back to RGBA matching of the composite frame tile against the
  primary's compiled palettes, with a `remark [cross-tileset-anim-rgba-fallback]`.
  If the colors do not fully match any primary palette either, the compiler emits a fatal error.
  Secondary palettes are never consulted:
  matching against them could produce an incorrect palette bank
  (secondary palettes vs. the required primary palette), causing wrong rendering on GBA hardware.
  Transparent subtiles get a dummy palette index since `register_animation` skips them.
- The key frame tiles (not composite tiles) are registered in the matcher's RGBA lookup map
  with `is_cross_tileset=true`. This is what secondary layer sheet tiles are matched against,
  since the key frame represents the canonical visual appearance of the animation.
- The composite frame is used only for the RGBA palette fallback above
  (it covers all colors across all animation frames);
  matcher registration itself uses the key frame tiles.

Primary animations without a key frame (manual frame linking) are skipped with a
`remark [cross-tileset-anim-skip-no-keyframe]` diagnostic.

### Stale primary detection

Two layers guard against source/compiled divergence in the paired primary:

- **Use-case layer, content-aware.** `CompileSecondaryTileset::compile()` runs
  `find_unsynced_tileset_artifacts()` against the paired primary's Porytiles keys and fatals on drift
  (modified pixel art, changed frame counts, etc.).
  Only runs when checksum verification is enabled and both tilesets have cached checksum files.
- **Domain layer, name-only.** `pipeline_helper_register_animations()` fatals when a primary animation
  exists in `porytiles_component` (source) but not in `porymap_component` (compiled), or vice versa.
  Only runs when cross-tileset linking is enabled.

Both instruct the user to recompile the primary before compiling the secondary.
The two layers' gating conditions are independent, so both can be bypassed at once;
the blind spot is tracked in #326.

### Diagnostics

Six diagnostics cover cross-tileset animation linking scenarios:

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

5. `remark [cross-tileset-anim-rgba-fallback]` -- Fires during primary animation registration when a
   key frame subtile is not referenced by any primary metatile,
   so its palette index resolves via RGBA matching against the primary's compiled palettes instead.

6. `warning [primary-anim-unreferenced-subtile]` -- Fires at primary compile time when a non-transparent
   key frame subtile is not referenced by any metatile.
   Palette assignment for that subtile will use the RGBA fallback during a paired secondary compile.

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

A name-level check runs earlier in the same per-primary-animation loop: after the key frame skip (so
manual-linking primary animations, which are never registered here, keep compiling) and before the RGBA
art collision check and palette resolution. If a secondary animation shares a name with the primary
animation, the compiler emits a fatal error requiring distinct names. This catches the same-name /
different-art case that the RGBA art check cannot: two animations named `flower` with visually distinct
key frames. Before this check existed, that input reached `register_animation` and aborted the compiler via
the matcher's cross-tileset name panic (#328). Same-name / same-art is caught here by the name check first,
which is correct: the art-collision guidance ("make key frames visually distinct") would be misleading when
the real conflict is the shared name.

### Extrinsic transparency mismatch warning

The use-case layer (`CompileSecondaryTileset::compile()`) now checks whether the secondary and paired
primary tilesets have different `extrinsic_transparency` config values. If they differ, a
`warning [cross-tileset-extrinsic-transparency-mismatch]` is emitted with config source notes for both
values.

Keyframe matching classifies each side's pixels under its own tileset's ET
(see the split-ET comparator under "AnimTileMatcher changes"),
so differing configured values are handled correctly by design.
The warning exists because the check can only see current config,
not the ET each tileset was actually compiled under:
if either side was compiled under a different ET and has not been recompiled since,
cross-tileset matching may produce unexpected results.
Upgrading this warning to a hard error against cached compile-time ET values is part of #230.

**Guard condition:** If either `extrinsic_transparency()` call fails to resolve, the use-case layer
refuses to proceed, propagating a formatted config error.
Only when both sides resolve successfully does the check compare the values:
if they differ, a non-fatal warning is emitted and compilation proceeds.

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

Long writeups for open gaps have moved to GitHub issues; each entry below is just status plus a link.

#### Gap A: Unreferenced primary animation subtiles -- CLOSED

Closed by a palette resolution cascade rather than hard validation.
At primary compile time, `pipeline_helper_validate_primary_anim_subtile_coverage()` warns
(`warning [primary-anim-unreferenced-subtile]`) about non-transparent key frame subtiles
that no metatile references.
At secondary compile time, unreferenced subtiles resolve their palette via the RGBA fallback
(see "Primary animation registration" above);
a fatal only fires when the fallback also fails, with recompile-the-primary guidance.

#### Gap B: Both staleness layers can be silently bypassed at once -- OPEN

Tracked in #326.

#### Gap C: Animation matcher skipped in patch/locked modes for newly-linked tiles -- OPEN

Tracked in #247.

#### Gap D: Extrinsic transparency mismatch failure modes in subtile processing -- CLOSED

Closed in two parts.
Misclassification: primary subtiles are now classified under the paired primary's own ET,
and the matcher's split-ET comparator handles mismatched-ET registration and lookup
(see "AnimTileMatcher changes" above).
Unresolvable ET: both the use-case layer and the compiler propagate a formatted config error
instead of crashing.
A differing ET still only produces `warning [cross-tileset-extrinsic-transparency-mismatch]`;
upgrading that to a hard error against cached compile-time ET values is folded into #230.

#### Gap E: No integration tests for cross-tileset paths -- OPEN

Tracked in #323, which absorbed this gap into the broader secondary compilation test coverage work.

#### Gap F: `nothing-to-do` early exit swallows config-only changes -- OPEN

Tracked in #230.

#### Gap G: `primary_tile_pal_map` can record a wrong palette via silent primary-side self-dedup -- OPEN (latent, needs repro)

During the primary's own compile, a static tile that workspace-dedups into the primary's own
animation range (indexed-pixel coincidence) takes the packer's palette with no warning: the
fallthrough warnings in `pipeline_helper_assign_tile_via_pal_match` are gated on
`is_secondary() && has_paired_primary()`, so the primary-side case is silent. If such a metatile
entry precedes the true anim-linked entry in `metatiles_bin` order, a paired secondary's
`try_emplace`-built `primary_tile_pal_map` records that palette, and a cross-linked secondary
tile can be emitted with a palette that disagrees with the frame data the primary's animation
DMAs over it at runtime. Silent wrong rendering. The mechanism is verified in code, but the
palette-coincidence-plus-ordering condition has never been reproduced. Repro fixture tracked as
a line item in #323; not issue-worthy on its own until reproduced.

### Intentional invariants

These are behaviors that look like bugs or accidents but are load-bearing for correctness.
Document them here so future refactors do not "fix" them into regressions.
Each invariant is also called out in a comment at the relevant code site;
the code comments are the authoritative copies.

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

- **Primary-vs-primary subtile collisions are not checked.** The collision detection loop in
  `pipeline_helper_register_animations()` checks `!match->is_cross_tileset`, meaning it only
  catches primary-vs-secondary collisions. If two different primary animations share a canonical subtile, the second one
  silently loses in the matcher's first-registration-wins lookup with no diagnostic. This is
  intentional: two primary animations sharing subtile art would be a primary-side authoring
  mistake, and the primary compiler is responsible for its own validation. The secondary
  collision check exists only to protect the cross-tileset boundary.

- **The matcher's cross-tileset name panic is a backstop, not the user-facing guard.**
  `AnimTileMatcher::register_animation` panics when a cross-tileset animation reuses an
  already-registered name. That panic is an internal invariant assertion: the user-facing guard is the
  name-level check in `pipeline_helper_register_animations()`, which returns a `FormattableError` before
  the primary animation is ever registered (#328). Do not "fix" the panic into a diagnostic or delete it;
  it exists to catch any future caller that reaches `register_animation` with a colliding name without
  going through the compiler-level check.

- **`total_keyframe_tiles()` counts cross-tileset registrations.** `AnimTileMatcher::register_animation`
  increments `total_tiles_` for every call, including `is_cross_tileset=true` registrations. The
  accessor returns the combined count across both secondary-owned and cross-tileset animations.
  Currently nothing in the compiler uses this accessor (the local variable `total_keyframe_tiles`
  in `pipeline_helper_register_animations()` is a separate thing), so no bug exists. If a future caller uses the
  accessor assuming it counts only the current tileset's animation tiles, it will get a wrong number.

### Notes

#### Automatic pairing assumes a single effective partner primary

When layouts pair a secondary with multiple primaries, `resolve_partner_primary` warns and picks
the alphabetically-first one; the compiled secondary's cross-tileset links are only correct for
layouts that use that primary, and per-layout correctness is out of reach at compile time (one
compiled secondary, one baked partner). Diagnostics improvements tracked in #329.

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

There is no `DecompileSecondaryTileset` use case yet. Tracked in #324.
