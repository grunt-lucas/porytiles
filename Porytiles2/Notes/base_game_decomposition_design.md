# Design: Decomposing BaseGame into Independent Config Dimensions

## Problem

The `BaseGame` enum is detected by scanning `include/global.fieldmap.h` for string markers. This is
brittle: pokeemerald-expansion's "Add FRLG" PR added `METATILE_ATTRIBUTE_BEHAVIOR` (previously a
firered-only marker), causing misdetection as pokefirered. Since decomp users modify their projects
freely, hardcoded game presets are inflexible and will keep breaking.

## Motivating Capability: User-Defined Attribute Layouts

The real driver for this refactor is not cleanup. It is enabling a capability that Porytiles2
does not currently support. Real-world decomp users have already modified their metatile attribute
layouts: added new fields, removed fields, changed bit widths, or renamed existing ones. A
preset-based model ("are you emerald or firered?") cannot cover those forks no matter how many
presets we add. A schema-driven model, where the attribute layout is *data* detected from the
decomp's own header files or specified in config, does.

Removing `BaseGame` is the *means*. Unblocking customized attribute layouts is the *end*.

## Key Finding: BaseGame Is a Single Boolean

Every production consumption site checks `== BaseGame::pokefirered`. The distinctions between
pokeemerald, pokeruby, and pokeemerald_expansion are never used for behavioral decisions. They only
matter for detection disambiguation.

### Production Consumption Sites

Line numbers drift; function names are stable. Authoritative lookup is
`grep -rn base_game Porytiles2/`.

| File                                   | Function                                                               | What It Controls                                                                               |
|----------------------------------------|------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| `project_primary_tileset_importer.cpp` | `ProjectPrimaryTilesetImporter::import_porymap_component_from_vanilla` | Binary attr parse format (2 vs 4 byte), migrated to size detected by `MetatilesHeaderProvider` |
| `project_tileset_artifact_reader.cpp`  | `ProjectTilesetArtifactReader::read_metatile_attributes_bin`           | Binary attr parse format, migrated                                                             |
| `project_tileset_artifact_writer.cpp`  | `ProjectTilesetArtifactWriter::write_metatile_attributes_bin`          | Binary attr write format, migrated                                                             |
| `project_tileset_artifact_writer.cpp`  | `ProjectTilesetArtifactWriter::write_attributes_csv`                   | CSV column format and default filtering                                                        |
| `attributes_csv_loader.cpp`            | `parse_attributes_csv` (free function)                                 | CSV format validation vs project's `BaseGame`                                                  |
| `command_compile_tileset.hpp`          | `CompileTilesetCommand::Run`                                           | Terrain/encounter provider instantiation                                                       |
| `command_create_tileset.hpp`           | `CreateTilesetCommand::Run`                                            | Terrain/encounter provider instantiation                                                       |
| `command_decompile_tileset.hpp`        | `DecompileTilesetCommand::Run`                                         | Terrain/encounter provider instantiation                                                       |
| `command_import_tileset.hpp`           | `ImportTilesetCommand::Run`                                            | Terrain/encounter provider instantiation and pokeruby "not supported" gate                     |

## The Three Behavioral Dimensions

### 1. Binary Attribute Format -- DONE

- **Invariant**: Total attribute size is authoritative from `MetatilesHeaderProvider`, which scans
  the element type of `gMetatileAttributes_*` in `metatiles.h` (`const u16` -> 2 bytes,
  `const u32` -> 4 bytes). There is no user-facing override. The previous `metatile_attr_size`
  YAML key and the `--metatile-attr-size` CLI flag are **deleted**. Porytiles2 is unreleased, so
  no migration path is needed.
- **Schema-load validation**: the schema loader checks that every field mask fits within
  `detected_attr_bytes * 8` bits and raises a diagnostic if any mask overflows. This catches the
  "user wrote a mask at bit 30 but metatiles.h says u16" class of error at load time.
- **No numeric escape hatch**: if `metatiles.h` cannot be parsed (novel formatting, unusual
  variable names), the user writes a full explicit `metatile_attr_fields:` schema. The schema
  implies the required bit width through its masks; there is no separate numeric size knob.
- **Completed refactor**: Replaced `base_game == pokefirered` checks in all three binary
  parse/write sites with size-driven logic. Removed `BaseGame` dependency from
  `ProjectPrimaryTilesetImporter` and `ProjectTilesetArtifactReader` entirely. Added the
  detected attr size as a resolved constructor parameter to `ProjectTilesetArtifactWriter`
  (which still needs `BaseGame` for CSV writing, pending Dimension 2).

