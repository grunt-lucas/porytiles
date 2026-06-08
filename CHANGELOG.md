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

## [1.0.0] - 2026-06-05

First stable release of Porytiles.
The cumulative scope of changes since the pre-1.0.0 legacy version is substantial and not enumerated here.
The project binary is `porytiles`, a complete backwards-incompatible overhaul,
and the prior legacy compiler is preserved as `porytiles-legacy` for users with existing setups.
