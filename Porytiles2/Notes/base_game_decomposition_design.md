# Design: Decomposing BaseGame into Independent Config Dimensions

**Date**: 2026-04-15 (revised 2026-04-17)
**Status**: Design exploration, not yet planned for implementation

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

| File | Function | What It Controls |
|------|----------|------------------|
| `project_primary_tileset_importer.cpp` | `ProjectPrimaryTilesetImporter::import_porymap_component_from_vanilla` | Binary attr parse format (2 vs 4 byte), migrated to `metatile_attr_size` |
| `project_tileset_artifact_reader.cpp` | `ProjectTilesetArtifactReader::read_metatile_attributes_bin` | Binary attr parse format, migrated |
| `project_tileset_artifact_writer.cpp` | `ProjectTilesetArtifactWriter::write_metatile_attributes_bin` | Binary attr write format, migrated |
| `project_tileset_artifact_writer.cpp` | `ProjectTilesetArtifactWriter::write_attributes_csv` | CSV column format and default filtering |
| `attributes_csv_loader.cpp` | `parse_attributes_csv` (free function) | CSV format validation vs project's `BaseGame` |
| `command_compile_tileset.hpp` | `CompileTilesetCommand::Run` | Terrain/encounter provider instantiation |
| `command_create_tileset.hpp` | `CreateTilesetCommand::Run` | Terrain/encounter provider instantiation |
| `command_decompile_tileset.hpp` | `DecompileTilesetCommand::Run` | Terrain/encounter provider instantiation |
| `command_import_tileset.hpp` | `ImportTilesetCommand::Run` | Terrain/encounter provider instantiation and pokeruby "not supported" gate |

## The Three Behavioral Dimensions

### 1. Binary Attribute Format -- DONE

- **Config**: `metatile_attr_size` (2 or 4 bytes), in `config_schema.yaml`
- **Auto-detected**: `MetatilesHeaderProvider` scans `metatiles.h` for `const u16` vs `const u32`
- **Overridable**: via CLI `--metatile-attr-size` or YAML `fieldmap.metatile_attribute_size`
- **Completed**: Replaced `base_game == pokefirered` checks in all three binary parse/write sites
  with `metatile_attr_size == attr::bytes_per_attr_firered`. Removed `BaseGame` dependency from
  `ProjectPrimaryTilesetImporter` and `ProjectTilesetArtifactReader` entirely. Added
  `metatile_attr_size` as a resolved constructor parameter to `ProjectTilesetArtifactWriter`
  (which still needs `BaseGame` for CSV writing).

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
4. Mask fits within `metatile_attr_size * 8` bits.

Explicit offset only helps for split or interleaved fields. Gen III decomps do not use
those. Defer until someone asks.

#### Schema Inference via a Config Provider

The attribute schema integrates into the existing `config_schema.yaml` provider chain.
The explicit YAML provider reads user-authored schema entries (with `provider:` blocks
as shown above) directly. A new `MetatileAttributeConfigProvider` sits alongside it as
an **inference layer**: when YAML does not supply an entry, the inference provider
scans the decomp's own headers and synthesizes the same shape of config a user could
have written by hand. No hidden "stock pokefirered preset" lives in Porytiles; only
header scanning driven by documented rules.

Inference is **rule-based**, not preset-based. The rule set is small, documented, and
lives next to the provider code. A few illustrative rules:

- Find `#define METATILE_ATTR_*_MASK` defines in `global.fieldmap.h`. Each becomes a
  named field whose `mask` is the define's value and whose `name` is derived from the
  define name (e.g. `METATILE_ATTR_BEHAVIOR_MASK` -> field name `behavior`).
- If a mask name contains `BEHAVIOR`, assume the enum values live in
  `include/constants/metatile_behaviors.h` with `prefix: MB_` and `format: either`.
- If a mask name contains `TERRAIN`, assume a C enum with `prefix: TILE_TERRAIN_`
  inside `include/global.fieldmap.h` itself and `format: enums_only`.
- If a mask name contains `ENCOUNTER`, assume a C enum with `prefix: TILE_ENCOUNTER_`
  inside `include/global.fieldmap.h` and `format: enums_only`.
- Any mask without a recognized keyword is emitted as a raw field (no provider block).

When a rule fires but the referenced header or prefix yields no entries, the inference
provider reports a warning and falls back to a raw field. The user then writes an
explicit YAML entry to correct the inference. This is the standard escape hatch:
inference is a convenience layer, explicit YAML wins.

Rule-based inference (rather than a `BaseGame`-style preset) makes failures
actionable and localizable. If the `TERRAIN` rule does not fire for a particular fork
(maybe they renamed the constant to `BIOME`), the user overrides exactly that one
field in YAML rather than having to declare a whole new preset. Adding support for
a new fork never requires a Porytiles source change, only a rule tweak or a user-level
config override.

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

