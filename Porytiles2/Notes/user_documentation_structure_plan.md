# Porytiles User Documentation Structure Plan

## Context

Porytiles v2 is approaching a state where user documentation is needed. The existing `porytiles-user-docs` repo has one complete page (`tile-sharing.md`) and three placeholders. This plan defines the full documentation structure: 14 pages organized into 4 sections, covering everything from GBA background to CLI reference.

The docs use **Sphinx + Read the Docs theme + MyST parser** (markdown files). The existing `tile-sharing.md` sets the tone: clear prose, concrete examples with ASCII diagrams, accessible to users who may be new to GBA tilesets.

---

## Documentation Structure

### Toctree (in `index.rst`)

```rst
.. toctree::
   :maxdepth: 2
   :caption: Background

   gba-tileset-system
   manual-tileset-insertion
   porytiles-concepts

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   installation
   creating-your-first-tileset
   importing-an-existing-tileset

.. toctree::
   :maxdepth: 2
   :caption: Guides

   compile-decompile-workflow
   metatile-attributes
   palette-packing
   tile-sharing
   animations
   diagnostics

.. toctree::
   :maxdepth: 2
   :caption: Reference

   configuration
   cli-reference
```

### Files to delete
- `docsrc/getting-started.md` (placeholder, replaced by `installation.md`)
- `docsrc/guide.md` (placeholder, replaced by guide pages)
- `docsrc/reference.md` (placeholder, replaced by `configuration.md` + `cli-reference.md`)

### Files to preserve
- `docsrc/tile-sharing.md` — already complete (~384 lines). May add cross-reference links to new pages later.

---

## Page-by-Page Breakdown

### Section 1: Background

#### 1. `gba-tileset-system.md` — How the GBA Tileset System Works

Covers GBA hardware fundamentals for users new to tilesets:
- 8x8 tiles, 4bpp indexed color, palette slots 0–15 with slot 0 as transparency
- Metatiles: 16x16 or 24x24 composites of 8x8 subtiles; tilemap entries (tile index + palette index + flip flags)
- Primary vs secondary tilesets: tile count limits, palette allocation (primary gets palettes 0–5, secondary gets 6–12 in default emerald)
- The `fieldmap.h` constants and what they control at a high level
- Artifact files: `tiles.png`, `palettes/*.pal`, `metatiles.bin`, `metatile_attributes.bin`
- How Porymap fits in as the visual editor vs Porytiles as the compiler

**Cross-references:** → `configuration.md` for fieldmap constants, → `metatile-attributes.md` for attribute details

---

#### 2. `manual-tileset-insertion.md` — Inserting a Tileset Manually

Standalone page covering the manual (non-Porytiles) tileset creation process. This is **not just historical** — manual insertion is still needed for certain edge-case optimizations Porytiles can't handle.

