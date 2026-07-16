# Changelog

All notable changes to Porytiles are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com),
simplified to a single flat list of changes per version.

<!--
Entry format: one bullet per change, starting with a verb that signals the type:
(Added / Changed / Fixed / Removed / Deprecated).
End each entry with a link to its PR,
and to its issue too when one was filed
(some reports, e.g. Discord messages, have no issue and so link only the PR):
  - Fixed ... ([#<issue>](https://github.com/grunt-lucas/porytiles/issues/<issue>), [#<pr>](https://github.com/grunt-lucas/porytiles/pull/<pr>))
-->

## [Unreleased]

- Fixed `import-tileset` failing to resolve artifact paths for tilesets declared with `INCGFX_*`-style macros (e.g. `INCGFX_U32`) - [#315](https://github.com/grunt-lucas/porytiles/pull/315)

- Fixed tileset names with a `snake_case` segment (e.g. `gTileset_velvet_forest`) producing a mismatched animation callback symbol in `headers.h`, where the `.callback` field kept the raw `snake_case` name while the generated init function used `PascalCase`, breaking the decomp build. - [#317](https://github.com/grunt-lucas/porytiles/pull/317)

- Fixed a same-named animation in a secondary and its paired primary aborting the compiler with an internal panic instead of a diagnostic when cross-tileset animation linking is enabled. - [#331](https://github.com/grunt-lucas/porytiles/pull/331)

- Fixed manual frame-linking overrides in `anim.json` aborting the compiler with an internal panic on an out-of-range `metatile_id`. Gave the manual and `primary_references` override paths consistent diagnostics: both now report for out-of-range `frame_subtile` and `metatile_id`, a `pal_index` that does not fit the hardware palette field, a `pal_index` past the configured palette count, and an override targeting a layer that dual-layer conversion drops. Also fixed a secondary that uses `primary_references` without defining any animations of its own having all of its overrides silently ignored. - [#332](https://github.com/grunt-lucas/porytiles/pull/332)

- Fixed INCBIN declarations for a managed tileset being written inside a trailing preprocessor conditional (such as pokeemerald-expansion's `#if IS_FRLG` block at the end of `graphics.h`/`metatiles.h`) instead of after it, which compiled the data out of a non-FRLG build while the `headers.h` struct still referenced it, breaking the decomp build with `undeclared` errors. Also fixed the same declarations being duplicated when an import was retried. - [#333](https://github.com/grunt-lucas/porytiles/pull/333)

- Added a fully configurable metatile attribute schema, replacing the hardcoded `BaseGame` (Emerald vs. FireRed/LeafGreen) system. - [#336](https://github.com/grunt-lucas/porytiles/issues/336) [#341](https://github.com/grunt-lucas/porytiles/pull/341) [#346](https://github.com/grunt-lucas/porytiles/pull/346)

- Added auto-wrap support for all diagnostic messages to simplify internal callsites. On the user side, diagnostics now dynamically wrap based on terminal width (as reported by ioctl). This can be overridden via the standard `COLUMNS` environment var. - [#342](https://github.com/grunt-lucas/porytiles/pull/342)

- When importing, duplicate key frame detection and mangling now operate using canonical RGBA tiles instead of by pal slot indices. - [#352](https://github.com/grunt-lucas/porytiles/pull/352)

## [1.0.0] - 2026-06-05

First stable release of Porytiles.
The cumulative scope of changes since the pre-1.0.0 legacy version is substantial and not enumerated here.
The project binary is `porytiles`, a complete backwards-incompatible overhaul,
and the prior legacy compiler is preserved as `porytiles-legacy` for users with existing setups.
