# Porytiles 1.0.0 Release Prep — Working Tracker

This is the **operational** tracking document for the Porytiles 1.0.0 release. It
distills the locked-in decisions, the phase outline, and a tickable checklist so a
session can be directed with "execute Phase A2 from `RELEASE_PREP_1_0_0.md`."

The full planning rationale lives in
[`Porytiles2/Notes/release_1_0_0_prep_plan.md`](Porytiles2/Notes/release_1_0_0_prep_plan.md).
That file is the *why*; this file is the *what's done / what's next*. When the two
disagree, the planning doc is authoritative for intent; update this tracker to match.

---

## Status dashboard

| Phase | Title | Status |
|-------|-------|--------|
| 0  | Document the plan in the repo (this file) | ✅ Done |
| A1 | Legacy include-prefix rename (collision unblocker) | ✅ Done |
| A2 | Porytiles2 → Porytiles (the big rename) | ✅ Done |
| A3 | Porytiles1 → Legacy directory rename | ✅ Done |
| A4 | Scripts, configs, IDE files, docs sweep | ✅ Done |
| A5 | GitHub Actions hardcoded paths | ✅ Done |
| A6 | Phase A end-to-end verification | ✅ Done |
| B  | CHANGELOG infrastructure | ✅ Done |
| C  | Versioning system | ⬜ Not started |
| D  | CI / release pipeline overhaul | ⬜ Not started |
| E  | Gitflow adoption + 1.0.0 cut | ⬜ Not started |
| F  | Documentation repos gitflow alignment | ⬜ Not started |
| G  | AI policy documentation | 🟡 In progress |

Legend: ⬜ not started · 🟡 in progress · ✅ done · 🚫 blocked

---

## Critical sequencing invariant

**A1 (Legacy include-prefix rename) MUST land on `develop` before A2 (Porytiles2
rename) merges.** Both codebases currently want `<dir>/include/porytiles/`; until
Porytiles1's prefix becomes `porytiles_legacy/`, any TU including a bare
`porytiles/...` header is ambiguous. This dependency is non-negotiable. Everything
else can be reordered or parallelized.

Other dependencies:
- **C** depends on **A2** (CMake target names must match).
- **D** depends on **A** and **C** complete.
- **E** is last; happens when A–D are stable on `develop`.
- **B**, **F**, **G** can land in parallel with **A** (no source-file overlap).

---

## Open blockers (must be resolved before tagging `v1.0.0`)

- [ ] **1.0.0 public API surface / `STABILITY.md`** — classify each surface (CLI
  flags, YAML config schema, output file formats, project layout, exit codes,
  diagnostic codes vs. message text, C++ library API, CMake targets, build
  requirements) as **stable / experimental / internal**, plus a deprecation policy.
  Release-cut blocker (Phase E2). See "Open design question" in the planning doc.
- [ ] **Open feature branches audit** — decide for each of `bug/anim-tiles`,
  `bug/issue-0060/key-frame-bug`, `feature/issue-0047/compiled-paired-primary`:
  hand-port onto post-rename `develop`, or close. NOT a release prerequisite (can
  ship in 1.0.x), but audit before tagging (Phase E1).

---

## Locked-in decisions

**Renames**
- `Porytiles2/` → `Porytiles/`; namespace `porytiles2` → `porytiles`; include prefix
  `porytiles2/` → `porytiles/`; executable `porytiles2` → `porytiles`.
- `Porytiles1/` → `Legacy/`; namespace `porytiles1` → `porytiles_legacy`; include
  prefix `porytiles/` (inside Porytiles1) → `porytiles_legacy/`; executable
  `porytiles` → `porytiles-legacy`.
- CMake targets: `Porytiles2Lib`→`PorytilesLib`, `Porytiles2Driver`→`PorytilesDriver`,
  `Porytiles2{Unit,Integration,All}Tests`→`Porytiles{Unit,Integration,All}Tests`;
  legacy mirror `Porytiles1Lib`→`LegacyLib`, etc.

**CHANGELOG.md** — simplified Keep A Changelog (flat list, no Added/Removed/Modified
split). Repo root. Starts at `## [1.0.0]`; ongoing work under `## [Unreleased]`.

**Snapshot release** — rolling `snapshot` tag, force-replaced on every push to
`develop`. Version string `1.0.0-snapshot.YYYYMMDDHHMMSS.<short-sha>` (full SHA in
`--version` for bug reports). Both binaries bundled per platform. Concurrency-grouped
so back-to-back develop pushes serialize.

**Versioned release** — triggered on `v[0-9]+.[0-9]+.[0-9]+` pushed to `master`;
preserved permanently; same four-platform matrix; both binaries bundled.