### 2. CSV Attribute Schema -- NEEDS DECOMPOSITION

CSV *write* is hardcoded by `BaseGame` in `ProjectTilesetArtifactWriter::write_attributes_csv`.
CSV *read* already auto-detects the column layout from the header row inside `parse_attributes_csv`
(`attributes_csv_loader.cpp`), but a follow-up validation block in the same function cross-checks
the detected format against the project's `BaseGame`. The read side's format detection is mostly
ready; most of the Dimension 2 work lives in the writer plus the validator.

- **Target**: Derive CSV columns from the active attribute schema (see Design Direction below).
  One column per named field that is marked CSV-visible.
- **Auto-detected** but overridable via config.

### 3. Terrain/Encounter Provider Instantiation -- NEEDS DECOMPOSITION

- **Currently**: Created only when `base_game == pokefirered`
- **Target**: Driven by which attribute fields are configured. If the schema includes a "terrain"
  field, create the terrain provider. If it includes "encounter_type", create the encounter
  provider.
- This falls out naturally from decomposing dimension #2.

## Design Direction: Schema-Driven Attributes

The core idea: instead of asking "which game are you?" and selecting a preset, detect (or configure)
the actual attribute schema. Each `METATILE_ATTR_*_MASK` define becomes a named field with a bit
position, bit width, and CSV column. This is data-driven, extensible, and decomp-friendly.

### Bit-Field Model

The binary bit layout is the source of truth; CSV is a user-visible *projection* of that layout.
The "schema" describes which bit-fields exist and which are exposed to CSV (and under what column
names).

Stock pokefirered has **eight** bit-fields in its 4-byte layout (see
`include/porytiles2/domain/models/metatile_attribute.hpp`):

```
 0..8   behavior
 9..13  terrain
 14..17 attribute_2   (reserved, currently unused by stock firered)
 18..23 attribute_3   (reserved)
 24..26 encounter_type
 27..28 attribute_5   (reserved)
 29..30 layer_type
 31     attribute_7   (reserved)
```

The current `MetatileAttribute` carrying anonymous `attribute_2/3/5/7` members is a shortcut.
Those exist only because stock pokefirered does not name them, and Porytiles2 has no model for
"a field the user named." The right model:

- The binary bit layout is the source of truth; CSV is a projection of it.
- The schema enumerates *named fields* with their bit ranges. User-defined fields have real names
  in the schema (not `attribute_2`).
- `MetatileAttribute` becomes a map from field-name to value, backed by the active schema. The
  anonymous `attribute_2/3/5/7` members go away.
- Unused bits in the layout are just "gap": zero on write, ignored on read. If a user wants those
  bits, they add a named field to the schema.

No preservation semantics, no "mystery reserved bits." Every bit is either schema-named or zero.

### Typed Fields and Provider Mapping

Not every field on a metatile attribute is a raw integer in the Porytiles representation.
`behavior`, `terrain`, and `encounter_type` are backed by provider classes that translate
decomp enum-name strings (e.g. `"MB_NORMAL"`) to integer bit values and back. That is why
`BehaviorMapProvider`, `TerrainTypeMapProvider`, and `EncounterTypeMapProvider` currently
exist. The providers are used only in the CSV path; the binary path never touches them.

The schema must let users:

1. Change a provider-backed field's mask/position (e.g. shrink `behavior` to 4 bits)
   while still using its provider for CSV stringification.
2. Mark a field as a raw integer (no provider, integer column in CSV).
3. Drop a provider-backed field entirely (provider not instantiated, CSV column
   disappears).