- Tools involved: TilemapStudio, Aseprite, Porymap, text editor
- Creating indexed `tiles.png` manually: converting RGBA to 4bpp indexed, arranging 8x8 tiles
- Building JASC-format `.pal` files by hand
- Writing `metatiles.bin`: how metatile entries encode tile index, palette, flip flags
- Writing `metatile_attributes.bin` for behaviors
- Registering the tileset in the decomp project (`tileset.c/h` headers)
- Pain points: re-indexing when palettes change, manual palette slot assignment, error-prone binary editing
- When manual is still the right choice (specific optimizations, edge cases Porytiles doesn't cover)

**Cross-references:** → `gba-tileset-system.md` for terminology, → `creating-your-first-tileset.md` for the Porytiles alternative

---

#### 3. `porytiles-concepts.md` — Porytiles Concepts and Terminology

Bridges GBA background → hands-on tutorials. Introduces Porytiles-specific concepts that span multiple guides:

- **Managed vs unmanaged tilesets**: what makes a tileset "managed" (`tileset-manifest.json`), how `list-tilesets` shows them
- **Project directory structure**: `porytiles/config.yaml`, `porytiles/config.local.yaml`, `porytiles/tilesets/<name>/config.yaml`, `porytiles/tilesets/<name>/config.local.yaml`, `tileset-manifest.json`
- **Porytiles component vs Porymap component**: `porytiles_src/` (RGBA layer PNGs + `attributes.csv`) vs `porytiles_bin/` (indexed artifacts)
- **The compile/decompile duality**: Porytiles assets as "source of truth"
- **Extrinsic transparency**: what it is, why it exists, how to configure
- **Edit modes**: locked / patch / optimize — when each is appropriate
- **Checksum verification**: anti-clobber protection
- **Supported base games**: pokeruby (limited), pokefirered (full, terrain/encounter types), pokeemerald, pokeemerald-expansion

**Cross-references:** → `gba-tileset-system.md` for GBA terms, → `configuration.md` for setting extrinsic transparency and edit modes

---

### Section 2: Getting Started

#### 4. `installation.md` — Installation

Short, procedural:
- Homebrew installation (primary)
- Download release binaries (Linux, macOS, Windows/WSL)
- Building from source (brief, link to dev docs for details)
- Verifying installation (`porytiles2 --version`)
- Shell completion setup (`porytiles2 completion bash/zsh/fish`)

**Cross-references:** → `creating-your-first-tileset.md` as next step

---

#### 5. `creating-your-first-tileset.md` — Creating Your First Tileset

The primary tutorial — step-by-step walkthrough for new users:
- Prerequisites: working pokeemerald/expansion project, Porytiles installed, Porymap installed
- Preparing RGBA layer PNGs (bottom, middle, top): dimensions, transparency rules, extrinsic transparency color
- Writing `attributes.csv`: format, columns, `MB_NORMAL`
- Running `porytiles2 create-tileset gTileset_MyTileset`
- What happens: directory creation, artifact generation
- Running `porytiles2 compile-tileset gTileset_MyTileset` and viewing in Porymap
- Common first-time errors and how to fix them

**Cross-references:** → `gba-tileset-system.md` for "what is a metatile", → `metatile-attributes.md` for attributes, → `configuration.md` for customization

---

#### 6. `importing-an-existing-tileset.md` — Importing an Existing Tileset

Tutorial for bringing pre-existing tilesets under Porytiles management:
- When to import vs create
- Running `porytiles2 import-tileset gTileset_General`
- What happens: Porytiles reads Porymap artifacts, decompiles to RGBA layers + `attributes.csv`, creates manifest
- Verifying the import: re-compile and diff, or visual inspection in Porymap
- Import transparency options (`import_transparency`: alpha, extrinsic, mixed)
- Limitations: pokeruby import not supported
- Brief note on importing animation-bearing tilesets (→ animations page)

**Cross-references:** → `porytiles-concepts.md` for managed vs unmanaged, → `compile-decompile-workflow.md` for ongoing workflow, → `animations.md` for animation import

---

### Section 3: Guides

#### 7. `compile-decompile-workflow.md` — The Compile/Decompile Workflow

The daily-use workflow guide (assumes user already has ≥1 managed tileset):
- The daily loop: edit Porytiles assets → compile → view in Porymap; or edit in Porymap → decompile back
- `compile-tileset`: what it does, when to run it, what artifacts it writes
- `decompile-tileset`: what it does, when to run it, what it regenerates
- Edit modes in practice: how locked/patch/optimize affect compilation
- Checksum verification: what happens on external edit detection, how to resolve
- Integrating with build systems (Makefile rules)
- Using `dump-tileset-config` to debug configuration
- Using `list-tilesets` to see project state

**Cross-references:** → `porytiles-concepts.md` for edit modes, → `configuration.md` for config tuning, → `diagnostics.md` for reading output

---

#### 8. `metatile-attributes.md` — Metatile Attributes

Reference-style guide for `attributes.csv`:
- File format: columns, delimiters, row ordering
- Behavior field: metatile behaviors, `metatile_behaviors.h` defines
- Layer type / layer mode
- Encounter type (FireRed only)
- Terrain type (FireRed only)
- Attribute size: 2 bytes (Emerald/Ruby) vs 4 bytes (FireRed)
- How attributes map to metatile indices
- Common behaviors table

**Cross-references:** → `gba-tileset-system.md` for metatile basics, → `configuration.md` for `metatile-attr-size`

---

#### 9. `palette-packing.md` — Palette Packing

Deep guide structured as "basics first, tuning later":
- What palette packing is: assigning tiles to hardware palettes under the 15-color constraint
- The three strategies: `best_fusion` (fast greedy), `backtracking` (default, BFS/DFS), `overload_and_remove` (multi-start retries)
- When to use each strategy
- Per-strategy tuning parameters:
  - backtracking: `search_algorithm`, `node_cutoff`, `best_branches`, `smart_prune`
  - overload_and_remove: `max_attempts`, `seed`, `shuffle_strategy`
- Palette hints: what they are, YAML syntax, when they help
- `pal_hints_enabled` toggle
- Relationship to tile sharing packing modes

**Cross-references:** → `tile-sharing.md`, → `configuration.md` for YAML syntax, → `diagnostics.md` for packing diagnostics

---

#### 10. `tile-sharing.md` — Tile Sharing (EXISTING, PRESERVED)

Already complete at ~384 lines. No content changes needed. May add cross-reference links to `palette-packing.md`, `configuration.md`, and `diagnostics.md` later.

---

#### 11. `animations.md` — Animations

The most complex subsystem — needs significant depth:
- What tile animations are on the GBA: frame sequences swapping tile data at runtime
- Animation directory structure: frame PNGs, `key.png`, `anim.json`
- Frame linking modes: automatic (key.png), manual (anim.json), hybrid (NYI)
- Palette resolution strategies: `scan_local_metatiles`, `palette-00`–`palette-15`, `internal-png-palette`, `scan-all-tilesets` (NYI)
- Key frame resolution: `error`, `warning`, `mangle`
- Multi-palette subtile resolution: `error`, `warning`, `split` (NYI)
- Per-animation overrides: the three-tier cascade (per-tile > per-animation > global)
- `wire_anim_code`: automatic vs manual wiring into `tileset_anims.c/h`
- Worked example: importing an animated tileset, modifying an animation

**Cross-references:** → `porytiles-concepts.md` for base game differences, → `configuration.md` for animation config, → `importing-an-existing-tileset.md` for animation import

---

#### 12. `diagnostics.md` — Understanding Diagnostics

How to read and control Porytiles output:
- Four severity levels: remarks, warnings, errors, fatal
- How to read error chain output (proximate → steps → root cause)
- Tag-based regex filtering: the exclude/include system, how include overrides exclude
- CLI flags: `--diagnostic-warnings-exclude`, `--diagnostic-warnings-include`, `--diagnostic-remarks-exclude`, `--diagnostic-remarks-include`
- YAML equivalents
- Common diagnostic tags overview table (with links to topic-specific pages)
- Suppressing all warnings/remarks, selectively enabling specific tags

**Cross-references:** → `tile-sharing.md` for tile-sharing diagnostics, → `configuration.md` for diagnostic config values

---

### Section 4: Reference

#### 13. `configuration.md` — Configuration Reference

The exhaustive config reference:
- The layered provider system (5 layers in priority order): CLI > YAML per-tileset local > YAML per-tileset > YAML project local > YAML project > Header defines > Defaults
- YAML file locations and precedence
- How to use `dump-tileset-config` to inspect provenance
- Which values are YAML-only (palette hints, strategy params, per-animation overrides)
- **Full config value listing**, organized by YAML path group:
  - `fieldmap.*` (9 values)
  - `tileset.extrinsic_transparency`, `tileset.import_transparency`
  - `tileset.tiles.*` (edit_mode, palette_mode, sharing.packing, sharing.alignment)
  - `tileset.palettes.*` (edit_mode, packing.strategy, packing.strategy_params, packing.hints_enabled, packing.hints)
  - `tileset.animations.*` (all animation config + per_animation_overrides)
  - `tileset.paths.*` (primary/secondary src/bin)
  - `diagnostics.*` (warning/remark filters)
  - `verify_checksums`
- For each value: canonical name, YAML path, CLI flag (or "YAML-only"), type, default, description, link to relevant guide
- Cross-field validation rules
- The example YAML file (reproduce or reference `porytiles.example.yaml`)

**Cross-references:** Every config value links to its conceptual guide page

---

#### 14. `cli-reference.md` — CLI Command Reference

Man-page style reference for all 7 commands:
- Global options: `-C/--project-root`, `-V/--version`, `--help`
- **COMMANDS group:**
  - `create-tileset` — synopsis, description, positional args, relevant config
  - `import-tileset` — synopsis, description, positional args, limitations
  - `compile-tileset` — synopsis, description, positional args
  - `decompile-tileset` — synopsis, description, positional args
- **UTILITIES group:**
  - `dump-tileset-config` — synopsis, description, output format
  - `list-tilesets` — synopsis, filtering modes (all/managed/unmanaged), `--prefix`
  - `completion` — synopsis, supported shells (bash/zsh/fish), setup instructions

Each command gets a usage example and links to its guide page.

**Cross-references:** → guide pages for each command's workflow context

---

## Page Summary

| #  | File                               | Section         | Status              |
|----|------------------------------------|-----------------|---------------------|
| 1  | `gba-tileset-system.md`            | Background      | New                 |
| 2  | `manual-tileset-insertion.md`      | Background      | New                 |
| 3  | `porytiles-concepts.md`            | Background      | New                 |
| 4  | `installation.md`                  | Getting Started | New                 |
| 5  | `creating-your-first-tileset.md`   | Getting Started | New                 |
| 6  | `importing-an-existing-tileset.md` | Getting Started | New                 |
| 7  | `compile-decompile-workflow.md`    | Guides          | New                 |
| 8  | `metatile-attributes.md`           | Guides          | New                 |
| 9  | `palette-packing.md`               | Guides          | New                 |
| 10 | `tile-sharing.md`                  | Guides          | Existing (preserve) |
| 11 | `animations.md`                    | Guides          | New                 |
| 12 | `diagnostics.md`                   | Guides          | New                 |
| 13 | `configuration.md`                 | Reference       | New                 |
| 14 | `cli-reference.md`                 | Reference       | New                 |

## Implementation Approach

This is a **documentation structure plan only** — actual content writing will happen page-by-page. Implementation order should follow the toctree order (background → getting started → guides → reference) since later pages reference earlier ones.

### Step 1: Restructure the toctree
- Update `docsrc/index.rst` with the 4-section toctree above
- Delete the 3 placeholder files (`getting-started.md`, `guide.md`, `reference.md`)
- Create stub files for all 13 new pages (title + brief description placeholder)
- Verify the Sphinx build succeeds with stubs

### Step 2: Write pages (one at a time, in toctree order)
Each page should match the tone of `tile-sharing.md`: clear prose, concrete examples, ASCII diagrams where helpful, accessible to newcomers.

### Verification
- `cd porytiles-user-docs/docsrc && uv run make html` — build must succeed with no warnings
- Visual inspection of built HTML at `docsrc/_build/html/index.html`
- Cross-reference links resolve correctly