**Homebrew** (`grunt-lucas/homebrew-porytiles` tap) — two formulas. `porytiles.rb`
tracks latest `v*` tag (versioned-release workflow). `porytiles@snapshot.rb` tracks
rolling snapshot (snapshot workflow). Both install both binaries.

**Gitflow** — `develop` = integration (push triggers snapshot); `master` = production
(created at 1.0.0 cut, tag pushes trigger versioned release); `release/<v>` cut from
develop; `hotfix/<v>` cut from master. Both `release/*` and `hotfix/*` merge back to
BOTH `master` and `develop`.

**Version source of truth** — top-level `CMakeLists.txt`
`project(Porytiles VERSION 1.0.0 ...)`. Sub-CMakeLists read `${CMAKE_PROJECT_VERSION}`
(NOT `${PROJECT_VERSION}` — sub-projects shadow it). CI overrides via existing
`-DPORYTILES_BUILD_VERSION_=...`.

**Pre-1.0 tags** (`0.0.1`–`0.0.7`, `nightly-3a9d31c...`) — preserved as historical
artifacts; do not delete.

**Docs repos versioning** — `porytiles-user-docs` and `porytiles-dev-docs` adopt the
same gitflow. GH Pages deploys ONLY from `master` HEAD (public site always shows
latest stable). Docs tags mirror main repo tags exactly; each release cut tags all
three repos in lockstep.

**Main repo Doxygen GH Pages** (`grunt-lucas.github.io/porytiles/`) — same stable-only
model; `build_pages.yml` rewritten to trigger on `push: branches: [master]`. Snapshot
Doxygen is local-only (`cmake --build porytiles-build-debug --target doxygen`).

**AI contribution policy** — `AI-POLICY.md` at repo root, two tracks. Legacy is
AI-free. Active Porytiles accepts AI-assisted contributions at maintainer discretion,
slop rejected. No automated detector, no disclosure checkbox.

---

## Phase 0 — Document the plan in the repo

- [x] Write `RELEASE_PREP_1_0_0.md` (this file) at repo root with locked-in
  decisions, phase outline, and a tickable checklist.

---

## Phase A — Renames (land on `develop` via incremental PRs)

> ⚠️ After A1, A2, or A3 lands, every clone must `rm -rf porytiles-build-*` before
> rebuilding — stale CMake caches reference old target names.

### A1 — Legacy include-prefix rename (collision unblocker) — must merge before A2

- [x] Rename `Porytiles1/include/porytiles/` → `Porytiles1/include/porytiles_legacy/` (via `git mv`).
- [x] Sweep external-style includes (`#include <porytiles/...>` → `<porytiles_legacy/...>`). Actual scope was 1 file (`Porytiles1/tools/driver/main.cpp`, 8 lines), not the planned 55 — lib-internal includes are PRIVATE-path-relative and follow `CANONICAL_LIB_NAME` automatically.
- [x] `Porytiles1/lib/CMakeLists.txt`: `CANONICAL_LIB_NAME "porytiles"` → `"porytiles_legacy"`.
- [x] `Porytiles1/tests/CMakeLists.txt`: same `CANONICAL_LIB_NAME` change (duplicated config — missed in original plan).
- [x] `Porytiles1/CMakeLists.txt`: `PORYTILES1_INCLUDE_DIR` unchanged — it points at `Porytiles1/include/` (parent), so the renamed subdir is reached via `${PORYTILES1_INCLUDE_DIR}/${CANONICAL_LIB_NAME}` automatically. Variable name itself renames in A3.
- [x] Namespace `porytiles1` → `porytiles_legacy` across all 32 source files (1,374 occurrences total): `namespace porytiles1` open/close, `porytiles1::` qualified refs, single `using namespace porytiles1` in `diagnostics.cpp`.
- [x] Verify `Porytiles1Driver`, `Porytiles1Tests` still build and run — `Porytiles1Tests` passes 73/73 cases / 2,689,245 assertions; `porytiles --help` and `--version` work.
- [x] (Directory name stays `Porytiles1/` — that rename is A3.)
- [x] Commit + open PR to `develop`; merge.

### A2 — Porytiles2 → Porytiles (the big rename) — after A1 merged

Executed via two-pass strategy (proven by A1, scales here): pass 1 = namespace
sweep only, build + test verify; pass 2 = directory rename + include paths +
CMake skeleton + generator script + templates, regenerate, build + test.

