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
         - name: layer_type
           mask: 0xF000
     ```
     Fully general, verbose. If auto-detection covers >95% of cases, a named-preset escape hatch
     is probably preferable to a full bit-field DSL.

3. **Scope of auto-detection**: Auto-detection must cover stock pokeemerald, pokefirered, and
   pokeemerald-expansion (both emerald and FRLG sides) without requiring user config.
   Unrecognized projects must supply an explicit schema.

## Phased Plan

Phases here are review-chunking units, not compat milestones. Porytiles2 is unreleased; there is
no need to keep `BaseGame` around for diagnostics or migration.

1. **Phase 1 (done)**: Binary format via `metatile_attr_size`.
2. **Phase 2**: Field-schema abstraction in the domain. Named fields with bit ranges and a
   CSV-visibility flag, nothing else. `MetatileAttribute` becomes schema-backed; anonymous
   `attribute_2/3/5/7` members deleted.
3. **Phase 3**: Auto-detection of the schema from `global.fieldmap.h` mask defines. Validate
   against stock pokeemerald, pokefirered, pokeemerald-expansion (both emerald and FRLG sides).
4. **Phase 4**: Per-tileset schema in the project config (both auto-detected and explicit
   override). This is where the expansion dual-mask problem actually gets resolved.
5. **Phase 5**: Drive CSV write/read validation and terrain/encounter provider instantiation from
   the schema. Remove `BaseGame` from every behavioral path.
6. **Phase 6**: Delete `BaseGame` enum, `BaseGameDetector`, and `base_game` from configs, CLI,
   and domain. Delete the pokeruby gate inside `ImportTilesetCommand::Run`. The project emerges
   with no global "which game" concept.

Diagnostic messages that previously said "detected as pokefirered" get reworked to describe the
schema ("detected 4-byte attributes with behavior / terrain / encounter_type fields").
