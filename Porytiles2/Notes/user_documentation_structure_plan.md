# Porytiles User Documentation Structure Plan

## Context

Porytiles v2 is approaching a state where user documentation is needed. The existing `porytiles-user-docs` repo has one complete page (`tile-sharing.md`) and three placeholders. This plan defines the full documentation structure: 14 pages organized into 5 sections, covering everything from GBA background to CLI reference.

The docs use **Sphinx + Read the Docs theme + MyST parser** (markdown files). The existing `tile-sharing.md` sets the tone: clear prose, concrete examples with ASCII diagrams, accessible to users who may be new to GBA tilesets.

This structure follows the [Diataxis framework](https://diataxis.fr/): explanation (Background), tutorials (Getting Started), how-to guides (How-to Guides), and reference (Reference). The "Topics" section covers deep-dive explanation+reference hybrid pages for Porytiles subsystems that are too complex for pure reference but too detailed for pure explanation.

---

## Documentation Structure

### Toctree (in `index.rst`)

```rst
.. toctree::
   :maxdepth: 2
   :caption: Background

   gba-decomp-tileset-system
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
   :caption: How-to Guides

   compile-decompile-workflow

.. toctree::
   :maxdepth: 2
   :caption: Topics

   palette-packing
   tile-sharing
   animations
   diagnostics

.. toctree::
   :maxdepth: 2
   :caption: Reference

   metatile-attributes
   configuration
   cli-reference
```

### Files to delete
- `docsrc/getting-started.md` (placeholder, replaced by `installation.md`)
- `docsrc/guide.md` (placeholder, replaced by how-to guides and topic pages)
- `docsrc/reference.md` (placeholder, replaced by `configuration.md` + `cli-reference.md`)

### Files to preserve
- `docsrc/tile-sharing.md` — already complete (~384 lines). May add cross-reference links to new pages later.

---

## Page-by-Page Breakdown

### Section 1: Background

#### 1. `gba-decomp-tileset-system.md` — How the GBA + Gen III Decomp Tileset System Works

Covers GBA hardware + Gen III decomp fundamentals for users new to tilesets:
- 8x8 tiles, 4bpp indexed color, palette slots 0–15 with slot 0 as transparency
- Metatiles: 16x16 composites of 8x8 subtiles (2x2 tiles); tilemap entries (tile index + palette index + flip flags)
- Metatile dual vs triple layer concept
- Primary vs secondary tilesets: tile count limits, palette allocation (primary gets palettes 0–5, secondary gets 6–12 in default emerald)
- The `fieldmap.h` constants and what they control at a high level
- Artifact files: `tiles.png`, `palettes/*.pal`, `metatiles.bin`, `metatile_attributes.bin`
- How Porymap fits in as the visual editor with some overlapping features
  - Porytiles + Porymap together are a "super suite" of tools and are truly complementary
  - Porytiles is designed with this in mind, assumes you are using both tools in a complementary way

**Cross-references:** → `configuration.md` for fieldmap constants, → `metatile-attributes.md` for attribute details

---

#### 2. `manual-tileset-insertion.md` — Inserting a Tileset Manually

Standalone page covering the manual (non-Porytiles) tileset creation process. This is **not just historical** — manual insertion is still needed for certain edge-case optimizations Porytiles can't handle.

- Tools involved: TilemapStudio, Aseprite, Porymap, text editor
- Creating a new tileset via Porymap
- Creating indexed `tiles.png` manually: converting RGBA to 4bpp indexed, arranging 8x8 tiles
- Building JASC-format `.pal` files by hand
- `metatiles.bin`: how metatile entries encode tile index, palette, flip flags
- `metatile_attributes.bin` for behaviors
- How to edit `metatiles.bin` and `metatile_attributes.bin` via Porymap (binary editing is not necessary)
- Pain points: re-indexing when palettes change, manual palette slot assignment, tedious metatile painting
- When manual is still the right choice (specific optimizations, edge cases Porytiles doesn't cover)

**Cross-references:** → `gba-decomp-tileset-system.md` for terminology, → `creating-your-first-tileset.md` for the Porytiles alternative

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

**Cross-references:** → `gba-decomp-tileset-system.md` for GBA terms, → `configuration.md` for setting extrinsic transparency and edit modes

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

**Cross-references:** → `gba-decomp-tileset-system.md` for "what is a metatile", → `metatile-attributes.md` for attributes, → `configuration.md` for customization

---

#### 6. `importing-an-existing-tileset.md` — Importing an Existing Tileset

Tutorial for bringing pre-existing tilesets under Porytiles management:
- When to import vs create
- Running `porytiles2 import-tileset gTileset_General`
- What happens: Porytiles reads Porymap artifacts, decompiles to RGBA layers + `attributes.csv`, creates manifest
- Verifying the import: re-compile and diff, or visual inspection in Porymap
- Import transparency options (`import_transparency`: alpha, extrinsic, mixed)
- Limitations: pokeruby import not currently supported
- Brief note on importing animation-bearing tilesets (→ animations page)

**Cross-references:** → `porytiles-concepts.md` for managed vs unmanaged, → `compile-decompile-workflow.md` for ongoing workflow, → `animations.md` for animation import

---

### Section 3: How-to Guides

#### 7. `compile-decompile-workflow.md` — The Compile/Decompile Workflow

**Diataxis type: How-to guide.** Task-oriented, assumes competence, focused on the daily workflow.

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

### Section 4: Topics

These pages cover Porytiles subsystems that require both conceptual explanation and detailed reference material. Each page is structured in two parts: **explanation prose first** (what it is, how it works, when to use it), followed by a clearly marked **reference section** (exact parameters, modes, formats). This hybrid structure is a pragmatic choice — splitting each into separate explanation and reference pages would fragment tightly coupled content.

#### 8. `palette-packing.md` — Palette Packing

**Diataxis type: Explanation + Reference hybrid.** Structured as explanation first, reference second.

Deep guide structured as "basics first, tuning later":
- What palette packing is: assigning tiles to hardware palettes under the 15-color constraint
- The three strategies: `best_fusion` (fast greedy), `backtracking` (default, BFS/DFS), `overload_and_remove` (multi-start retries)
- When to use each strategy
- Per-strategy tuning parameters:
  - backtracking: `search_algorithm`, `node_cutoff`, `best_branches`, `smart_prune`
  - overload_and_remove: `max_attempts`, `seed`, `shuffle_strategy`
- Palette hints: what they are, YAML syntax, when they help
- `pal_hints_enabled` toggle
- How to troubleshoot, common failure causes
- Relationship to tile sharing packing modes

**Cross-references:** → `tile-sharing.md`, → `configuration.md` for YAML syntax, → `diagnostics.md` for packing diagnostics

---

#### 9. `tile-sharing.md` — Tile Sharing (EXISTING, PRESERVED)

Already complete at ~384 lines. No content changes needed. May add cross-reference links to `palette-packing.md`, `configuration.md`, and `diagnostics.md` later.

---

#### 10. `animations.md` — Animations

**Diataxis type: Explanation + Reference hybrid.** Structured as explanation first, reference second.

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

#### 11. `diagnostics.md` — Understanding Diagnostics

**Diataxis type: Explanation + Reference hybrid.** Structured as explanation first, reference second.

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

### Section 5: Reference

#### 12. `metatile-attributes.md` — Metatile Attributes Reference

**Diataxis type: Reference.** Information-oriented, organized by the structure of the data format.

Reference for `attributes.csv`:
- File format: columns, delimiters, row ordering
- Behavior field: metatile behaviors, `metatile_behaviors.h` defines
- Layer type / layer mode
- Encounter type (FireRed only)
- Terrain type (FireRed only)
- Attribute size: 2 bytes (Emerald/Ruby) vs 4 bytes (FireRed)
- How attributes map to metatile indices
- Common behaviors table
- future support for completely customizable attributes

**Cross-references:** → `gba-decomp-tileset-system.md` for metatile basics, → `configuration.md` for `metatile-attr-size`

---

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

| #  | File                               | Section         | Diataxis Type              | Status              |
|----|------------------------------------|-----------------|----------------------------|---------------------|
| 1  | `gba-decomp-tileset-system.md`     | Background      | Explanation                | New                 |
| 2  | `manual-tileset-insertion.md`      | Background      | Explanation                | New                 |
| 3  | `porytiles-concepts.md`            | Background      | Explanation                | New                 |
| 4  | `installation.md`                  | Getting Started | Tutorial (prerequisite)    | New                 |
| 5  | `creating-your-first-tileset.md`   | Getting Started | Tutorial                   | New                 |
| 6  | `importing-an-existing-tileset.md` | Getting Started | Tutorial                   | New                 |
| 7  | `compile-decompile-workflow.md`    | How-to Guides   | How-to guide               | New                 |
| 8  | `palette-packing.md`              | Topics          | Explanation + Reference    | New                 |
| 9  | `tile-sharing.md`                  | Topics          | Explanation + Reference    | Existing (preserve) |
| 10 | `animations.md`                    | Topics          | Explanation + Reference    | New                 |
| 11 | `diagnostics.md`                   | Topics          | Explanation + Reference    | New                 |
| 12 | `metatile-attributes.md`           | Reference       | Reference                  | New                 |
| 13 | `configuration.md`                 | Reference       | Reference                  | New                 |
| 14 | `cli-reference.md`                 | Reference       | Reference                  | New                 |

## Implementation Approach

This is a **documentation structure plan only** — actual content writing will happen page-by-page. Implementation order should follow the toctree order (background → getting started → how-to guides → topics → reference) since later pages reference earlier ones.

### Step 1: Restructure the toctree
- Update `docsrc/index.rst` with the 5-section toctree above
- Delete the 3 placeholder files (`getting-started.md`, `guide.md`, `reference.md`)
- Create stub files for all 13 new pages (title + brief description placeholder)
- Verify the Sphinx build succeeds with stubs

### Step 2: Write pages (one at a time, in toctree order)
Each page should match the tone of `tile-sharing.md`: clear prose, concrete examples, ASCII diagrams where helpful, accessible to newcomers.

### Verification
- `cd porytiles-user-docs/docsrc && uv run make html` — build must succeed with no warnings
- Visual inspection of built HTML at `docsrc/_build/html/index.html`
- Cross-reference links resolve correctly