**A2a — Directory + CMake skeleton**
- [x] Rename `Porytiles2/` → `Porytiles/` (via `git mv`; git rename-detection clean).
- [x] Root `CMakeLists.txt`: `add_subdirectory(Porytiles2)` → `add_subdirectory(Porytiles)`.
- [x] `PORYTILES2_INCLUDE_DIR` → `PORYTILES_INCLUDE_DIR` everywhere.
- [x] CMake targets renamed (`Porytiles2Lib`→`PorytilesLib`, Driver, three Tests).
- [x] `Porytiles/lib/CMakeLists.txt`: `project(Porytiles2Lib CXX)`→`PorytilesLib`;
  `CANONICAL_LIB_NAME "porytiles2"`→`"porytiles"`; `export()` file renamed.
- [x] `Porytiles/tools/driver/CMakeLists.txt`: project rename; `OUTPUT_NAME "porytiles2"`→`"porytiles"`.
- [x] `Porytiles/tests/CMakeLists.txt`: all three test-target names.
- [x] `Porytiles/CMakeLists.txt`: all generated-file paths + `config_templates/` references.
- [x] `Documentation/CMakeLists.txt`: `Porytiles2Lib`, `PORYTILES2_PUBLIC_HEADER*`, three source paths.

**A2b — Include directory + namespace sweep**
- [x] Rename `Porytiles/include/porytiles2/` → `Porytiles/include/porytiles/` (via `git mv`).
- [x] `#include "porytiles2/` → `#include "porytiles/` (1,830 occurrences pre-sweep — actual count, plan said ~1,670).
- [x] `namespace porytiles2` → `namespace porytiles` (799 pre-sweep — plan said ~722). Pattern catches namespace decls, `using namespace`, and closing comments in one shot.
- [x] `porytiles2::` → `porytiles::` qualified refs (135 pre-sweep).
- [x] `command_completion.hpp` shell completion literals (≈37 occurrences): bash/zsh/fish function names like `_porytiles2_completions` and CLI strings like `complete -c porytiles2` — these are not include/namespace patterns but represent the executable name, so they swept in pass 2 along with `OUTPUT_NAME`. Plan flagged this as an unknown; turns out completion is hardcoded (not `argv[0]`-derived).
- [x] One tmpdir literal in `anim_json_parser_override_test.cpp` (`porytiles2_test_anim_json_parser` → `porytiles_test_anim_json_parser`).
- [x] Doxygen comment refs to "Porytiles2" in 5 source files (project-name references in `@details` blocks).

**A2c — Build-version macro defaults**
- [x] `Porytiles/include/porytiles/build_version.h`: `PORYTILES_EXECUTABLE_` already
  defaulted to `porytiles` pre-rename (which was wrong-for-old-state but correct-for-new). Namespace decl + closing comment swept by A2b pattern.
- [x] (Legacy `porytiles-legacy` default handled in A3.)

**A2d — Jinja2 templates + generator script**
- [x] 27 templates under `Porytiles/config_templates/*.jinja2` (plan said 29 — actual file count includes 2 non-`.jinja2` files): swept `porytiles2/` include paths and `namespace porytiles2`/`porytiles2::` patterns.
- [x] `config_schema.yaml`: 12 `header_path: porytiles2/...` entries → `porytiles/...`.
- [x] `Scripts/generate_config.py`: all `Porytiles2/` → `Porytiles/` and `porytiles2/` → `porytiles/` substitutions.
- [x] `uv run Scripts/generate_config.py` runs clean; idempotent (no diff on second invocation).
- [x] Reconciled GENERATED_CONFIG_FILES (CMake) vs `templates` list (script): added 5 missing entries that the script generates but CMake didn't track — `header_define_provider.{hpp,cpp}` (the flagged inconsistency) plus 3 test mocks (`mock_{domain,infra,app}_config.hpp` under `tests/support/`).

