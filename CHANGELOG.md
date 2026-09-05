# Changelog

All notable changes to Porytiles are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com),
simplified to a single flat list of changes per version.

<!--
Entry format: one bullet per change, ideally starting with a verb that signals the type:
(Added / Changed / Fixed / Removed / Deprecated / etc.).
End each entry with a link to its PR(s), or a specific commit(s) if that's clearer.
  - Fixed blah blah. - [#<pr>](https://github.com/grunt-lucas/porytiles/pull/<pr>))
-->

## [Unreleased]

- Added a `edit-tileset-config` and `edit-project-config` commands as well as a project-wide variant for the dumper, `dump-project-config`. - [#372](https://github.com/grunt-lucas/porytiles/pull/372)

- Added fuzzy tileset name matching for CLI commands that target an existing tileset. E.g. `gTileset_SecretBase`, `SecretBase`, `secret_base`, and `secretBase` all resolve to the same tileset (`gTileset_SecretBase`), and an ambiguous name fails with an error that lists the candidates. `create-tileset` still requires the exact canonical name, and `--allow-missing-tileset` skips resolution. Also centralized `gTileset_` prefix handling behind shared validators. - [#374](https://github.com/grunt-lucas/porytiles/pull/374)

- Added a `tileset.animations.create_sample_anims` config value (CLI: `--create-sample-anims`, default `true`). When disabled, `create-tileset` skips the sample flower animation. - [#375](https://github.com/grunt-lucas/porytiles/pull/375))

- Added a `tileset.import_transparency` config (CLI: `--import-transparency`, default `extrinsic`). It controls how `import-tileset` and `decompile-tileset` write transparent pixels to the RGBA layer images and animation frames. Mode `alpha` writes them with alpha channel set to 0, `extrinsic` writes them as the configured `extrinsic_transparency` color, and `mixed` writes the layer group absent from dual-layer Porymap data as alpha 0, while present-but-transparent pixels get the `extrinsic_transparency` color (this is equivalent to `extrinsic` in triple-layer mode). - [#376](https://github.com/grunt-lucas/porytiles/pull/376)

- Added a `find-tileset-color` command that locates every pixel of a given color in a tileset's RGBA layer images and animation frames, displaying each metatile / animation frame subtile match as ASCII art with the relevant pixels highlighted. Also added a `dump-tileset-colors` command that lists every color with pixel counts and compares the unique color total against the configured color limit. - [#378](https://github.com/grunt-lucas/porytiles/pull/378)

## [2.0.0] - 2026-08-27

- **BREAKING:** Changed the YAML config keys `fieldmap.num_pals_in_primary` and `fieldmap.num_pals_total` to `fieldmap.num_palettes_in_primary` and `fieldmap.num_palettes_total`. The old keys no longer work. A config file still using them will fail with an unknown configuration key error. The `--num-pals-in-primary` and `--num-pals-total` CLI options and the `NUM_PALS_IN_PRIMARY` and `NUM_PALS_TOTAL` header defines are unchanged. - [ba9a4d7](https://github.com/grunt-lucas/porytiles/commit/ba9a4d7cf1ff087901a97a15f39d302b8ffa0cc2)

- **BREAKING:** Changed the CLI option `--metatile-attr-size` to `--metatile-attribute-size`. The old spelling no longer works. The equivalent YAML key, `fieldmap.metatile_attribute_size`, is unchanged. - [16bce0a](https://github.com/grunt-lucas/porytiles/commit/16bce0a7898629ae8f0126bfbb576e121ff14596)

- Fixed `import-tileset` failing to resolve artifact paths for tilesets declared with `INCGFX_*`-style macros (e.g. `INCGFX_U32`) - [#315](https://github.com/grunt-lucas/porytiles/pull/315)

- Fixed tileset names with a `snake_case` segment (e.g. `gTileset_velvet_forest`) producing a mismatched animation callback symbol in `headers.h`, where the `.callback` field kept the raw `snake_case` name while the generated init function used `PascalCase`, breaking the decomp build. - [#317](https://github.com/grunt-lucas/porytiles/pull/317)

- Fixed a same-named animation in a secondary and its paired primary aborting the compiler with an internal panic instead of a diagnostic when cross-tileset animation linking is enabled. - [#331](https://github.com/grunt-lucas/porytiles/pull/331)

- Fixed manual frame-linking overrides in `anim.json` aborting the compiler with an internal panic on an out-of-range `metatile_id`. Gave the manual and `primary_references` override paths consistent diagnostics: both now report for out-of-range `frame_subtile` and `metatile_id`, a `pal_index` that does not fit the hardware palette field, a `pal_index` past the configured palette count, and an override targeting a layer that dual-layer conversion drops. Also fixed a secondary that uses `primary_references` without defining any animations of its own having all of its overrides silently ignored. - [#332](https://github.com/grunt-lucas/porytiles/pull/332)

- Fixed INCBIN declarations for a managed tileset being written inside a trailing preprocessor conditional (such as pokeemerald-expansion's `#if IS_FRLG` block at the end of `graphics.h`/`metatiles.h`) instead of after it, which compiled the data out of a non-FRLG build while the `headers.h` struct still referenced it, breaking the decomp build with `undeclared` errors. Also fixed the same declarations being duplicated when an import was retried. Appending is now an idempotent operation that places declarations after the last non-blank line and fixes any declarations previously misplaced inside such a conditional. - [#333](https://github.com/grunt-lucas/porytiles/pull/333)

- Added a fully configurable metatile attribute schema, replacing the hardcoded `BaseGame` (Emerald vs. FireRed/LeafGreen) system. Includes a `dump-attribute-schema` command that prints the resolved metatile attribute schema for a tileset. There is no more base game auto-detection, nor is there a base game concept at all. A project that declares more than one attribute mask layout, such as pokeemerald-expansion, must now set `fieldmap.metatile_attribute_size` (or pass `--metatile-attribute-size`) to select a layout. - [#336](https://github.com/grunt-lucas/porytiles/issues/336) [#341](https://github.com/grunt-lucas/porytiles/pull/341) [#346](https://github.com/grunt-lucas/porytiles/pull/346)

- Added auto-wrap support for all diagnostic messages to simplify internal callsites. On the user side, diagnostics now dynamically wrap based on terminal width (as reported by ioctl). This can be overridden via the standard `COLUMNS` environment var. - [#342](https://github.com/grunt-lucas/porytiles/pull/342)

- When importing, duplicate key frame detection and mangling now operate using canonical RGBA tiles instead of by pal slot indices. - [#352](https://github.com/grunt-lucas/porytiles/pull/352)

- Changed the Linux release binaries (`porytiles`, `porytiles-legacy`, and the bundled test executables) to be fully statically linked, which currently includes glibc, libpng, and zlib. Porytiles no longer fails to start on distros with an older system glibc (e.g. `GLIBC_2.38' not found` on WSL Ubuntu 22.04). - [#353](https://github.com/grunt-lucas/porytiles/pull/353)

- Backported a DFS/BFS algorithm improvement into legacy Porytiles. - [#355](https://github.com/grunt-lucas/porytiles/pull/355)

- Remarks and warnings are now opt-in instead of opt-out. - [#356](https://github.com/grunt-lucas/porytiles/pull/356)

- Fixed `import-tileset` failing when tilesets used animation arrays named with a double shorthand, e.g. pokeemerald-expansion's `gTileset_General_Frlg`, whose arrays are named `sTilesetAnims_General_*`. The parser can now fall back to "shorthand of the shorthand" and reports each fallback with a remark under an `anim-code-parse` tag. - [#364](https://github.com/grunt-lucas/porytiles/pull/364)

## [1.0.0] - 2026-06-05

First stable release of Porytiles.
The cumulative scope of changes since the pre-1.0.0 legacy version is substantial and not enumerated here.
The project binary is `porytiles`, a complete backwards-incompatible overhaul,
and the prior legacy compiler is preserved as `porytiles-legacy` for users with existing setups.