4. Add a **new** provider-backed field whose values come from some user-defined decomp
   enum (e.g. a fork's `BUILDING_TYPE_*` header) that Porytiles has never heard of.

Requirement (4) means the registry of provider kinds must be **open from day one**.
Porytiles does not special-case the names `behavior`, `terrain`, or `encounter_type`;
those are just the names stock decomps happen to use.

#### Per-Field Provider Block

Each field entry carries an optional `provider:` block. Presence means the field is
provider-backed. Absence means the field is a raw integer.

```yaml
fieldmap:
  metatile_attr_fields:
    - name: behavior
      mask: 0x01FF
      provider:
        header: include/constants/metatile_behaviors.h
        prefix: MB_
        skipped: [MB_INVALID]
        format: either
    - name: terrain
      mask: 0x3E00
      provider:
        header: include/global.fieldmap.h
        prefix: TILE_TERRAIN_
        format: enums_only
    - name: encounter_type
      mask: 0x07000000
      provider:
        header: include/global.fieldmap.h
        prefix: TILE_ENCOUNTER_
        format: enums_only
    - name: custom_flag
      mask: 0x08000000
      # no provider block -> raw integer column in CSV
```

Field names are arbitrary. A fork that renames `encounter_type` to `encounter` (or to
`terrain_type_2`, or adds a brand-new `building_type` field pointing at its own header)
works with zero Porytiles source changes.

See also `metatile_attr_field_overrides` for partial overrides that don't require writing the
full list.

Provider block shape:

- `header`: path to the header file, relative to the project root.
- `prefix`: the name prefix Porytiles uses to filter entries inside the header.
- `skipped` (optional, default `[]`): names to ignore, e.g. sentinels like `MB_INVALID`.
- `format` (optional, default `either`): one of `defines_only | enums_only | either`.
  Controls which C declaration styles the parser considers. See the "Provider
  Consolidation" subsection for semantics.

Max value is derivable from the field's mask width (`std::popcount(mask)`); schema-load
validates that every value parsed out of the header fits, and diagnoses the specific
offending name when it doesn't.

Shrinking `behavior` to 4 bits is just `mask: 0x0F` with the same provider block. The
provider does not care about bit width; it is a string-integer map. Schema-load handles
the validation.

#### Mask Only, No Explicit Offset

Offset is derivable from mask: `offset = std::countr_zero(mask)`,
`width = std::popcount(mask)`. Schema-load validates:

1. `mask != 0`.
2. Mask is a single contiguous run of 1-bits (rejects `0xF0F0`).
3. No two fields have overlapping masks.
4. Mask fits within `detected_attr_bytes * 8` bits (from `MetatilesHeaderProvider`).

Explicit offset only helps for split or interleaved fields. Gen III decomps do not use
those. Defer until someone asks.

#### Per-Field CSV Default for Row Omission

Each schema field carries an optional `default:` integer. The CSV writer omits a row
when every field on that metatile equals its declared default — this is the same
"don't write rows that match the implicit default" compression the current writer
applies to `behavior=0` for emerald and to `behavior=0,terrain=0,encounter=0` for
firered, generalized to the schema. The symmetric reader path (the
`MetatileAttribute new_attr{}` fill site in `TilesetCompiler` for IDs absent from
the CSV) reads each field's `default` from the schema rather than relying on
`MetatileAttribute{}`'s zero value-init.

Default `default:` is `0`, which keeps stock pokeemerald and pokefirered
behavior identical to today. Forks that add a custom provider-backed field, or that
operate on a tileset whose most common behavior is not `MB_NORMAL` (e.g. a
grass-themed outdoor tileset where most metatiles are `MB_TALL_GRASS`), can declare
a non-zero default and get the same row-omission compression with zero Porytiles
source edits. Schema-load validates that each `default:` fits within the field's
mask width and, for provider-backed fields, that the value resolves to a known
provider entry.

#### Schema Inference via a Config Provider

The attribute schema integrates into the existing `config_schema.yaml` provider chain.
The explicit YAML provider reads user-authored schema entries (with `provider:` blocks
as shown above) directly. A new `MetatileAttributeConfigProvider` sits alongside it as
an **inference layer**: when YAML does not supply an entry, the inference provider
scans the decomp's own headers and synthesizes the same shape of config a user could
have written by hand. No hidden "stock pokefirered preset" lives in Porytiles; only
header scanning driven by documented rules.

##### Partial Override (Field-Level Tweaks)

Porytiles2's `LazyLayeredConfig` resolves each config value atomically through the
provider chain. It does not merge lists across providers: whichever provider "wins"
for a given key returns the entire list. That means `metatile_attr_fields:` is
all-or-nothing. A user who only wants to tweak the `skipped` list on `behavior`
cannot do so by supplying a partial list and hoping it merges with inference output;
supplying a list would replace the inferred list entirely.

The ergonomic answer is a **sibling** config key, `metatile_attr_field_overrides:`,
whose shape is a map of `field_name -> partial_field_spec`. It resolves atomically
through the same provider chain (so a user can override it in exactly the same way
as any other config value), and the domain-layer schema loader applies it **after**
`metatile_attr_fields` has been resolved from either YAML or inference.

Merge rule:

- For each entry in `metatile_attr_field_overrides`, look up the matching baseline
  field in the resolved `metatile_attr_fields` list by `name`.
- Keys present in the override replace the baseline values. Keys absent from the
  override fall through to the baseline.
- An override entry naming a field that does not exist in the baseline is a hard
  error at schema load. The user gets a diagnostic that names the missing field
  and lists the available baseline names; silent creation of new fields would
  defeat the intent, which is "tweak inference, not supplement it."

Representative example:

```yaml
# Baseline inferred from global.fieldmap.h. User only tweaks behavior's skipped list
# and changes the custom_flag field to raw (drops its provider block).
fieldmap:
  metatile_attr_field_overrides:
    behavior:
      provider:
        skipped: [MB_INVALID, MB_DEPRECATED]
    custom_flag:
      provider: null    # explicit null drops the provider block
```

Three ergonomic tiers fall out of this:

1. **Pure inference** — zero YAML. Stock decomps hit this path.
2. **Override-only** — small YAML targeting one or two fields, keeping the rest
   of inference intact. This is the expected common case for lightly forked projects.
3. **Full explicit** — the existing `metatile_attr_fields:` path. Users who diverge
   heavily from stock write the full list and skip inference altogether.

`provider: null` is the escape-hatch syntax for demoting a provider-backed field to
raw. Without the explicit `null`, omitting `provider:` in an override means "do not
touch the provider block," not "drop it." This distinction matters: the override
merge is additive by default, not destructive.

Inference is **rule-based**, not preset-based. The rule set is small, documented, and
lives next to the provider code. Instead of hardcoding names like `BEHAVIOR`,
`TERRAIN`, and `ENCOUNTER`, inference runs a two-phase cascade driven by what it
actually finds in `global.fieldmap.h`. That way a fork that invents a new attribute
enum (e.g. `BIOME`) gets a working schema with zero Porytiles source edits.

##### Phase A -- Field Discovery

Scan `include/global.fieldmap.h` and `src/fieldmap.c` for up to three sources,
reconciling them:

- **Declaration enum** (source 1): an anonymous enum whose members begin with
  `METATILE_ATTRIBUTE_` in `include/global.fieldmap.h`. Provides field ordering
  and name suffixes. Present in pokefirered and pokeemerald-expansion, absent
  in pokeemerald.
- **Mask defines** (source 2): `#define METATILE_ATTR_*_MASK 0x...` in
  `include/global.fieldmap.h`. Provides authoritative bit masks. Present in
  pokeemerald and pokeemerald-expansion, absent in pokefirered.
- **Mask array in `src/fieldmap.c`** (source 3): an array literally named
  `sMetatileAttrMasks`, declared in `src/fieldmap.c`, with designated
  initializers of the form `[METATILE_ATTRIBUTE_<SUFFIX>] = 0x...`. Each
  initializer becomes the authoritative mask for the field whose suffix matches
  the enum member. This is the pokefirered-stock source. Forks that renamed the
  array fall through to explicit YAML.

Reconciliation rules:

1. If only mask defines exist (source 2), each define becomes a field; name
   suffix comes from the define (`METATILE_ATTR_BEHAVIOR_MASK` -> suffix
   `BEHAVIOR`). Pokeemerald-stock.
2. If only the declaration enum exists (source 1) **but the `sMetatileAttrMasks`
   array in `src/fieldmap.c` is also found (source 3)**, masks come from the
   array keyed by enum member, field suffixes come from the enum.
   Pokefirered-stock.
3. If the declaration enum AND mask defines both exist (sources 1 + 2),
   reconcile by suffix: a field exists if *either* source names it. Mask comes
   from the define when available. Declaration-enum ordering wins for
   field-order display. Pokeemerald-expansion-emerald-side.
4. If only the declaration enum exists and `sMetatileAttrMasks` is **not**
   found (or its array name differs), fields are declared but masks are
   unknown at this step — Phase C handles the diagnostic. Forks of pokefirered
   that renamed the array.
5. `_FRLG`-suffixed mask defines are matched to their bare-name counterparts
   and become **alternates**: a same-named field with a different mask. The
   per-tileset schema (Phase 4) picks which mask applies. This is how
   pokeemerald-expansion's dual layout is handled cleanly. Additionally,
   pokeemerald-expansion-FRLG-side may carry both a declaration enum AND the
   `sMetatileAttrMasks` array; in that case sources 1 + 3 apply and produce
   the FRLG-layout alternates naturally. (Whether pokeemerald-expansion-FRLG-side
   actually ships `sMetatileAttrMasks` under that exact name is a Phase 3
   implementation verification step, not a design-doc claim.)

##### Phase B -- Provider Classification (per field)

Normalize each field's suffix to uppercase. Then:

1. **`BEHAVIOR`** — hardcoded to the behavior provider:
   `header: include/constants/metatile_behaviors.h`, `prefix: MB_`,
   `skipped: [MB_INVALID]`, `format: either`. This is the one special case because
   its provider lives *outside* `global.fieldmap.h`.
2. **`LAYER_TYPE`** — skip. Not a `metatile_attr_fields` schema field; see the
   "Layer Type" section for its dedicated treatment.
3. **Purely numeric suffix** (e.g. `2`, `3`, `5`, `7`) — raw integer field, no
   provider block.
4. **Any other suffix `X`** — grep `global.fieldmap.h` for an anonymous enum whose
   members match `TILE_{X}_*`. If found, emit a provider block:
   `header: include/global.fieldmap.h`, `prefix: TILE_{X}_`, `format: enums_only`.
   If not found, emit a raw integer field.

##### Phase C -- Mask Derivation

- **If Phase A produced a mask from sources 2 or 3**, use it. (Source 2 wins
  if both are present, but that case should not occur in stock decomps. The
  source-2 / source-3 conflict-resolution rule lands in the implementation
  plan, not here.)
- **If Phase A produced only source 1 (declaration enum, no mask from
  anywhere)**, inference cannot guess bit widths. Emit a diagnostic pointing
  the user to either (a) restore the `sMetatileAttrMasks` array under its
  original name in `src/fieldmap.c`, (b) add `METATILE_ATTR_*_MASK` defines
  to `global.fieldmap.h`, or (c) supply a `metatile_attr_field_overrides:`
  entry with an explicit `mask:` per affected field.
- **Exception**: if the detected total size is 2 bytes AND Phase A produced
  only `BEHAVIOR` and `LAYER_TYPE` fields, the stock emerald-style layout
  (`0x00FF` / `0xF000`) is unambiguous — apply it silently.

Alongside `sMetatileAttrMasks`, pokefirered's `src/fieldmap.c` also ships
`sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT]`. The shifts are derivable from
the masks (`std::countr_zero(mask)`), so inference does NOT require parsing the
shifts array. If the shifts array is present and differs from the computed
shifts, emit a warning and trust the masks — that combination indicates a
non-contiguous bit field, which the design already rejects in the
mask-validation rules ("Mask is a single contiguous run of 1-bits").

The elegance: a fork that adds `METATILE_ATTR_BIOME_MASK` alongside
`enum { TILE_BIOME_DESERT, TILE_BIOME_FOREST, ... }` gets a working schema with zero
Porytiles source edits. The `BEHAVIOR` special-case survives only because its header
is elsewhere; everything else follows the generic suffix-driven rule.

When a rule fires but the referenced header or prefix yields no entries, the
inference provider reports a warning and falls back to a raw field. The user then
writes a `metatile_attr_field_overrides:` entry (or a full `metatile_attr_fields:`
list) to correct the inference. This is the standard escape hatch: inference is a
convenience layer, explicit YAML wins.

Rule-based inference (rather than a `BaseGame`-style preset) makes failures
actionable and localizable. If the suffix-cascade does not fire for a particular
fork's attribute, the user overrides exactly that one field in YAML rather than
having to declare a whole new preset. Adding support for a new fork never requires
a Porytiles source change, only a rule tweak or a user-level config override.

#### Provider Instantiation from the Schema

The existing `if (base_game == BaseGame::pokefirered)` gate in each command
(`CompileTilesetCommand::Run`, peers) is replaced by a uniform schema walk at command
startup:

```c++
ProviderMap providers;
for (const auto &field : schema.fields()) {
    if (!field.has_provider()) {
        continue;
    }
    providers.emplace(
        field.name(),
        std::make_unique<HeaderEnumMapProvider>(
            project_root / field.provider_spec().header,
            field.provider_spec().to_enum_spec(field.name(), field.mask_width()),
            fmt, diag));
}
```

CSV writer/parser looks up the provider by field name when rendering or parsing a cell.
If a field has no provider, the cell is a raw integer.

Diagnostics improve. If a user's schema declares a provider block with
`prefix: TILE_TERRAIN_` in `include/global.fieldmap.h` but no matching names exist in
that header, that is a real actionable error
(`"Field 'terrain' declared provider prefix 'TILE_TERRAIN_' in 'include/global.fieldmap.h' but no matching names were found."`)
instead of a silent base-game misdetection.

#### Impact on `MetatileAttribute`

Consistent with the Phase 2 plan, `MetatileAttribute` becomes a schema-backed map of
`field_name -> integer bit value`. **Typing is not stored on the attribute.** The
attribute does not know any of its fields are provider-backed. The CSV writer asks the
schema for the field's provider (if any) and routes through it. The anonymous
`attribute_2/3/5/7` members still go away.

#### Provider Consolidation

The three `Header*MapProvider` classes are structurally identical: three
parameterizations of the same class, not three different classes. They collapse into
one.

What actually differs between them today:

- **Prefix filter**: `"MB_"` / `"TILE_TERRAIN_"` / `"TILE_ENCOUNTER_"`.
- **Value integer type**: `uint16_t` for behavior, `uint8_t` for the other two. Dictated
  by bit width (9 / 5 / 3). All fit in `uint32_t`.
- **Max value**: `0x1FF` / `0x1F` / `0x07`.
- **Skipped names**: behavior skips `MB_INVALID`; the other two skip nothing.
- **Parse format**: behavior parses both `#define` and C enum members; terrain and
  encounter skip defines because their headers may contain complex expressions.
  pokefirered's `include/constants/metatile_behaviors.h` is pure `#define MB_* 0xNN`,
  so behavior must accept defines.

Everything else (the `ensure_loaded` logic, the `try_add_*_entry<Entry>` template, four
`unordered_map` cache members, the constructor shape `(path, fmt, diag)`, and
duplicate-detection diagnostics with `SourcePosition` tracking) is identical across all
three ~210-line cpp files.

The consolidated shape:

- **Domain interface** (`domain/services/enum_map_provider.hpp`): a single
  `EnumMapProvider` with `lookup(string) -> ChainableResult<std::uint32_t>` and
  `lookup(std::uint32_t) -> ChainableResult<std::string>`. Widening the return type is
  safe; every Gen III metatile attr field fits in 32 bits, and callers mask and shift
  against the schema anyway.
- **Infra implementation** (`infra/services/header_enum_map_provider.hpp`): one
  `HeaderEnumMapProvider` parameterized by an `EnumSpec`:

```c++
enum class HeaderFormat {
    // #define NAME 0xNN only. Use for headers with simple hex/decimal literals,
    // e.g. pokefirered/include/constants/metatile_behaviors.h.
    defines_only,
    // C enum members only. Use when the header has enum { NAME, ... } and
    // either has no simple defines or its defines use complex expressions
    // the parser cannot evaluate.
    enums_only,
    // Parse both formats and merge. Use when a header may appear in either
    // style across decomp versions. Default.
    either
};

struct EnumSpec {
    std::string prefix;                          // "MB_"
    std::uint32_t max_value;                     // derived from the field's mask width
    std::unordered_set<std::string> skipped;     // {"MB_INVALID"}
    HeaderFormat format;
    std::string field_display_name;              // the schema field name, for diagnostics
};
```

Every `EnumSpec` is constructed from a schema field's `provider:` block at load time.
There are **no hardcoded per-provider constants** in Porytiles: no `kBehaviorSpec`,
no `kTerrainSpec`, no `kEncounterSpec`. A user who adds a `BUILDING_TYPE_*` enum to
their fork just writes a schema entry with the matching provider block and it works
the same way as `behavior`.

**Wins**:

- Deletes roughly **420 lines of duplicated implementation**. Three ~210-line cpp files
  become one. Headers shrink from three 104-line files to one.
- Single test suite for header parsing, not three.
- Single diagnostics path. Fixes apply globally.
- The three abstract domain interfaces collapse into one.
- Users can add their own provider-backed fields without touching Porytiles source.

**Watch-outs**:

- Diagnostics quality. Max-value and prefix-mismatch errors must include the field name
  and prefix so users can trace which field overflowed.
  `EnumSpec::field_display_name` carries that text.
- Type-level distinction lost. Today, passing an `EncounterTypeMapProvider *` where a
  `BehaviorMapProvider *` is expected is a compile error. After consolidation both are
  `EnumMapProvider *` and the distinction is a runtime key (field name). Fine in a
  schema-driven world; the schema is authoritative.
- Uint widening to `uint32_t` changes call-site types. CSV read/write call sites assign
  to `MetatileAttribute`'s narrower fields today, but those narrow fields go away with
  the Phase 2 attribute rework. Timing matters: consolidation must land with or after
  Phase 2 so no narrow-field call sites linger. That is why this is Phase 2.5 rather
  than Phase 0 cleanup.
- Include-path churn. Any code importing one of the three domain interfaces needs
  updating.

## Per-Tileset vs. Per-Project Schema

Expansion projects already store `isFrlg` per-tileset at the Porymap level, which means a single
project may mix emerald-layout and FRLG-layout tilesets. A per-project schema with a global
"which layout" flag would break for those projects.

**Decision**: the attribute schema lives at the **tileset** level. The project config supplies a
default schema (typically auto-detected from `global.fieldmap.h`), and individual tilesets may
override it. Open Question 1 below is about the *shape* of that override, not whether to support
it.

## Layer Type

`layer_type` is **not** part of the `metatile_attr_fields` schema. It is a first-class
structural field backed by the tileset's layer mode (dual vs triple) and the metatile's
position/role within the tileset. Inference happens inside `layer_mode_converter` and
`porymap_artifact_parsers`, not through a mask-driven schema field.

**Default behavior (no knob set)**: the CSV has no `layer_type` column at all. CSV load treats
`layer_type` as unset and relies on `layer_mode_converter` to compute the correct value for
each metatile; CSV write omits the column entirely. This matches today's behavior and the
common case.

**Opt-in manual override**: a per-tileset boolean config key enables manual CSV-level control.
Working name: `write_layer_type_column: false` (default `false`). When set to `true`:

- **CSV write** emits an extra `layer_type` column, serializing each metatile's current
  `layer_type` value (whatever `layer_mode_converter` resolved it to).
- **CSV load** recognises the column. A filled cell is an explicit override for that row; a
  blank cell falls back to the normal auto-computed value from `layer_mode_converter`. This
  gives users a "touch only the rows I care about" workflow without requiring they fill in
  every row.
- **Responsibility**: any user editing the CSV with this knob enabled owns those rows'
  `layer_type` values. There is no uniform "default" to diff against — dual-layer tilesets
  have a roughly even split across `NORMAL` / `COVERED` / `SPLIT`, so the "emit only if any
  row differs from the default" heuristic does not apply cleanly.

The knob exists **only** for users who deliberately want per-row control. It is an escape
hatch; the common path is to leave `write_layer_type_column: false` and trust the inference
in `layer_mode_converter`.

## Pokeruby Gate

`ImportTilesetCommand::Run` currently throws when `base_game == BaseGame::pokeruby`. This gate
exists because ruby import was never validated, not because of a structural incompatibility.
Under the schema-driven model, ruby is just "emerald-family layout with different palette/tile
conventions"; there is no fundamental reason it cannot import.

**Decision**: delete the gate during Phase 6 (below). Test import against a real pokeruby project
and fix whatever breaks. If something structural prevents ruby support that goes beyond BaseGame
detection, capture it then, but do not gate on it upfront.

## Mask-Parser Robustness

Trading one brittle scanner (string-contains on `global.fieldmap.h`) for another (parsing
`#define` hex literals) is a real risk. The new scanner has to handle at minimum:

- `#define NAME 0x00FF` (direct hex literal)
- `#define NAME (OTHER_NAME >> N)` (derived values referencing other defines)
- Mixed representations in the same file: `#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF`
  (value = mask) alongside `enum MetatileAttr { METATILE_ATTRIBUTE_BEHAVIOR = 0 }`
  (value = bit index, not mask)
- Values guarded by `#ifdef` branches (pick the active branch or warn)
- Whitespace and formatting variance

A simple regex pass will not be enough for derived values or `#ifdef` guards. The defensible
approach is a minimal preprocessor that evaluates `#define` chains and tracks active `#ifdef`
branches, not a flat text scan.

The schema loader also cross-validates derived masks against the detected total attribute size
from `MetatilesHeaderProvider`. A user (or inference) declaring a mask bit beyond
`detected_attr_bytes * 8` gets a diagnostic at schema load time rather than a silent runtime
truncation.

Two packing styles exist in the wild and the inference layer treats them as equivalent because
the masks are what matter:

- **Pokeemerald-style**: `PACK(data, shift, mask)` / `UNPACK(data, shift, mask)` in `global.h`,
  with per-field helpers (`PACK_BEHAVIOR`, `UNPACK_BEHAVIOR`) in `global.fieldmap.h`. Mask and
  shift defines are paired in-source.
- **Pokefirered-style**: `ExtractMetatileAttribute(u32, u8)` in `src/fieldmap.c` indexing the
  `sMetatileAttrMasks` / `sMetatileAttrShifts` arrays by enum member.

The inference layer does not parse or recognize the PACK/UNPACK macros or the extraction
function — those are runtime plumbing. Only the mask sources (enum defines, array initializers)
are authoritative for schema inference.

## Open Questions

1. **Per-tileset schema representation**: How do `metatile_attr_fields`,
   `metatile_attr_field_overrides`, and `write_layer_type_column` compose at the tileset level?
   Expansion projects can mix emerald-layout and FRLG-layout tilesets in one project, so at
   least `metatile_attr_fields` (or its `_FRLG` alternate mask selection) needs a per-tileset
   override story. Open questions inside this one: does `metatile_attr_field_overrides` also
   take a per-tileset form, or is per-tileset restricted to full-list overrides? Does
   `write_layer_type_column` live next to it, or one level up in project config?

2. **Scope of auto-detection**: Auto-detection must cover stock pokeemerald, pokefirered, and
   pokeemerald-expansion (both emerald and FRLG sides) without requiring user config.
   Unrecognized projects must supply an explicit schema.

## Phased Plan

Phases here are review-chunking units, not compat milestones. Porytiles2 is unreleased; there is
no need to keep `BaseGame` around for diagnostics or migration.

1. **Phase 1 (done)**: Binary format size detection via `MetatilesHeaderProvider`.
2. **Phase 2**: Field-schema abstraction in the domain. Named fields with bit ranges and an
   optional `provider:` block (`header` + `prefix` + optional `skipped` / `format`) that is
   translated into an `EnumSpec` at load time. No hardcoded knowledge of `behavior` /
   `terrain` / `encounter_type` anywhere. `MetatileAttribute` becomes a schema-backed map of
   `field_name -> integer bit value`, with anonymous `attribute_2/3/5/7` members deleted.
   `layer_type` is retained as a first-class member **outside** the field map per the "Layer
   Type" section. This phase also resolves the TODO comment in
   `metatile_attribute.hpp` that currently documents pokefirered's mask layout inline — the
   schema supersedes that comment. Typing lives on the schema, not on the attribute.
3. **Phase 2.5**: Provider consolidation. Collapse `BehaviorMapProvider`,
   `TerrainTypeMapProvider`, and `EncounterTypeMapProvider` into one `EnumMapProvider`
   domain interface, with one `HeaderEnumMapProvider` infra implementation parameterized
   by `EnumSpec`. Must land with or after Phase 2 so the narrow-integer CSV call sites
   disappear before `EnumMapProvider` widens its return type to `uint32_t`. See the
   "Provider Consolidation" section under "Design Direction" for the shape.
4. **Phase 3**: Schema inference via a new `MetatileAttributeConfigProvider` that slots into
   the `config_schema.yaml` provider chain as an inference layer below the explicit YAML
   provider. Implements the two-phase suffix cascade (Phase A: field discovery,
   Phase B: provider classification, Phase C: mask derivation) against `global.fieldmap.h`
   and companion enum headers, **including the `metatile_attr_field_overrides` merge step**
   that applies partial overrides onto the resolved baseline. Validate against stock
   pokeemerald, pokefirered, pokeemerald-expansion (both emerald and FRLG sides). See the
   "Schema Inference via a Config Provider" section above for the rule shape. This includes
   parsing `sMetatileAttrMasks` in `src/fieldmap.c` (exact-name match) to recover masks
   for pokefirered-stock when `global.fieldmap.h` has a declaration enum but no mask defines.
5. **Phase 4**: Per-tileset schema in the project config (both auto-detected and explicit
   override), and `write_layer_type_column` per-tileset knob. This is where the expansion
   dual-mask problem actually gets resolved.
6. **Phase 5**: Drive CSV and provider plumbing from the schema. Remove `BaseGame` from every
   behavioral path.
   - CSV column set is derived directly from the schema field list (provider presence
     determines rendering; there is no separate CSV-visibility flag).
   - CSV cell parse and write look up `field.provider()` in the runtime `ProviderMap`
     (keyed by field name). Provider present means run the cell through it; provider
     absent means treat the cell as a raw integer.
   - `layer_type` CSV handling is governed by `write_layer_type_column` per the "Layer Type"
     section, independent of the schema-field loop.
   - Provider instantiation is one pass over schema fields at command startup
     (replacing the `if (base_game == BaseGame::pokefirered)` gate in each command).
7. **Phase 6**: Delete `BaseGame` enum, `BaseGameDetector`, and `base_game` from configs, CLI,
   and domain. Delete the `metatile_attr_size` YAML key and the `--metatile-attr-size` CLI
   flag at the same time (the detected size from `MetatilesHeaderProvider` is authoritative
   and has no user-facing override). Delete the pokeruby gate inside
   `ImportTilesetCommand::Run`. The project emerges with no global "which game" concept.

Diagnostic messages that previously said "detected as pokefirered" get reworked to describe the
schema ("detected 4-byte attributes with behavior / terrain / encounter_type fields").