**A2 verification**
- [x] Pass 1 build (Porytiles2 -j7): exit 0. Pass 1 tests (`Porytiles2AllTests`): 1144/1144 pass.
- [x] Pass 2 build (post dir-rename, fresh `porytiles-build-debug`): exit 0 after one transient nlohmann/json FetchContent retry and one CMake re-run for GSL.natvis state settling.
- [x] Pass 2 tests (`PorytilesAllTests`): 1144/1144 pass in 1.544s.
- [x] Porytiles1 sanity (`Porytiles1Tests`): 73/73 cases / 2,689,245 assertions pass — confirms A2 didn't regress A1.
- [x] Binary: `./porytiles-build-debug/Porytiles/tools/driver/porytiles --version` reports `porytiles default_build_version 1970...` (executable name correct; real version comes in Phase C).
- [x] `cmake --install --prefix ~/.local`: writes `~/.local/bin/porytiles` (old `~/.local/bin/porytiles2` from prior install remains — see unresolved item #3, deprecation symlink decision).

### A3 — Porytiles1 → Legacy directory rename — after A1 + A2 stable

- [x] Rename `Porytiles1/` → `Legacy/` via `git mv`.
- [x] Root `CMakeLists.txt`: `add_subdirectory(Porytiles1)` → `add_subdirectory(Legacy)`.
- [x] `PORYTILES1_INCLUDE_DIR` → `PORYTILES_LEGACY_INCLUDE_DIR` (3 refs across 3 CMake files; swept via `s/PORYTILES1/PORYTILES_LEGACY/g`).
- [x] Rename targets: `Porytiles1Lib`→`LegacyLib`, `Porytiles1Driver`→`LegacyDriver`,
  `Porytiles1Tests`→`LegacyTests`, `Porytiles1LibTests`→`LegacyLibTests`,
  `Porytiles1TestSuite`→`LegacyTestSuite`, CTest `AllPorytiles1Tests`→`AllLegacyTests`, export filename `Porytiles1LibraryTargets.cmake`→`LegacyLibraryTargets.cmake`. Single `s/Porytiles1/Legacy/g` sweep handled all because every name has `Porytiles1` as substring. `Porytiles1/tools/CMakeLists.txt` contained no tokens (just `add_subdirectory(driver)`), no edit needed.
- [x] `Legacy/tools/driver/CMakeLists.txt`: `OUTPUT_NAME "porytiles"`→`"porytiles-legacy"`;
  added `install(TARGETS LegacyDriver RUNTIME DESTINATION bin)`.
- [x] `Legacy/lib/legacy/cli_parser.cpp`: hardcoded `porytiles` help/usage literals → `porytiles-legacy`. **Plan said 37; actual was 15 CLI literals + 7 URLs (22 total `\bporytiles\b` matches).** URLs preserved unchanged (`github.com/grunt-lucas/porytiles/wiki` etc. — the repo itself is still named `porytiles`). Used literal-sed `s|porytiles |porytiles-legacy |g` (trailing space cleanly partitions the two groups; `porytiles_legacy` underscore-tokens auto-protected). macOS BSD sed `-E` does NOT honor `\b` — first attempt was a no-op.
- [x] **Out-of-scope addition (originally A2c per plan, fixed here):** `Legacy/include/porytiles_legacy/build_version.h` `#define PORYTILES_EXECUTABLE_ porytiles` → `porytiles-legacy`. Required for `porytiles-legacy --version` to print its own name; pp-token stringification of `porytiles-legacy` works because C99 6.10.3.2/2 preserves token spelling and adjacent no-whitespace tokens concatenate (same mechanism CI uses for `1.0.0-snapshot.<ts>.<sha>`).
- [x] Build green: `cmake --build` exit 0 (yaml-cpp FetchContent populate race on first try → resolved by retry, like A2's GSL.natvis flake).
- [x] Tests green: PorytilesAllTests 1144/1144 (unchanged from A2 — Porytiles/ untouched by A3); LegacyTests 73 cases / 2,689,245 assertions.
- [x] Install green: `~/.local/bin/porytiles` and `~/.local/bin/porytiles-legacy` both installed; `--version` outputs correct executable names (`porytiles ...` and `porytiles-legacy ...`). Pre-A2 `~/.local/bin/porytiles2` still present — unresolved item #3.

### A4 — Scripts, configs, IDE files, docs sweep

Discovery grep across the A4 surface returned 34 files with hits, of which 28 were
actionable (4 gitignored chatter: `.idea/QuickJinja_project.xml`, `.idea/workspace.xml`,
`.claude/settings.local.json`; 2 deferred: `release_1_0_0_prep_plan.md` per user
decision, `GEMINI.md` was a 13-byte `@./CLAUDE.md` import stub with no hits).

- [x] **A4a — Scripts (5 files, 33 hits).** `format.py` (7), `new_class.py` (10),
  `coverage.py` (4), `tidy.py` (7), `todo.py` (5). All swept via `Porytiles2`→`Porytiles`
  + `porytiles2`→`porytiles`. `generate_config.py` confirmed clean from A2d (re-run
  produced zero diff to generated headers). 3 scripts had no lowercase `porytiles2`
  refs (they only used path-style `Porytiles2`).
- [x] **A4b — Configs.** `.clang-tidy` `HeaderFilterRegex: 'porytiles2/.*'`
  → `'porytiles/.*'` (1 line); `pyproject.toml` description string (1 line).
- [x] **A4c — IDE files (2 files, plan said 3).** Only `.vscode/c_cpp_properties.json`
  (Porytiles1→Legacy + Porytiles2→Porytiles) and `.vscode/launch.json` (Porytiles2,
  porytiles2 binary + arg) needed sweeps. **Plan listed tracked `.idea/*` as scope;
  actual: only `.idea/cmake.xml` is tracked under `.idea/`, and it had ZERO
  Porytiles1/2 hits (its CMake profile names use bare `Porytiles*` — already correct
  post-A2/A3).** Side note: `launch.json` line 9 LLDB arg `porytiles2_test_simple`
  was mechanically renamed to `porytiles_test_simple`; no such tileset actually exists
  in the testbed (the convention is `porytiles_test1` etc.), so this is a stale
  placeholder either way.
- [x] **A4d — Top-level docs (3 files, plan said 4).** `STYLE.md` mechanical sweep
  (9 hits, all code-fenced namespace/include examples). `README.md` mechanical
  `Porytiles1`→`Legacy` (3 hits) + one targeted edit for legacy binary path
  (`./build/Legacy/tools/driver/porytiles` → `porytiles-legacy`). `CLAUDE.md` (24 hits)
  hand-edited 3 prose sections (architecture overview lines 11-15 now reference
  directory names not version suffixes; "### Legacy Porytiles1 Wiki" → "### Legacy
  Wiki" with surrounding prose; behavioral rule `Porytiles1/`→`Legacy/`), then
  mechanical sweep for the rest. **`GEMINI.md` had zero hits** — it's a 13-byte
  `@./CLAUDE.md` import directive; will pick up updates automatically.
- [x] **A4e — Claude-Code docs (5 files).** `.claude/agents/{architect,build-expert,
  code-reviewer,debugger}.md` + `.claude/skills/fix-includes.md`. `debugger.md`
  needed special handling for legacy-CLI line (`./porytiles-build-debug/Porytiles1/
  tools/driver/porytiles` → `./porytiles-build-debug/Legacy/tools/driver/porytiles-legacy`);
  the rest mechanical. **`.claude/settings.local.json` is gitignored user state, skipped.**
- [x] **A4f — Sub-project docs (12 files, plan said 9 Notes).** `Porytiles/ARCHITECTURE.md`
  (6 hits), `Porytiles/README.md` (3 hits) and `Legacy/README.md` (1 hit + legacy binary
  edit) mechanical. Notes/*.md sweep across 9 files (`c_parser_ast_analysis`,
  `diagnostics_formatting_cookbook`, `fmtlib_usage_analysis`, `fruit_migration_plan`,
  `project_structure_refactoring_plan`, `secondary_animation_handling`,
  `secondary_compilation_plan`, `std_formatter_guide`, `tile_sharing_indirect_system`)
  via `sed -i ''` with the two patterns. `secondary_animation_handling.md` line 197
  additionally had `porytiles1` (lowercase prose) → `porytiles-legacy`. 3 Notes files
  (`cpp_tile_mask.md`, `diagnostic_notes_review.md`, `service_vs_free_function_architecture.md`)
  had zero hits. **`release_1_0_0_prep_plan.md` (74 hits) deferred as historical
  artifact per user decision** — its prose describes the rename as-it-was-planned,
  so rewriting paths would create self-contradictions ("A1 renames Porytiles2 to
  Porytiles" doesn't make sense if both names are "Porytiles"). Future readers
  understand it as a planning doc frozen at pre-rename.
- [x] **Verification.** Final wide-grep clean except the 4 expected residual files
  (3 gitignored + 1 deferred plan). All 6 Python scripts run green with renamed
  paths (`--help` output reflects updated descriptions; `generate_config.py`
  produces zero diff). Edit-tool quirk: a few earlier `replace_all` calls on
  `Porytiles/ARCHITECTURE.md` and the Notes files reported success but didn't
  modify files (Edit tracks Read-state inconsistently when files were only seen
  via `grep` output); worked around with `sed -i ''`.

**Note on architectural framing in `Legacy/README.md` and `Porytiles/README.md`:**
both still describe the old "MV1 is the current Porytiles offering" / "MV2 is the
next-generation" framing, which is factually wrong post-1.0.0 but outside A4 scope
(mechanical sweep only). Defer to Phase E6 README rewrite for the release.

### A5 — GitHub Actions hardcoded paths

- [x] **Discovery delta** — planning doc listed 10 files needing edits; wide grep
  finds **only 5** with actual hits (all composite `action.yml` files). The wrapper
  workflows (`build_pages.yml`, `build_jobs_template.yml`, `dev_build.yml`,
  `pr_dev_build.yml`, `nightly_release.yml`) are thin dispatchers that
  `uses:` the composite actions and contain zero hardcoded Porytiles1/2
  paths or target names — they need no edits. This is good CI hygiene we
  benefit from. Total hits: 26 (4+4+4+9+5).
- [x] `build_linux_clang/action.yml` (4 hits) — `file`/`objdump` lines on
  driver binaries: legacy → `Legacy/tools/driver/porytiles-legacy`, active →
  `Porytiles/tools/driver/porytiles`.
- [x] `build_linux_gcc/action.yml` (4 hits) — same pattern.
- [x] `build_macos_clang/action.yml` (4 hits) — same pattern.
- [x] `create_release_package/action.yml` (9 hits) — both driver `cp`s; three
  test-binary `cp`s (`Porytiles1Tests` → `LegacyTests`, `Porytiles2UnitTests` →
  `PorytilesUnitTests`, `Porytiles2IntegrationTests` → `PorytilesIntegrationTests`);
  `Porytiles1/vendor` → `Legacy/vendor`; three executable invocations updated
  to match.
- [x] `run_test_suite/action.yml` (5 hits) — both `--version` invocations + all
  three test-binary executions.
- [x] **Cross-reference vs. post-A3 filesystem**: confirmed
  `Legacy/tools/driver/CMakeLists.txt` sets `OUTPUT_NAME "porytiles-legacy"`,
  `Porytiles/tools/driver/CMakeLists.txt` sets `OUTPUT_NAME "porytiles"`,
  `Porytiles/tests/CMakeLists.txt` produces `PorytilesUnitTests` +
  `PorytilesIntegrationTests` + `PorytilesAllTests`, `Legacy/tests/CMakeLists.txt`
  produces a single `LegacyTests` binary, and `Legacy/vendor/` exists.
- [x] **Verification**: re-grep across `.github/` for any residual
  `Porytiles[12]|porytiles[12]|PORYTILES[12]` returned zero hits. All 14 yaml
  files under `.github/workflows/` parse via `yaml.safe_load`. Visual diff is
  clean: 5 files, 26 insertions / 26 deletions, every change is a 1-for-1
  substitution with no incidental edits.

**Note**: `nightly_release.yml` was on the planning doc's A5 list but already
contains zero Porytiles1/2 path refs — its release artifact name is
`porytiles-linux-amd64.zip` (no version suffix in the binary name) and it
delegates all build/test work to the composite actions we edited. The file's
trigger semantics + the `nightly_release.yml` → `snapshot_release.yml` file
rename remain Phase D1's job and are NOT in A5 scope.

**Note**: We cannot actually exercise the workflows without pushing the branch
(the Phase-A-as-one-PR strategy forbids that until A6 closes). A6 will validate
the changes by extension when it runs the full local build matrix and confirms
the paths/targets these workflows reference still resolve.

### A6 — Phase A verification

- [x] `rm -rf porytiles-build-debug/` then `cmake -B porytiles-build-debug -S .`.
- [x] `cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1` (check exit code).
- [x] `./porytiles-build-debug/Porytiles/tests/PorytilesAllTests > /tmp/test.log 2>&1` (check exit code).
- [x] `./porytiles-build-debug/Legacy/tests/LegacyTests > /tmp/legacy_test.log 2>&1` (check exit code).
- [x] `cmake --install porytiles-build-debug --prefix ~/.local`.
- [x] `~/.local/bin/porytiles --version` and `~/.local/bin/porytiles-legacy --version`.
- [x] `uv run Scripts/generate_config.py` produces no diff.

**Verification results (local, 2026-05-31)**

Configure required a retry after the documented FetchContent race (json clone hit `fatal: could not open ... tmp_pack_*: No such file or directory` on first run, then fmt and cpptrace `cmake/InstallRules.cmake` each tripped on subsequent retries — a two-layered race between git clone and macOS Spotlight indexing of the newly-extracted `.git/objects/` dirs). The reliable cure was *not* to nuke and retry but to leave the build dir alone and re-invoke `cmake -B`: CMake's FetchContent skips already-populated deps via stamp files, so the second configure runs against a quiesced disk state. Also worth noting: `rm -rf porytiles-build-debug/` consistently emitted spurious "Directory not empty" warnings on macOS even though the directory was actually removed (mdworker holding phantom file handles during recursive descent). Build then completed in one pass at -j7 (5891 lines of build output, exit 0).

Test counts:
- **`PorytilesAllTests`**: 1144/1144 passed across 99 test suites (1765 ms). GoogleTest.
- **`LegacyTests`**: 73/73 cases passed, 2,689,245/2,689,245 assertions passed. doctest.

Binaries installed: `~/.local/bin/porytiles` (25 MB) and `~/.local/bin/porytiles-legacy` (5.6 MB). `--version` outputs `porytiles default_build_version 1970.01.01T00:00:00+00:00` and `porytiles-legacy default_build_version 1970.01.01T00:00:00+00:00` respectively — exec-name tokens are correct (the `default_build_version` and 1970-epoch placeholders are the expected pre-Phase-C static fallbacks defined in `build_version.h` when CI doesn't override `-DPORYTILES_BUILD_VERSION_=...`; Phase C wires `${CMAKE_PROJECT_VERSION}` automatically). `generate_config.py` rerun produced zero generated-file diff, confirming the A2d template + generator sweep is self-consistent.

End-to-end through `cmake --build` + install + execute is the first validation that A1+A2+A3+A4+A5 compose correctly — no individual sub-phase could test the rename composition in isolation. Phase A is closed locally; ready for the bundled PR to develop.

---

## Phase B — CHANGELOG infrastructure (parallel with A)

- [x] **B1** Created `CHANGELOG.md` at repo root: empty `## [Unreleased]` above a catch-all `## [1.0.0] - YYYY-MM-DD` release note. Release-prep work itself is not enumerated as changelog entries per user direction — accumulation begins post-1.0.0 cut.
- [x] **B2** Added `.github/workflows/changelog_check.yml`: triggers on `pull_request` to `develop` (not `master` — release cuts always touch CHANGELOG by construction); step-level `if:` skips the gate when the PR carries `no-changelog`, leaving job status as success for required-check compatibility; diff check uses `git diff --name-only origin/${{ github.base_ref }}...HEAD | grep -q '^CHANGELOG\.md$'`. yaml.safe_load passes.
- [x] **B3** Added "Changelog" section to `CONTRIBUTING.md` (after "Branch Cleanup", with matching TOC entry) documenting the rule, the `no-changelog` opt-out, and the release-cut migration of `[Unreleased]` to a dated heading. Uses sembr to match the rest of the file.

**Phase B prerequisites for the gate to actually function** (carried forward — do NOT skip before pushing the PR):
- [x] Created the `no-changelog` label (`#cfd3d7`, "PR is exempt from CHANGELOG.md enforcement") via `gh label create`. Now selectable by contributors on PRs.
- [ ] The recursive-bootstrap test: B's own PR will be the first run of `Porytiles Changelog Check` — confirm it passes (because the PR touches `CHANGELOG.md` by creating it).

---

## Phase C — Versioning system (depends on A2)

- [ ] **C1** Root `CMakeLists.txt`: `project(Porytiles CXX)` → `project(Porytiles VERSION 1.0.0 CXX)`;
  confirm sub-CMakeLists read `${CMAKE_PROJECT_VERSION}` (validate with a `message(STATUS ...)`).
- [ ] **C2** `Porytiles/lib/CMakeLists.txt` + `Legacy/lib/CMakeLists.txt`: add
  `target_compile_definitions(... PRIVATE PORYTILES_BUILD_VERSION_=${CMAKE_PROJECT_VERSION})`.
- [ ] **C3** Uniform `<exec-name> <version> <date>` across both binaries
  (`Porytiles/tools/driver/main.cpp` + `Legacy/lib/legacy/cli_parser.cpp`).
- [ ] **C4** Verify: `porytiles --version` → `porytiles 1.0.0 <date>`;
  `porytiles-legacy --version` → `porytiles-legacy 1.0.0 <date>`; override flag echoes.

---

## Phase D — CI / release pipeline overhaul (depends on A, C)

- [ ] **D1** `nightly_release.yml` → `snapshot_release.yml`: trigger `push: [develop]`;
  concurrency group `snapshot-release` (no cancel); drop "already built" gate;
  pre-delete prior snapshot release+tag; build-version flag with timestamp+sha;
  bundle both binaries; `softprops/action-gh-release` `tag_name: snapshot` prerelease;
  update `porytiles@snapshot.rb` in tap.
- [ ] **D2** New `versioned_release.yml`: trigger `push: tags: ['v[0-9]+.[0-9]+.[0-9]+']`;
  concurrency `versioned-release-${{ github.ref }}`; build-version from tag; four-platform
  matrix; bundle both binaries; non-prerelease; update `porytiles.rb`; never delete prior releases.
- [ ] **D3** CHANGELOG-extraction helper shared by D1/D2.
- [ ] **D4** Audit overlaps: recommend deleting `dev_build.yml` (snapshot becomes the
  single develop-push pipeline); keep `pr_dev_build.yml`; decide `build_pages.yml` scope.
- [ ] **D5** Verify on a real develop push + a throwaway `v0.0.0-test` tag.

---

## Phase E — Gitflow adoption + 1.0.0 cut (last; A–D stable on develop)

- [ ] **E1** Audit open feature branches (see Open blockers).
- [ ] **E2** Cut `release/1.0.0` from `develop`; confirm `project(... VERSION 1.0.0)`;
  migrate `[Unreleased]` → `[1.0.0] - <date>`; **resolve `STABILITY.md` blocker**; smoke test.
- [ ] **E3** Tag + merge in lockstep across main + both docs repos: create `master` from
  `release/1.0.0`; tag `v1.0.0` (verify `versioned_release.yml` fires before tagging docs
  repos); merge release back to `develop`; delete release branch. Verify docs sites.
- [ ] **E4** Branch + tag protection (`master`, `develop`, `v[0-9]+.[0-9]+.[0-9]+`).
- [ ] **E5** Document gitflow conventions in `CONTRIBUTING.md`.
- [ ] **E6** README install section; confirm default branch `develop`; housekeeping; announce.

---

## Phase F — Documentation repos gitflow alignment (parallel with A–D; tag at E3)

> Commit all Phase F changes in `porytiles-user-docs/` and `porytiles-dev-docs/`,
> NOT the main repo.

- [ ] **F1** Adopt gitflow branches in each docs repo (create/establish `develop` default;
  rename `main`→`master` recommended; `master` created at first cut).
- [ ] **F2** Parameterize Sphinx version display in `docsrc/conf.py` (Option A: tracked
  `VERSION` file recommended).
- [ ] **F3** GH Pages deploy from `master` only — both Sphinx repos AND main-repo Doxygen
  (`build_pages.yml` trigger → `master`); verify post-rename Doxygen paths.
- [ ] **F4** Local-only viewing instructions in each docs `README.md`.
- [ ] **F5** Docs/code coordination convention in main `CONTRIBUTING.md`.
- [ ] **F6** Branch protection on docs repos.
- [ ] **F7** Initial sync: both docs `develop` reflect post-rename names/CLI/install.
- [ ] **F8** Verify per docs repo (build on develop+master; deploy on master only;
  checkout-tag-build works; version string renders).

---

## Phase G — AI policy documentation (parallel with A–F; land early)

- [x] **G1** Drafted `AI-POLICY.md` at repo root. Two-track stance: `Legacy/` is closed to AI
  contributions of any kind (hard-line, no exceptions); `Porytiles/` accepts AI-assisted work at
  maintainer discretion. Slop description stayed abstract per maintainer choice (defers to STYLE.md
  and reviewer judgment, no enumeration of specific tells). No disclosure checkbox, no detector.
  Revisit-cadence section omitted entirely. Pointers to `STYLE.md` and `CONTRIBUTING.md` included.
  ~25 lines of prose, one screen. Slop self-audit pass complete (no em-dashes, no marketing prose,
  no platitude bullets).
- [ ] **G2** Reference from `CONTRIBUTING.md` (and optionally `README.md`). **Deferred to a later
  step** per maintainer direction; `CONTRIBUTING.md`/`README.md` overhaul folded into a later
  phase rather than landed alongside G1.
- [ ] **G3** Verify (deferred half): `CONTRIBUTING.md`/`README.md` references resolve. The G1 half
  (file exists at repo root, reads as not-itself-slop) is satisfied.

---

## End-to-end verification (after all phases)

- [ ] Clean build produces `porytiles` + `porytiles-legacy`; `--version` reports `1.0.0` + date.
- [ ] `cmake --install` writes both binaries to `<prefix>/bin/`.
- [ ] Develop push → exactly one `snapshot` release; zips contain both binaries; brew snapshot re-pulls.
- [ ] `v1.0.1` tag → versioned release; `v1.0.0` untouched; brew stable re-pulls.
- [ ] `brew install grunt-lucas/porytiles/porytiles` and `...porytiles@snapshot` both work, both binaries.
- [ ] `CHANGELOG.md` has a date-stamped 1.0.0 section.
- [ ] Branch protection rejects direct push to `master`.
- [ ] CHANGELOG workflow blocks a PR missing a CHANGELOG entry (unless `no-changelog`).
- [ ] `AI-POLICY.md` exists; `CONTRIBUTING.md` links it; legacy AI-free stance unambiguous.
- [ ] `STABILITY.md` (or equivalent) classifies every public surface.
