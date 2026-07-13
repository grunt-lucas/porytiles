# Finding 2: a mask's minimum width is treated as an authoritative storage stride

Status: open. To be fixed in this PR (#336) in a later session. This note is the handoff.

## The problem

When no explicit `metatile_attribute_size` is pinned, the reconciler infers the attribute
size from the mask layout: a unique non-synthesized candidate set's `required_bytes`
becomes the authoritative width.

- Width decision: `porytiles/lib/domain/algorithms/metatile_attribute_schema_reconciler.cpp`,
  Step 1 (the `else if (real_sets.size() == 1)` branch, currently around line 267).
- `required_bytes` computation: `porytiles/lib/domain/algorithms/metatile_attribute_inference.cpp`,
  `required_bytes_for()` (currently around line 124).

`required_bytes` is only the *smallest* width whose bit range covers the highest set bit
across the candidate's masks (`std::bit_width` over the masks, rounded up to 1/2/4 bytes).
It is a lower bound on the storage width, not the width itself. Masks can prove a layout is
*at least* N bytes wide; they can never prove it is *only* N bytes wide.

Meanwhile the scanner already reads a second, independent signal: the declared element
width of `struct Tileset`'s `metatileAttributes` member (`const u8/u16/u32 *`), surfaced as
`MetatileAttributeInferenceResult::declaration_size`. Today that value only feeds the
*declaration* size; it is not consulted when deciding the attribute size.

## Concrete failure

A project with `const u16 *metatileAttributes` (declaration width 2) but whose only declared
mask is `METATILE_ATTR_BEHAVIOR_MASK 0x00FF`:

- `required_bytes_for` sees a high bit of 7, rounds to **1 byte**, so the unique candidate
  pins the attribute size to 1 and it is marked authoritative.
- `declaration_size` is 2.

The result is a 1-byte attribute schema with a 2-byte declaration. The binary reader then
walks `metatile_attributes.bin` with a 1-byte stride and splits every real 2-byte entry into
two attributes. Silent corruption, no diagnostic.

## Why this is not urgent (but is real)

No stock testbed hits it:
- pokeemerald: `u16` declaration, masks reach bit 15 (layer type at `0xF000`) → `required_bytes` 2 = declaration.
- pokefirered: `u32` declaration, masks reach the high bits → 4 = declaration.
- pokeemerald-expansion: declares both mask layouts, so it takes the dual-layout fatal path
  (`real_sets.size() >= 2`) before any single-candidate width decision runs.

It is a latent footgun for an unusual project whose masks all sit below its declared width.
It is also pre-existing: the old `MetatileAttributeConfigProvider` used the identical
`required_bytes`-as-authoritative logic, so the #336 refactor preserved it byte-for-byte.
The refactor is what makes it cheap to fix now, because the declaration width and the mask
candidates are both in scope inside one pure function.

## Fix direction (and the trap)

The safe shape: when a unique candidate's `required_bytes` is **narrower** than the scanned
declaration width, do not treat the mask-derived width as authoritative. Either widen the
inferred attribute size up to the declaration width, or drop to the assumed-width path (with
the existing warning) and require the explicit `metatile_attribute_size` knob.

Do **not** take the tempting shortcut of "the declaration width is the authoritative
storage stride, always use it." Expansion deliberately decouples the two: it declares
`const u16 *metatileAttributes` but supports a 4-byte FRLG attribute layout. Declaration
width is a strong signal only as a *lower bound refinement* on the mask-derived width, not as
ground truth for the stride. Any fix must keep expansion's dual-layout case fatal and must not
regress the byte-identical artifacts on pokeemerald / pokefirered / expansion.

## Tests to add with the fix

- Unique low-bit masks + wider declaration → not silently narrowed (the failure above).
- Confirm the existing dual-layout fatal path is untouched.
- Re-verify byte-identical import/compile artifacts on all three testbeds (the #336 baseline
  was built from `a0b2f65f`).
