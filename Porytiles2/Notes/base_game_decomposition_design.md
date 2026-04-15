# Design: Decomposing BaseGame into Independent Config Dimensions

**Date**: 2026-04-15
**Status**: Design exploration, not yet planned for implementation

## Problem

The `BaseGame` enum is detected by scanning `include/global.fieldmap.h` for string markers. This is
brittle: pokeemerald-expansion's "Add FRLG" PR added `METATILE_ATTRIBUTE_BEHAVIOR` (previously a
firered-only marker), causing misdetection as pokefirered. Since decomp users modify their projects
freely, hardcoded game presets are inflexible and will keep breaking.

## Key Finding: BaseGame Is a Single Boolean

Every production consumption site checks `== BaseGame::pokefirered`. The distinctions between
pokeemerald, pokeruby, and pokeemerald_expansion are never used for behavioral decisions -- they only
matter for detection disambiguation.

### Production Consumption Sites

| File | Line(s) | What It Controls |
|------|---------|------------------|
| `project_primary_tileset_importer.cpp` | 40 | Binary attr parse format (2 vs 4 byte) |
| `project_tileset_artifact_reader.cpp` | 177 | Binary attr parse format |
| `project_tileset_artifact_writer.cpp` | 645 | Binary attr write format |
| `project_tileset_artifact_writer.cpp` | 818-902 | CSV column format + default filtering |
| `attributes_csv_loader.cpp` | 193-215 | CSV format validation |
| `command_compile_tileset.hpp` | 188 | Terrain/encounter provider instantiation |
| `command_create_tileset.hpp` | 178 | Terrain/encounter provider instantiation |
| `command_decompile_tileset.hpp` | 183 | Terrain/encounter provider instantiation |
| `command_import_tileset.hpp` | 192 | Terrain/encounter provider instantiation |
| `command_import_tileset.hpp` | 182 | Pokeruby "not supported" gate (not behavioral) |

## The Three Behavioral Dimensions

### 1. Binary Attribute Format -- ALREADY DECOMPOSED

- **Config**: `metatile_attr_size` (2 or 4 bytes), in `config_schema.yaml`
- **Auto-detected**: `MetatilesHeaderProvider` scans `metatiles.h` for `const u16` vs `const u32`
- **Overridable**: via CLI `--metatile-attr-size` or YAML `fieldmap.metatile_attribute_size`
- **Action**: Replace `base_game == pokefirered` checks in binary parse/write sites with
  `metatile_attr_size == 4` checks. This makes binary format follow the actually-detected attribute
  type rather than the guessed base game.

### 2. CSV Attribute Schema -- NEEDS DECOMPOSITION

- **Currently**: Hardcoded as 2-column `(id,behavior)` or 4-column `(id,behavior,terrainType,encounterType)`
- **Target**: Derive CSV columns from `METATILE_ATTR_*_MASK` defines in `global.fieldmap.h`
  - One column per mask value
  - Naturally supports custom user-defined attribute fields
  - Auto-detected but overridable via config

Example of what detection would produce:

```
emerald defines:
  METATILE_ATTR_BEHAVIOR_MASK 0x00FF  -> "behavior" column (bits 0-7)
  METATILE_ATTR_LAYER_MASK    0xF000  -> "layer_type" column (bits 12-15)

firered defines:
  METATILE_ATTR_BEHAVIOR  (bits 0-8)  -> "behavior" column
  METATILE_ATTR_TERRAIN   (bits 9-13) -> "terrain" column
  METATILE_ATTR_ENCOUNTER (bits 24-26) -> "encounter_type" column
  METATILE_ATTR_LAYER     (bits 29-30) -> "layer_type" column
  ...etc.

expansion with FRLG:
  Has both sets of defines -- need disambiguation strategy
  (e.g., porytiles.yaml selects which set, or isFrlg per-tileset)
```

### 3. Terrain/Encounter Provider Instantiation -- NEEDS DECOMPOSITION

- **Currently**: Created only when `base_game == pokefirered`
- **Target**: Driven by which attribute fields are configured. If the CSV schema includes a
  "terrain" field, create the terrain provider. If it includes "encounter_type", create the
  encounter provider.
- This falls out naturally from decomposing dimension #2.

## Design Direction: Schema-Driven Attributes

The core idea: instead of asking "which game are you?" and selecting a preset, detect (or configure)
the actual attribute schema. Each `METATILE_ATTR_*_MASK` define becomes a named field with a bit
position, bit width, and CSV column. This is data-driven, extensible, and decomp-friendly.

### Open Questions

1. **How to handle expansion's dual mask sets** (emerald + FRLG)? Options:
   - Config flag like `attribute_format: emerald | frlg`
   - Per-tileset attribute schema (requires `isFrlg` equivalent in porytiles config)
   - Just use the emerald masks since that's what the binary files actually use

2. **Layer type**: currently treated specially (not a CSV column, used internally for tile
   assignment). Should it remain special or become a regular attribute field?

3. **Migration path**: How to phase out BaseGame without breaking existing projects? Probably:
   - Phase 1: Make binary format follow `metatile_attr_size` (not BaseGame)
   - Phase 2: Add CSV schema config/detection
   - Phase 3: Derive terrain/encounter support from schema
   - Phase 4: Remove BaseGame from all behavioral decisions; keep for diagnostics only

4. **Config representation**: What does the CSV schema look like in `porytiles.yaml`? Something like:
   ```yaml
   fieldmap:
     metatile_attribute_fields:
       - name: behavior
         mask: 0x00FF
       - name: layer_type
         mask: 0xF000
   ```

5. **Backwards compatibility**: Existing projects without explicit config should auto-detect and
   work exactly as before. The schema detection from `global.fieldmap.h` masks provides this.