`layer_type` is a **tileset-wide structural property** tied to dual-layer vs triple-layer mode
(detected via `PorymapTilesetComponent::detect_layer_mode`), not a per-metatile value stored in
the CSV. CSV load hardcodes `LayerType::normal` inside `parse_attributes_csv`; the real value
comes from the binary.

**Decision**: keep `layer_type` out of the user-visible schema surface for now. The schema
describes per-metatile, per-row CSV-addressable fields, and `layer_type` is not one of those.
Revisit only if users ask for per-metatile layer overrides.

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

## Open Questions

1. **Per-tileset schema representation**: What does the override look like in porytiles config?
   Likely an optional `attribute_format` (or `attribute_schema`) field per tileset, falling back
   to the project-level default when absent. The per-tileset override is needed so expansion
   projects can tag individual tilesets as FRLG-layout without affecting the rest.

2. **Config representation for a full explicit schema**: What does a user-authored schema look
   like in `porytiles.yaml` when auto-detection is not sufficient? Two candidate shapes:
   - **Named preset**: `attribute_format: emerald` / `firered` / `emerald_expansion_frlg` /
     `custom`. Clear to users, limited in power.
   - **Bit-field description**:
     ```yaml
     fieldmap:
       metatile_attribute_fields:
         - name: behavior
           mask: 0x00FF
           provider:
             header: include/constants/metatile_behaviors.h
             prefix: MB_
         - name: custom_flag
           mask: 0x8000
           # no provider block -> raw integer column in CSV
     ```
     Fully general, verbose. If auto-detection covers >95% of cases, a named-preset escape hatch
     is probably preferable to a full bit-field DSL. See the "Typed Fields and Provider Mapping"
     section above for the shape of `provider:`.

3. **Scope of auto-detection**: Auto-detection must cover stock pokeemerald, pokefirered, and
   pokeemerald-expansion (both emerald and FRLG sides) without requiring user config.
   Unrecognized projects must supply an explicit schema.

## Phased Plan

Phases here are review-chunking units, not compat milestones. Porytiles2 is unreleased; there is
no need to keep `BaseGame` around for diagnostics or migration.

1. **Phase 1 (done)**: Binary format via `metatile_attr_size`.
2. **Phase 2**: Field-schema abstraction in the domain. Named fields with bit ranges, a
   CSV-visibility flag, and an optional `provider:` block (`header` + `prefix` +
   optional `skipped` / `format`) that is translated into an `EnumSpec` at load time.
   No hardcoded knowledge of `behavior` / `terrain` / `encounter_type` anywhere.
   `MetatileAttribute` becomes schema-backed; anonymous `attribute_2/3/5/7` members
   deleted. Typing lives on the schema, not on the attribute.
3. **Phase 2.5**: Provider consolidation. Collapse `BehaviorMapProvider`,
   `TerrainTypeMapProvider`, and `EncounterTypeMapProvider` into one `EnumMapProvider`
   domain interface, with one `HeaderEnumMapProvider` infra implementation parameterized
   by `EnumSpec`. Must land with or after Phase 2 so the narrow-integer CSV call sites
   disappear before `EnumMapProvider` widens its return type to `uint32_t`. See the
   "Provider Consolidation" section under "Design Direction" for the shape.
4. **Phase 3**: Schema inference via a new `MetatileAttributeConfigProvider` that slots into
   the `config_schema.yaml` provider chain as an inference layer below the explicit YAML
   provider. Implements a documented rule set that maps mask defines in `global.fieldmap.h`
   (and companion enum headers) to provider blocks. Validate against stock pokeemerald,
   pokefirered, pokeemerald-expansion (both emerald and FRLG sides). See the "Schema
   Inference via a Config Provider" section above for the rule shape.
5. **Phase 4**: Per-tileset schema in the project config (both auto-detected and explicit
   override). This is where the expansion dual-mask problem actually gets resolved.
6. **Phase 5**: Drive CSV and provider plumbing from the schema. Remove `BaseGame` from every
   behavioral path.
   - CSV column set = `{field.name for field in schema if field.csv_visible}`.
   - CSV cell parse and write look up `field.provider()` in the runtime `ProviderMap`
     (keyed by field name). Provider present means run the cell through it; provider
     absent means treat the cell as a raw integer.
   - Provider instantiation is one pass over schema fields at command startup
     (replacing the `if (base_game == BaseGame::pokefirered)` gate in each command).
7. **Phase 6**: Delete `BaseGame` enum, `BaseGameDetector`, and `base_game` from configs, CLI,
   and domain. Delete the pokeruby gate inside `ImportTilesetCommand::Run`. The project emerges
   with no global "which game" concept.

Diagnostic messages that previously said "detected as pokefirered" get reworked to describe the
schema ("detected 4-byte attributes with behavior / terrain / encounter_type fields").
