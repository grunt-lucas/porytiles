# Porytiles 1.0.0 Release Prep — Working Tracker

This is the **operational** tracking document for the Porytiles 1.0.0 release. It
distills the locked-in decisions, the phase outline, and a tickable checklist so a
session can be directed with "execute Phase A2 from `RELEASE_PREP_1_0_0.md`."

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
| C  | Versioning system | ✅ Done |
| D  | CI / release pipeline overhaul | ✅ Done |
| E  | Gitflow adoption + 1.0.0 cut | 🟡 In progress — E4 pending; docs lockstep deferred ([#313](https://github.com/grunt-lucas/porytiles/issues/313)) |
| F  | Documentation repos gitflow alignment | 🟡 In progress — F6/F8/F3-docs deferred to docs release ([#313](https://github.com/grunt-lucas/porytiles/issues/313)) |
| G  | AI policy documentation | ✅ Done |
| H  | Top-level lowercase migration | ✅ Done |

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
- **H** depends on **A** complete (already met); should land BEFORE **D** so the CI
  workflow rewrites reference final lowercase paths. Independent of **B**, **C**,
  **F**, **G**.

---

## Open blockers (must be resolved before tagging `v1.0.0`)

- [x] **1.0.0 public API surface / `STABILITY.md`** — *Resolved by absorption*.
  The classification matrix was deemed overkill for a CLI tool of Porytiles's
  size and audience (comparable indie/decomp tools — jq, ripgrep, Porymap,
  Poryscript — don't ship such docs and version fine without one). The
  version-bump philosophy now lives in `RELEASE_PROCESS.md`'s
  "Choosing the version number" subsection. CHANGELOG plus maintainer judgment
  is the per-release authority. (Phase E2.)
- [x] **Open feature branches audit** — *Resolved by archival deletion*.
  All three (`bug/anim-tiles`, `bug/issue-0060/key-frame-bug`,
  `feature/issue-0047/compiled-paired-primary`) were 1.5+ year old single-commit
  investigations against the legacy `src/` codebase with their tracking GitHub
  issues already closed. Archive-tagged as `archived/<branch>` (commits preserved)
  and deleted from `origin`. (Phase E1.)

---

## Locked-in decisions

**Renames**
- `Porytiles2/` → `porytiles/`; namespace `porytiles2` → `porytiles`; include prefix
  `porytiles2/` → `porytiles/`; executable `porytiles2` → `porytiles`.
- `Porytiles1/` → `legacy/`; namespace `porytiles1` → `porytiles_legacy`; include
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
preserved permanently; same platform matrix as snapshot; both binaries bundled.

**Platform matrix (both release pipelines)** — three platforms: `linux-amd64`,
`linux-arm64`, `macos-arm64`. **`macos-amd64` (Intel) is deliberately omitted** because
the active `porytiles/` codebase has C++23 deps that don't build on `macos-13`'s Apple
Clang/libc++ snapshot (see commit `745b0152`, Oct 2025). Reintroduce only after a
toolchain fix lands — either install homebrew LLVM clang and absorb the libc++ ABI work,
or migrate macos-amd64 to a GCC-based build with static-linked libstdc++.

**Homebrew** (`grunt-lucas/homebrew-porytiles` tap) — two formulas. `porytiles.rb`
tracks latest `v*` tag (versioned-release workflow). `porytiles-snapshot.rb` tracks
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
- [x] Rename `Porytiles2/` → `porytiles/` (via `git mv`; git rename-detection clean).
- [x] Root `CMakeLists.txt`: `add_subdirectory(Porytiles2)` → `add_subdirectory(Porytiles)`.
- [x] `PORYTILES2_INCLUDE_DIR` → `PORYTILES_INCLUDE_DIR` everywhere.
- [x] CMake targets renamed (`Porytiles2Lib`→`PorytilesLib`, Driver, three Tests).
- [x] `porytiles/lib/CMakeLists.txt`: `project(Porytiles2Lib CXX)`→`PorytilesLib`;
  `CANONICAL_LIB_NAME "porytiles2"`→`"porytiles"`; `export()` file renamed.
- [x] `porytiles/tools/driver/CMakeLists.txt`: project rename; `OUTPUT_NAME "porytiles2"`→`"porytiles"`.
- [x] `porytiles/tests/CMakeLists.txt`: all three test-target names.
- [x] `porytiles/CMakeLists.txt`: all generated-file paths + `config_templates/` references.
- [x] `docs/CMakeLists.txt`: `Porytiles2Lib`, `PORYTILES2_PUBLIC_HEADER*`, three source paths.

**A2b — Include directory + namespace sweep**
- [x] Rename `porytiles/include/porytiles2/` → `porytiles/include/porytiles/` (via `git mv`).
- [x] `#include "porytiles2/` → `#include "porytiles/` (1,830 occurrences pre-sweep — actual count, plan said ~1,670).
- [x] `namespace porytiles2` → `namespace porytiles` (799 pre-sweep — plan said ~722). Pattern catches namespace decls, `using namespace`, and closing comments in one shot.
- [x] `porytiles2::` → `porytiles::` qualified refs (135 pre-sweep).
- [x] `command_completion.hpp` shell completion literals (≈37 occurrences): bash/zsh/fish function names like `_porytiles2_completions` and CLI strings like `complete -c porytiles2` — these are not include/namespace patterns but represent the executable name, so they swept in pass 2 along with `OUTPUT_NAME`. Plan flagged this as an unknown; turns out completion is hardcoded (not `argv[0]`-derived).
- [x] One tmpdir literal in `anim_json_parser_override_test.cpp` (`porytiles2_test_anim_json_parser` → `porytiles_test_anim_json_parser`).
- [x] Doxygen comment refs to "Porytiles2" in 5 source files (project-name references in `@details` blocks).

**A2c — Build-version macro defaults**
- [x] `porytiles/include/porytiles/build_version.h`: `PORYTILES_EXECUTABLE_` already
  defaulted to `porytiles` pre-rename (which was wrong-for-old-state but correct-for-new). Namespace decl + closing comment swept by A2b pattern.
- [x] (Legacy `porytiles-legacy` default handled in A3.)

**A2d — Jinja2 templates + generator script**
- [x] 27 templates under `porytiles/config_templates/*.jinja2` (plan said 29 — actual file count includes 2 non-`.jinja2` files): swept `porytiles2/` include paths and `namespace porytiles2`/`porytiles2::` patterns.
- [x] `config_schema.yaml`: 12 `header_path: porytiles2/...` entries → `porytiles/...`.
- [x] `scripts/generate_config.py`: all `Porytiles2/` → `porytiles/` and `porytiles2/` → `porytiles/` substitutions.
- [x] `uv run scripts/generate_config.py` runs clean; idempotent (no diff on second invocation).
- [x] Reconciled GENERATED_CONFIG_FILES (CMake) vs `templates` list (script): added 5 missing entries that the script generates but CMake didn't track — `header_define_provider.{hpp,cpp}` (the flagged inconsistency) plus 3 test mocks (`mock_{domain,infra,app}_config.hpp` under `tests/support/`).

**A2 verification**
- [x] Pass 1 build (Porytiles2 -j7): exit 0. Pass 1 tests (`Porytiles2AllTests`): 1144/1144 pass.
- [x] Pass 2 build (post dir-rename, fresh `porytiles-build-debug`): exit 0 after one transient nlohmann/json FetchContent retry and one CMake re-run for GSL.natvis state settling.
- [x] Pass 2 tests (`PorytilesAllTests`): 1144/1144 pass in 1.544s.
- [x] Porytiles1 sanity (`Porytiles1Tests`): 73/73 cases / 2,689,245 assertions pass — confirms A2 didn't regress A1.
- [x] Binary: `./porytiles-build-debug/porytiles/tools/driver/porytiles --version` reports `porytiles default_build_version 1970...` (executable name correct; real version comes in Phase C).
- [x] `cmake --install --prefix ~/.local`: writes `~/.local/bin/porytiles` (old `~/.local/bin/porytiles2` from prior install remains — see unresolved item #3, deprecation symlink decision).

### A3 — Porytiles1 → Legacy directory rename — after A1 + A2 stable

- [x] Rename `Porytiles1/` → `legacy/` via `git mv`.
- [x] Root `CMakeLists.txt`: `add_subdirectory(Porytiles1)` → `add_subdirectory(Legacy)`.
- [x] `PORYTILES1_INCLUDE_DIR` → `PORYTILES_LEGACY_INCLUDE_DIR` (3 refs across 3 CMake files; swept via `s/PORYTILES1/PORYTILES_LEGACY/g`).
- [x] Rename targets: `Porytiles1Lib`→`LegacyLib`, `Porytiles1Driver`→`LegacyDriver`,
  `Porytiles1Tests`→`LegacyTests`, `Porytiles1LibTests`→`LegacyLibTests`,
  `Porytiles1TestSuite`→`LegacyTestSuite`, CTest `AllPorytiles1Tests`→`AllLegacyTests`, export filename `Porytiles1LibraryTargets.cmake`→`LegacyLibraryTargets.cmake`. Single `s/Porytiles1/legacy/g` sweep handled all because every name has `Porytiles1` as substring. `Porytiles1/tools/CMakeLists.txt` contained no tokens (just `add_subdirectory(driver)`), no edit needed.
- [x] `legacy/tools/driver/CMakeLists.txt`: `OUTPUT_NAME "porytiles"`→`"porytiles-legacy"`;
  added `install(TARGETS LegacyDriver RUNTIME DESTINATION bin)`.
- [x] `legacy/lib/legacy/cli_parser.cpp`: hardcoded `porytiles` help/usage literals → `porytiles-legacy`. **Plan said 37; actual was 15 CLI literals + 7 URLs (22 total `\bporytiles\b` matches).** URLs preserved unchanged (`github.com/grunt-lucas/porytiles/wiki` etc. — the repo itself is still named `porytiles`). Used literal-sed `s|porytiles |porytiles-legacy |g` (trailing space cleanly partitions the two groups; `porytiles_legacy` underscore-tokens auto-protected). macOS BSD sed `-E` does NOT honor `\b` — first attempt was a no-op.
- [x] **Out-of-scope addition (originally A2c per plan, fixed here):** `legacy/include/porytiles_legacy/build_version.h` `#define PORYTILES_EXECUTABLE_ porytiles` → `porytiles-legacy`. Required for `porytiles-legacy --version` to print its own name; pp-token stringification of `porytiles-legacy` works because C99 6.10.3.2/2 preserves token spelling and adjacent no-whitespace tokens concatenate (same mechanism CI uses for `1.0.0-snapshot.<ts>.<sha>`).
- [x] Build green: `cmake --build` exit 0 (yaml-cpp FetchContent populate race on first try → resolved by retry, like A2's GSL.natvis flake).
- [x] Tests green: PorytilesAllTests 1144/1144 (unchanged from A2 — porytiles/ untouched by A3); LegacyTests 73 cases / 2,689,245 assertions.
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
  (`./build/legacy/tools/driver/porytiles` → `porytiles-legacy`). `CLAUDE.md` (24 hits)
  hand-edited 3 prose sections (architecture overview lines 11-15 now reference
  directory names not version suffixes; "### Legacy Porytiles1 Wiki" → "### Legacy
  Wiki" with surrounding prose; behavioral rule `Porytiles1/`→`legacy/`), then
  mechanical sweep for the rest. **`GEMINI.md` had zero hits** — it's a 13-byte
  `@./CLAUDE.md` import directive; will pick up updates automatically.
- [x] **A4e — Claude-Code docs (5 files).** `.claude/agents/{architect,build-expert,
  code-reviewer,debugger}.md` + `.claude/skills/fix-includes.md`. `debugger.md`
  needed special handling for legacy-CLI line (`./porytiles-build-debug/Porytiles1/
  tools/driver/porytiles` → `./porytiles-build-debug/legacy/tools/driver/porytiles-legacy`);
  the rest mechanical. **`.claude/settings.local.json` is gitignored user state, skipped.**
- [x] **A4f — Sub-project docs (12 files, plan said 9 Notes).** `porytiles/ARCHITECTURE.md`
  (6 hits), `porytiles/README.md` (3 hits) and `legacy/README.md` (1 hit + legacy binary
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
  `porytiles/ARCHITECTURE.md` and the Notes files reported success but didn't
  modify files (Edit tracks Read-state inconsistently when files were only seen
  via `grep` output); worked around with `sed -i ''`.

**Note on architectural framing in `legacy/README.md` and `porytiles/README.md`:**
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
  driver binaries: legacy → `legacy/tools/driver/porytiles-legacy`, active →
  `porytiles/tools/driver/porytiles`.
- [x] `build_linux_gcc/action.yml` (4 hits) — same pattern.
- [x] `build_macos_clang/action.yml` (4 hits) — same pattern.
- [x] `create_release_package/action.yml` (9 hits) — both driver `cp`s; three
  test-binary `cp`s (`Porytiles1Tests` → `LegacyTests`, `Porytiles2UnitTests` →
  `PorytilesUnitTests`, `Porytiles2IntegrationTests` → `PorytilesIntegrationTests`);
  `Porytiles1/vendor` → `legacy/vendor`; three executable invocations updated
  to match.
- [x] `run_test_suite/action.yml` (5 hits) — both `--version` invocations + all
  three test-binary executions.
- [x] **Cross-reference vs. post-A3 filesystem**: confirmed
  `legacy/tools/driver/CMakeLists.txt` sets `OUTPUT_NAME "porytiles-legacy"`,
  `porytiles/tools/driver/CMakeLists.txt` sets `OUTPUT_NAME "porytiles"`,
  `porytiles/tests/CMakeLists.txt` produces `PorytilesUnitTests` +
  `PorytilesIntegrationTests` + `PorytilesAllTests`, `legacy/tests/CMakeLists.txt`
  produces a single `LegacyTests` binary, and `legacy/vendor/` exists.
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
- [x] `./porytiles-build-debug/porytiles/tests/PorytilesAllTests > /tmp/test.log 2>&1` (check exit code).
- [x] `./porytiles-build-debug/legacy/tests/LegacyTests > /tmp/legacy_test.log 2>&1` (check exit code).
- [x] `cmake --install porytiles-build-debug --prefix ~/.local`.
- [x] `~/.local/bin/porytiles --version` and `~/.local/bin/porytiles-legacy --version`.
- [x] `uv run scripts/generate_config.py` produces no diff.

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

- [x] **C1** Root `CMakeLists.txt`: `project(Porytiles CXX)` → `project(Porytiles VERSION 1.0.0 LANGUAGES CXX)`;
  confirmed sub-CMakeLists see `CMAKE_PROJECT_VERSION=1.0.0` (validated with a temporary
  `message(STATUS ...)` then removed). **Plan said `project(Porytiles VERSION 1.0.0 CXX)`; actual
  is `project(Porytiles VERSION 1.0.0 LANGUAGES CXX)`** — the moment `VERSION` is present,
  CMake's `project()` parser requires the explicit `LANGUAGES` keyword instead of the bare
  positional language list, otherwise it errors with `must use LANGUAGES before language names`.
  Easy one-keyword fix; not flagged by the plan landmines.
- [x] **C2** Compile-def wiring landed in **three** targets, not two, plus a top-level resolver
  block. **Plan said add the def to `PorytilesLib` + `LegacyLib`; actual added it to
  `PorytilesLib`, `LegacyLib`, AND `PorytilesDriver`.** The asymmetric extra is forced by where
  the macro is consumed: legacy's `--version` handler lives in `cli_parser.cpp` (a TU of
  `LegacyLib`), so `target_compile_definitions(... PRIVATE)` on the lib is sufficient; active's
  `--version` handler lives in `porytiles/tools/driver/main.cpp` (a TU of `PorytilesDriver`, not
  `PorytilesLib`), so a `PRIVATE` def on the lib does NOT reach it. Caught by the first install
  verification: legacy reported `1.0.0` but active reported `default_build_version`. Adding a
  matching `target_compile_definitions(PorytilesDriver PRIVATE ...)` line resolved it. The C2
  spec missed this because the driver wasn't named in the planning checklist — worth flagging
  for future code: header consumers in `tools/` need parallel CMake wiring.

  **Refactor over the literal plan wording:** rather than three sites of `=${CMAKE_PROJECT_VERSION}`
  (DRY violation that also broke the CMake-level override channel — see below), lifted the
  default resolution into the root `CMakeLists.txt`:
  ```cmake
  if(NOT DEFINED PORYTILES_BUILD_VERSION_)
      set(PORYTILES_BUILD_VERSION_ ${CMAKE_PROJECT_VERSION})
  endif()
  ```
  The three target sites then read `${PORYTILES_BUILD_VERSION_}` as a one-line interpolation.
  Single source of default truth, single override channel.
- [x] **C3** No-op. Both `--version` handlers already produced `<EXEC> <VERSION> <DATE>` —
  active driver via `std::cout << ... << " " << ...` (line 64 of
  `porytiles/tools/driver/main.cpp`), legacy via `fmt::println("{} {} {}", ...)` (line 675 of
  `legacy/lib/legacy/cli_parser.cpp`). The plan was conservative ("reconcile any divergence"),
  the actual shapes already matched, no edit needed.
- [x] **C4** Default path verified: `~/.local/bin/porytiles --version` → `porytiles 1.0.0
  1970.01.01T00:00:00+00:00`; `~/.local/bin/porytiles-legacy --version` →
  `porytiles-legacy 1.0.0 1970.01.01T00:00:00+00:00`. Date placeholder stays at the
  `build_version.h` 1970-epoch default (Phase C only touches the version macro, not the date
  macro). Override path verified via the cleaner CMake cache-var channel:
  `cmake -DPORYTILES_BUILD_VERSION_=1.0.0-snapshot.20260601000000.abc12345`, rebuild + reinstall,
  both binaries echo the snapshot string exactly. Tests stayed green at every step:
  `PorytilesAllTests` 1144/1144, `LegacyTests` 73 cases / 2,689,245 assertions.

  **Override-channel discovery worth recording for Phase D:** CI today (per
  `.github/workflows/build_{linux_clang,linux_gcc,macos_clang}/action.yml`) passes the version
  override via `-DCMAKE_CXX_FLAGS="-DPORYTILES_BUILD_VERSION_=..."` — i.e. as a literal compiler
  flag, NOT as a CMake cache variable. After Phase C, that channel STILL WORKS (compiler honors
  "last `-D` wins" for any macro defined twice), but with cosmetic noise: builds emit **109
  `-Wmacro-redefined` warnings** (one per TU that includes `build_version.h`) because both
  Phase C's `target_compile_definitions` AND CI's `CMAKE_CXX_FLAGS` end up on the command line
  for the same macro. Final binaries echo correctly, but the warning stream is loud. **`-Werror`
  is currently disabled** (see the `fast-cpp-csv-parser` `strncpy` comment in the root
  `CMakeLists.txt`) so the warnings don't break the build. Phase D should migrate CI to the
  cache-var channel (`cmake -DPORYTILES_BUILD_VERSION_=... -DPORYTILES_BUILD_DATE_=...` instead
  of `-DCMAKE_CXX_FLAGS="..."`); my Phase C resolver block then short-circuits the default and
  the double-define disappears.

  **→ Resolved in Phase D.** The three composite actions (`build_linux_clang/action.yml`,
  `build_linux_gcc/action.yml`, `build_macos_clang/action.yml`) were rewritten to pass
  `PORYTILES_BUILD_VERSION_` and `PORYTILES_BUILD_DATE_` as standalone cmake cache args. The
  resolver short-circuits the default, the double-define disappears, and a clean override-path
  build now produces zero `macro-redefined` warnings in the log (verified locally before D5).
  See the Phase D pre-work prose for the full migration record, including the date-resolver
  addition and the CI date-format normalization from `date -uIseconds` (dashes) to dotted form.

---

## Phase D — CI / release pipeline overhaul (depends on A, C)

**Pre-work (single-source-of-truth promotion).** Two open decisions from the prompt resolved at session start: (1) snapshot version prefix and (2) date-macro resolver. Decision (1) initially framed as "CI parses CMakeLists.txt vs hardcode in YAML" was upgraded to a third option the user surfaced: **promote the version constant to a top-level `VERSION` file** that both CMakeLists.txt and CI read. This is the LLVM / Linux kernel pattern (`file(STRINGS ... LIMIT_COUNT 1)` in CMake; `cat VERSION` in CI), strictly cleaner than parsing CMakeLists with a regex. Implemented in root `CMakeLists.txt` BEFORE `project()` (`CMAKE_CURRENT_SOURCE_DIR` is available pre-project; `PROJECT_SOURCE_DIR` is not). Decision (2): added a parallel `if(NOT DEFINED PORYTILES_BUILD_DATE_) string(TIMESTAMP ... "%Y.%m.%dT%H:%M:%S+00:00" UTC) endif()` resolver mirroring Phase C's version resolver. Local builds now report a real UTC timestamp in `--version` (verified: `porytiles 1.0.0 2026.06.02T01:53:27+00:00`) instead of the 1970 fallback. Two parallel `target_compile_definitions(... PRIVATE PORYTILES_BUILD_DATE_=${PORYTILES_BUILD_DATE_})` lines added at the same three sites Phase C touched (`PorytilesLib`, `LegacyLib`, `PorytilesDriver`). Plan didn't explicitly include the VERSION-file promotion — credit to the user for asking "can we put just the version in a simple dot file" rather than accepting the CMakeLists-parsing option.

**Composite-action overhaul (Phase C carried-forward fix).** Phase C left a note that CI's existing `-DCMAKE_CXX_FLAGS="-DPORYTILES_BUILD_VERSION_=... -DPORYTILES_BUILD_DATE_=..."` channel still worked but emitted 109 `-Wmacro-redefined` warnings per build (compiler `-D` and CMake `target_compile_definitions` colliding). Migrated `.github/workflows/build_linux_clang/action.yml`, `build_linux_gcc/action.yml`, `build_macos_clang/action.yml` off the compiler-flag channel onto standalone cmake cache args (`cmake -DPORYTILES_BUILD_VERSION_=... -DPORYTILES_BUILD_DATE_=...`). The Phase C resolver block now short-circuits the default and the double-define disappears (locally verified: clean override-path build produces zero `macro-redefined` matches in the log). Added a `build-date` input to each composite action; if empty, action computes its own UTC timestamp inline. Normalized the CI date format from `date -uIseconds` (dashes — `2026-06-01T...`) to `date -u +'%Y.%m.%dT%H:%M:%S+00:00'` (dots), matching the new local CMake resolver. Behavior change: prior nightly tags reported `2026-06-01T...`, snapshot tags will report `2026.06.01T...`. Acceptable since nightlies are being retired.

- [x] **D1** `snapshot_release.yml` written; `nightly_release.yml` deleted. Triggers on `push: [develop]` + `workflow_dispatch`. Workflow-level `permissions: contents: write`. Concurrency `group: snapshot-release, cancel-in-progress: false`. Seven jobs (originally eight; one tap-test job dropped with the macos-amd64 deprecation — see follow-ups): **prepare** (computes `1.0.0-snapshot.${TIMESTAMP}.${SHORT_SHA}` from a single Unix-epoch snapshot — avoids skew across two `date` calls; outputs `build_version`, `build_date`, `short_sha`), **delete-prior-snapshot** (`gh release delete snapshot --yes --cleanup-tag || true`; `needs: [prepare]` so it doesn't nuke the prior snapshot if version computation fails), **build** (3-platform matrix `fail-fast: false`; uploads each `porytiles-<arch>.zip` as a v4 artifact rather than uploading to the release directly — kills the race where four parallel `softprops/action-gh-release` calls in the old nightly all attached to the same rolling tag), **publish** (downloads all three artifacts, runs `scripts/extract_changelog.sh Unreleased` for the release body, creates the `snapshot` tag with `prerelease: true, make_latest: "false"` — quoted strings, since softprops expects string types not booleans), **update-homebrew-tap** (updates `Formula/porytiles-snapshot.rb` in the tap repo via `secrets.PORYTILES_TAP_REPO_PAT`; reuses the existing sed patterns from the old nightly workflow, minus the OS.mac+intel branch), **test-brew-tap-{linux-amd64, macos-arm64}** (2 jobs verifying `brew install porytiles-snapshot` resolves both binaries; linux-arm64 tap test stays disabled — the existing workflow's comment about `ubuntu-24.04-arm` not shipping Homebrew still applies). Matrix over a literal `include` list (3 entries) rather than three or four duplicated jobs as in the existing nightly workflow — significantly shorter YAML, single edit-point for platform-set changes.
- [x] **D2** `versioned_release.yml` written. Triggers on `push: tags: ['v[0-9]+.[0-9]+.[0-9]+']`. Per-tag concurrency (`versioned-release-${{ github.ref }}`) so different tag pushes don't serialize. Tag-as-version strip: `BUILD_VERSION="${TAG_NAME#v}"` (e.g. `v1.0.1` → `1.0.1` baked into binary). Added a **VERSION-file vs tag consistency check** in `prepare`: if `cat VERSION` doesn't match the stripped tag, the workflow fails with `::error::` before building anything. Guards against the gitflow-violation scenario where someone tags `v1.0.1` without bumping `VERSION` first. `prerelease: false, make_latest: "true"` for proper "latest stable" pointer. CHANGELOG extraction uses the version (e.g. `1.0.0`) rather than `Unreleased`. Updates `Formula/porytiles.rb` (not snapshot). No delete-prior step — versioned releases are immutable.
- [x] **D3** `scripts/extract_changelog.sh` written. Bash + awk; takes a section name (`Unreleased` or `1.0.0`), prints lines between the matching `## [<section>]` heading and the next `## [...` heading. Buffer-and-flush pattern for inner blank lines (preserved); trims leading/trailing blanks naturally. Empty section emits empty output (exit 0). Verified against current CHANGELOG: `Unreleased` → empty; `1.0.0` → just the "First stable release..." paragraph; nonexistent `9.9.9` → empty.
- [x] **D4** Deleted `dev_build.yml` — fully redundant with `snapshot_release.yml`'s push-to-develop trigger (the snapshot workflow does a superset of work and fails identically on build errors, so a separate dev-build pipeline only delays feedback). Kept `pr_dev_build.yml` (PR validation is distinct from post-merge develop builds — no overlap). Left `build_pages.yml` alone — Phase F3's master-trigger rewrite is the right place for it, since `master` doesn't exist yet (Phase E creates it) and flipping the trigger now would freeze Doxygen deploys until E3. Final workflow set: `build_jobs_template.yml`, `build_pages.yml`, `changelog_check.yml`, `pr_dev_build.yml`, `snapshot_release.yml`, `versioned_release.yml` (6 top-level workflows + 9 composite actions). `build_jobs_template.yml` is now only called by `pr_dev_build.yml` — release pipelines stay self-contained because their matrix scope (3 platforms) and inputs (`build-version`, `build-date`) don't fit the template signature.
- [x] **D5** Verified the snapshot pipeline on a real develop push, after four pipeline iterations. Final passing run: `26904203219` (PR #311 merged to develop), produced snapshot `1.0.0-snapshot.20260603181655.37606464` with 3 platform zips (linux-amd64 88 MB, linux-arm64 88 MB, macos-arm64 12 MB), tap repo updated by the bot (`9e6eea2 Update porytiles-snapshot formula to ...`), `brew install grunt-lucas/porytiles/porytiles-snapshot` works on both linux-amd64 (via the rewritten `$GITHUB_PATH` + auto-tap pattern) and macos-arm64. The four iterations each surfaced a distinct real-world issue invisible to local pre-flight: (1) expired `PORYTILES_TAP_REPO_PAT` secret blocked `update-tap`; (2) brew formula name→class rules don't accept `@<non-digit>` like `@snapshot` — required rename to `porytiles-snapshot.rb`; (3) Linuxbrew + GHA-runner SIGPIPE on `eval "$(brew shellenv)"` broke the linux brew-install test step. Each fix was committed back to develop via separate PRs (PR #309 initial, PR #310 rename, PR #311 brew-test rewrite) and re-triggered the full pipeline. **D2 (`versioned_release.yml`) intentionally not pre-flight-tested** — first real run will be the v1.0.0 cut in Phase E3, coordinated with the `porytiles.rb` rewrite at E2 (per the deferred follow-up).

**Verification done locally (pre-D5).** Both default and override CMake paths pass clean. Default: `~/.local/bin/porytiles --version` → `porytiles 1.0.0 2026.06.02T01:53:27+00:00` (UTC); `~/.local/bin/porytiles-legacy --version` → same. Override: `cmake -DPORYTILES_BUILD_VERSION_=1.0.0-snapshot.20260601000000.abc12345 -DPORYTILES_BUILD_DATE_=2026.06.01T00:00:00+00:00` → both binaries echo the override strings exactly, zero `-Wmacro-redefined` warnings in the build log. `PorytilesAllTests` 1144/1144, `LegacyTests` 73 cases / 2,689,245 assertions. All 15 workflow / composite-action YAML files parse via `yaml.safe_load`.

**Open follow-ups carried into D5 (and beyond).**
- **Tap-repo prerequisite (snapshot formula).** D1's `update-homebrew-tap` job assumes `Formula/porytiles-snapshot.rb` already exists in the `grunt-lucas/homebrew-porytiles` repo with the expected shape (specifically: a `version "..."` line and three `sha256 "..."` lines under the `OS.linux?/Hardware::CPU.intel?` / `arm?` / `OS.mac?/Hardware::CPU.arm?` branches that the new sed patterns target). Resolved: a new `Formula/porytiles-snapshot.rb` was authored in the local clone with the correct shape, Ruby syntax verified via `ruby -c`, and the sed patterns simulated against it to confirm each branch's `sha256` line is updated to the correct platform's hash. The tap-repo commit + push is the user's responsibility before triggering D5.
- **Legacy `porytiles.rb` shape mismatch deferred to Phase E2.** The existing `porytiles.rb` in the tap is the legacy nightly-style formula (single binary, URL interpolates `#{nightly}` which is bound to the historical `nightly-3a9d31c...` tag). Phase D's new `versioned_release.yml` sed patterns update `version` and the three `sha256` lines, but NOT the `nightly = "..."` line, so running D2 against the current formula would leave the URL pointing at the legacy nightly while updating the version + sha256s — a broken composite state. Rewriting `porytiles.rb` now to the new shape (v-tag URL interpolation, both binaries installed) would break `brew install porytiles` for users until the first v* tag ships real release zips. **Decision: defer the rewrite to Phase E2** (release-branch prep), so the legacy nightly stays installable in the interim, and the formula transitions to the new shape in lockstep with the v1.0.0 cut. Tracked as a new E2 substep.
- **D5 scope reduced.** Original plan included a throwaway `v0.0.0-test` tag to pre-flight `versioned_release.yml`. With the `porytiles.rb` rewrite deferred to E2, a throwaway-tag run would touch the legacy-shape formula and produce a broken commit in the tap repo. Cleaner to skip D2 pre-flight and let v1.0.0 be its first real run. D5 now verifies only the snapshot pipeline. D2's YAML is trusted based on D1 verification (shared structure: prepare, build matrix, publish, tap update, brew tests) and local sanity (yaml-parses clean across all 15 workflow files; the VERSION-vs-tag guardrail logic exercised against multiple scenarios mentally).
- **Test binaries in release zips.** `create_release_package/action.yml` currently bundles three test binaries (`LegacyTests`, `PorytilesUnitTests`, `PorytilesIntegrationTests`) into each platform zip alongside the two driver binaries. Not in D's scope but worth questioning: are test binaries meant to ship to end users, or is this a vestige of CI's pre-package smoke test? If the latter, the test binaries can stay in CI (the action's `pushd ... && ./LegacyTests` step runs them right after copy) but be dropped from the zip itself. Phase E6 README rewrite is the natural place to decide.
- **Linuxbrew brew-test step rewritten to avoid `eval "$(brew shellenv)"` SIGPIPE.** First three full snapshot pipeline runs all passed every job except the `Test the brew tap snapshot on linux-amd64` step, which died with `##[error]Broken pipe` consistently in the ~240ms gap between `Tapped 2 formulae` (the final output of `brew tap`) and the next command starting. The macos-arm64 brew test passed once the formula rename landed, so the failure was Linuxbrew-specific and unrelated to the formula itself. Diagnosed as a GHA-runner + Linuxbrew interaction: `eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"` spawns brew as a subprocess that may still be flushing output (analytics or cleanup) when bash transitions to the next command; with `bash -e` semantics, a SIGPIPE on that flush propagates as non-zero exit and `set -e` kills the script. Rewrote the linux test step in both `snapshot_release.yml` and `versioned_release.yml`: (1) PATH setup via `echo "/home/linuxbrew/.linuxbrew/bin" >> "$GITHUB_PATH"` (the official GHA mechanism, no subshell, no eval), (2) `brew install grunt-lucas/porytiles/porytiles-snapshot` (fully qualified form auto-taps without explicit `brew tap` + `brew update`, collapsing three pipe-prone commands into one), (3) smoke test as a separate step. Each step gets its own bash process, so any residual pipe state from brew doesn't propagate. macos test left unchanged (still works with the original simpler pattern).
- **Snapshot formula renamed `porytiles@snapshot.rb` → `porytiles-snapshot.rb` (brew naming rule).** Original "locked-in decision" specified `porytiles@snapshot` as the formula name, parallel to brew's `python@3.11` / `gcc@13` versioned-formula convention. That convention is **only** valid when what follows `@` starts with a digit: brew's `Formulary.class_s` has a regex `sub!(/(.)@(\d)/, "\\1AT\\2")` that fires solely for `@<digit>` names. For `@<non-digit>` like `@snapshot`, the `@` stays literal in the expected class name (`Porytiles@snapshot` — not a valid Ruby identifier), so no class can ever match and the formula is unfindable. Diagnosed only after the D5 brew-install tests on both linux-amd64 and macos-arm64 failed with `Expected to find class Porytiles@snapshot, but only found: PorytilesATSnapshot`. Renamed to `porytiles-snapshot.rb` (hyphen-separated, matching brew conventions for non-version variants like `mysql-client`, `code-server`); class becomes `PorytilesSnapshot`; install command becomes `brew install porytiles-snapshot`. Workflow `snapshot_release.yml`, the tap-repo file, and the locked-in decisions section all updated. **Lesson for future formula authoring:** `ruby -c` syntax check is not sufficient — brew's name→class rules are a separate validation layer. Real pre-flight would be `brew audit` against a local Brew install.
- **`macos-amd64` divergence resolved by dropping from release matrices.** Initial Phase D rewrite copied the four-platform set from the old `nightly_release.yml` without cross-referencing the PR template's commented-out entry. Investigation traced the deprecation to commit `745b0152` (Oct 2025, "Comment out Intel Mac build for now, Porytiles2 has some incompatible dependencies") — `macos-amd64` was decommissioned because the active codebase's C++23 deps don't build on `macos-13`'s Apple Clang. The old nightly continued listing it because nightlies ran only on `workflow_dispatch` against the legacy codebase, so the breakage was passive. Phase D's `push:[develop]` trigger would have surfaced this as a snapshot-pipeline failure on first run. Pre-emptively dropped `macos-amd64` from both `snapshot_release.yml` and `versioned_release.yml` matrices (and from `porytiles-snapshot.rb`'s OS-arch conditional, and from the homebrew tap update's sed targets and download loop). The locked-in decisions section was updated from "four-platform matrix" to "three-platform matrix" with the deprecation citation. Re-introducing `macos-amd64` is a future toolchain-fix project, not in Phase D scope.

---

## Phase E — Gitflow adoption + 1.0.0 cut (last; A–D stable on develop)

Phases A–D + G + H landed stably on develop over 2026-06-02 through 2026-06-04. The 1.0.0 cut itself executed on 2026-06-05 as a single coordinated push of `2/release-prep` → develop (#312), followed by master creation, CHANGELOG date bump, and `v1.0.0` tag push. `versioned_release.yml`'s first real run completed green across all 8 jobs in 33m07s.

- [x] **E1** Audit open feature branches — resolved via archive-tag-and-delete pattern. The three open branches (`bug/anim-tiles`, `bug/issue-0060/key-frame-bug`, `feature/issue-0047/compiled-paired-primary`) were all single-commit 1.5+ year old investigations against the legacy `src/` codebase with their tracking GitHub issues already closed; preserved as `archived/<branch>` annotated tags (commits preserved in tag refs) and deleted from origin.
- [x] **E2** Cut work landed across multiple sub-steps on the `2/release-prep` branch (chosen "one-PR-at-end" over per-step PRs to minimize churn on develop while release prep firmed up):
  - **E2a** `VERSION` file + `project(... VERSION 1.0.0)` already established by Phase D pre-work; no edits needed.
  - **E2b** CHANGELOG migrated from `[Unreleased]` to `[1.0.0] - 2026-06-04`. Date was bumped to `2026-06-05` directly on master immediately before the v1.0.0 tag push, satisfying the VERSION-must-match-tag invariant on the actual release day.
  - **E2c** Version-bump philosophy absorbed into `RELEASE_PROCESS.md`'s "Choosing the version number" subsection in lieu of a standalone `STABILITY.md`. The standalone doc was drafted, reviewed, and rejected as overkill for a CLI tool of Porytiles's size — comparable indie/decomp tools (jq, ripgrep, Porymap, Poryscript) don't ship classification matrices and version fine without one.
  - **E2d** `homebrew-porytiles/Formula/porytiles.rb` rewritten to the versioned shape: `version "1.0.0"`, `v#{version}` URL interpolation, three placeholder `sha256` lines that `versioned_release.yml`'s `update-homebrew-tap` job sed-replaces at tag-push time. macos-amd64 branch dropped per the Phase D toolchain-constraint decision. Pushed to tap origin/main ahead of E3 so the broken-window between formula push and first real v* release was minutes, not days.
  - **E2e** Local smoke test green: clean rebuild produces both binaries reporting `porytiles 1.0.0 ...+00:00` and `porytiles-legacy 1.0.0 ...+00:00`.
- [x] **E3** Lockstep tag executed for main repo on 2026-06-05; docs-side deferred (see sub-bullet). Main flow: PR `2/release-prep` → develop (#312, `no-changelog` labeled, merged green); created master from develop's tip via `git checkout -b master develop && git push -u origin master`; bumped CHANGELOG date on master via direct push (bootstrap exception since branch protection wasn't yet in place); pushed annotated `v1.0.0` tag with `git tag -a v1.0.0 -m "Porytiles 1.0.0"`. `versioned_release.yml` first real run completed in 33m07s across all 8 jobs: prepare green (`cat VERSION` matched `${TAG#v}`), 3 platform builds green (linux-amd64 31m28s, linux-arm64 29m52s, macos-arm64 13m02s), publish created the permanent `v1.0.0` GitHub release with 3 platform zips + CHANGELOG-extracted release notes + marked-latest, `update-homebrew-tap` sed-replaced the porytiles.rb sha256s with real hashes (tap commit `22fbaca`), both brew-install smoke tests green (linux-amd64 28s, macos-arm64 37s). CHANGELOG date bump cherry-picked back to develop (commit `5b6aa9d8`) so both branches stay content-aligned per Branch invariant 2.

  **Doxygen Pages env-policy fix carried into E3.** First post-master Pages deploy failed with "Branch 'master' is not allowed to deploy to github-pages due to environment protection rules." Root cause: GH Pages environment had `develop` and a stale `2/mvp` in the allowlist, not `master`. Fixed via three `gh api` calls (added `master`, removed `develop`, removed `2/mvp`), and re-ran the failed workflow. Doxygen now serves from master only, matching F3's intent.

  **Docs lockstep deferred to [#313](https://github.com/grunt-lucas/porytiles/issues/313).** Both docs repos had master + annotated `v1.0.0` tag bootstrap-created during E3.6 then rolled back: a quiet bootstrap push leaves no archival PR for the docs 1.0.0 state, which is worth more than the lockstep convenience. The proper docs 1.0.0 release will follow the canonical Regular versioned release runbook (release/1.0.0 from develop → PR to master → tag), so the PR itself documents the initial docs state. F3-docs (Pages source flip + `github_version` in conf.py) and F6 (docs branch protection) ride along with that release.
- [ ] **E4** Branch + tag protection on main repo (`master`, `develop`, `v[0-9]+.[0-9]+.[0-9]+` tag pattern). Outstanding; last item before Phase E fully closes.
- [x] **E5** Gitflow conventions documented in `CONTRIBUTING.md` with a new Branching section (develop/master/release/hotfix conventions, `vX.Y.Z` tag pattern, `VERSION`-file-as-source-of-truth) and a Documentation section explaining the three-repo lockstep tagging plus the homebrew-porytiles tap as a fourth-but-not-tagged repo.
- [x] **E6** README install section reworked: workflow badges updated (`snapshot_release.yml` + `versioned_release.yml` replaced the deleted `dev_build.yml` + `nightly_release.yml`); new Release Cadence section with both `brew install` commands plus direct-download alternative plus Homebrew install/Linux pointers; Getting Started stubbed pending the user-docs Quick Start page; Building From Source trimmed to a one-line pointer at dev-docs; default branch confirmed as `develop`; stale `nightly-3a9d31c...` GH release + tag deleted (the rolling `snapshot` release stays). Downstream announce is the maintainer's manual step.

---

## Phase F — Documentation repos gitflow alignment (parallel with A–D; tag at E3)

> Commit all Phase F changes in `porytiles-user-docs/` and `porytiles-dev-docs/`,
> NOT the main repo.

Phase F landed on 2026-06-05 in lockstep with Phase E rather than the originally-planned parallel-with-A–D timing. F1, F2, F4, F5, F7 closed cleanly; F3 closed for the main repo's Doxygen deploy but the docs-repos portion + F6 + F8 ride along with the deferred docs 1.0.0 release ([#313](https://github.com/grunt-lucas/porytiles/issues/313)).

- [x] **F1** `release` branch deleted from each docs repo. Both repos had a stale `release` branch (4 commits behind develop, zero unique commits — vestigial reference from pre-gitflow days; not the canonical `release/<v>` form). `master` was NOT created at F1 because both docs repos were destined for the proper "release branch + PR + tag" flow at the eventual docs 1.0.0 release, not a bootstrap-master-from-develop push. Once that proper flow lands, master is created by the PR merge automatically.
- [x] **F2** Top-level `VERSION` file added to each docs repo (containing `1.0.0`). `docsrc/conf.py` modified in both: pathlib-based `_version_str = (Path(__file__).parent.parent / 'VERSION').read_text().strip()`; `version` and `release` both derive from `_version_str` (plus `myst_substitutions['version']` in dev-docs). Sphinx build verified locally: page titles render "Porytiles ... 1.0.0 documentation".
- [x] **F3** Main repo's `build_pages.yml` trigger flipped from `push: [develop]` to `push: [master]`. After master creation at E3, the published Doxygen site rebuilt successfully (after the GH Pages env-policy fix noted in E3). The docs-repos portion of F3 (GH Pages source flip from `develop /docs` to `master /docs` on each docs repo + GH Actions deploy workflow + `github_version` bumped to `master` in conf.py) is deferred to the future docs 1.0.0 release per the E3 docs-deferred note.
- [x] **F4** "Viewing Docs For Other Versions" section added to each docs `README.md`, covering both released-tag checkout and develop-branch checkout flows. Cross-references the existing "Building Locally" section to avoid duplicating build commands.
- [x] **F5** "Documentation" section added to main repo `CONTRIBUTING.md` explaining the three-repo lockstep tagging convention plus the tap as a fourth-but-not-tagged repo.
- [ ] **F6** Branch + tag protection on docs repos. Deferred — blocked by the docs 1.0.0 release flow (master doesn't exist on either docs repo until that flow lands). See [#313](https://github.com/grunt-lucas/porytiles/issues/313).
- [x] **F7** Initial content sync to current 1.0.0 state ("path-rename pass" scope). Discovered ~14 .md files with stale references to the old codebase naming (`Porytiles2/`, `porytiles2`, `Scripts/`, `Resources/`, `Notes/`) from before commit `8ea58d71`'s file/dir rename. Mechanical sweep updated all instances in `index.rst`, `tile-sharing.md`, `creating-your-first-tileset.md`, `importing-an-existing-tileset.md`, `installation.md` (user-docs) plus `index.rst`, `build-and-test.md`, `layered-architecture.md`, `project-layout.md`, `scripts-and-tooling.md`, `testbed.md`, `adding-a-config-value.md`, `adding-a-command.md`, `config-generation-system.md`, `writing-tests.md` (dev-docs). Index `.. note::` hardcoded "v2.0.0" version note switched to dynamic `|release|` substitution (renders from the VERSION file via Sphinx's built-in substitution). Testbed page's intentionally-illustrative 2.0.0 examples (JSON/YAML/RST field-list/`versionadded` directive demos) left as-is — they're syntax-demo content, not version claims.
- [ ] **F8** Per-repo end-to-end verify (build develop+master locally; GH Pages fires only on master push; checkout-tag v1.0.0 builds; version string renders). Deferred — pre-master portions are already verified (develop builds green, version renders 1.0.0); post-master portions ride along with the docs 1.0.0 release. See [#313](https://github.com/grunt-lucas/porytiles/issues/313).

---

## Phase G — AI policy documentation (parallel with A–F; land early)

- [x] **G1** Drafted `AI-POLICY.md` at repo root. Two-track stance: `legacy/` is closed to AI
  contributions of any kind (hard-line, no exceptions); `porytiles/` accepts AI-assisted work at
  maintainer discretion. Slop description stayed abstract per maintainer choice (defers to STYLE.md
  and reviewer judgment, no enumeration of specific tells). No disclosure checkbox, no detector.
  Revisit-cadence section omitted entirely. Pointers to `STYLE.md` and `CONTRIBUTING.md` included.
  ~25 lines of prose, one screen. Slop self-audit pass complete (no em-dashes, no marketing prose,
  no platitude bullets).
- [x] **G2** Added AI-POLICY reference to `CONTRIBUTING.md` (intro paragraph). Bundled with a
  broader simplification of `CONTRIBUTING.md` per maintainer direction: dropped the Topic Branch
  Conventions section (6 subsections) and Issues section, since GitHub labels handle PR
  categorization. The sembr endorsement that lived in the dropped Documentation subsection moved
  to `STYLE.md` under a renamed "Prose Style" section (was "Comment Prose Style"). Document went
  from ~115 lines to ~50. Optional `README.md` mention still deferred to a later phase.
- [x] **G3** Verified: `AI-POLICY.md` exists at repo root; `STYLE.md` and `CONTRIBUTING.md` link
  references resolve; the doc reads as not-itself-slop after maintainer edits and the bundled
  simplification pass.

---

## Phase H — Top-level lowercase migration (after A; before D)

Bundles Tier 1 + Tier 2 + Tier 3 of the naming convention audit into one
mechanical pass: rename PascalCase top-level dirs to lowercase, normalize
test-asset kebab-case to snake_case, and rename `AI-POLICY.md` to `AI_POLICY.md`.
Lands BEFORE Phase D so the CI/release pipeline rewrite already references final
paths. Independent of B, C, F, G.

> ⚠️ Case-only renames on macOS HFS+/APFS need the intermediate workaround:
> `git mv X X_tmp && git mv X_tmp x`. Every clone must `rm -rf porytiles-build-*`
> after merging Phase H — CMake won't detect the path case change.

### H1 — Decide final target slugs (one-shot, at execution start)

- [x] `Porytiles/` → `porytiles/`, `Legacy/` → `legacy/`, `Scripts/` → `scripts/`,
  `Porytiles/Notes/` → `porytiles/notes/` (Tier 1 PascalCase outlier subsumed).
- [x] **Documentation slug chosen: `docs/`** (LLVM / Catch2 / fmt convention; three
  characters shorter than `documentation/`, no abbreviation cost since `docs` is the
  industry norm for C++ projects). Applied uniformly: `Documentation/` → `docs/`,
  `Documentation/Wiki/` → `docs/wiki/`.
- [x] `Resources/` → `resources/`, with all PascalCase children lowercased in lockstep:
  `Examples/`, `Doctests/`, `Readme/`, `Tests/` → `examples/`, `doctests/`, `readme/`,
  `tests/`.

### H2 — Mechanical path sweep

- [x] **One ordered sed script applied to all tracked text files.** Long-prefix-first
  ordering (e.g., `Resources/Doctests/` before `Resources/`, `precision-loss-test-2`
  before `precision-loss-test`, tutorial names before bare `palette-overrides`) to
  avoid substring overshoot. 24 substitution rules covering 11 path prefixes + 13
  kebab→snake fixture-container names. **Touched 101 tracked text files** across
  workflows, scripts, CMakeLists, IDE configs, Claude-Code docs, top-level docs,
  Jinja templates, config headers, integration tests, legacy doctests, and Notes.
- [x] **Three identifier-form holes caught by post-sweep audit** (regex required a
  trailing `/`, so no-trailing-slash refs slipped):
  (1) Root `CMakeLists.txt` `add_subdirectory(Legacy)` / `(Porytiles)` /
  `(Documentation)` triplet;
  (2) `.github/workflows/create_release_package/action.yml` `resources/Doctests`,
  `resources/Examples`, `resources/Readme`, `resources/Tests` (the `cp -r` lines
  bundling release zips) plus the destination subdir `./porytiles-<arch>/Resources`;
  (3) `porytiles/tests/integration/utilities/c_parser/c_parser_facade_test.cpp` lines
  22-25: a `"Resources"` sentinel string used by `test_resource_path()` to walk up
  from cwd to the repo root.
- [x] **Two prose-case false positives reverted:**
  `porytiles/include/porytiles/infra/algorithms/anim_frame_loader.hpp` line 12
  contained `Porytiles/Porymap` as prose meaning "Porytiles or Porymap" (not a path
  literal); reworded to that phrasing rather than left as broken kebab.
  `docs/Doxyfile.in` line 1539 references the macOS system path
  `~/Library/Developer/Shared/Documentation/DocSets` — sed had lowercased the
  `Documentation` to `docs`, reverted since it's a Doxygen-docset install location
  Apple controls, not a project path.
- [x] `.clang-tidy`'s `HeaderFilterRegex: 'porytiles/.*'` already targets the
  include-namespace (lowercase from Phase A) — no edit needed, as the plan
  predicted.
- [x] **Planning artifact at `.claude/plans/...`** intentionally NOT swept — it lives
  outside the tracked tree and the dashboard (this file) is the durable artifact;
  the plan file is ephemeral.

### H3 — `AI-POLICY.md` → `AI_POLICY.md` (Tier 1)

- [x] `git mv AI-POLICY.md AI_POLICY.md`; inbound `CONTRIBUTING.md` reference
  (`[`AI-POLICY.md`](./AI-POLICY.md)`) updated to point at the new filename.
- [x] **Historical mentions left as-is per Phase A4f precedent.** The dashboard's
  Phase G log entries and `porytiles/notes/release_1_0_0_prep_plan.md` describe the
  artifact under its pre-rename name `AI-POLICY.md`; rewriting them would create
  self-contradictions ("renamed AI-POLICY.md to AI_POLICY.md" doesn't make sense if
  both names read `AI_POLICY.md`).

### H4 — `resources/` test-asset normalization (Tier 2)

- [x] **5 tutorial container dirs** under `resources/examples/`:
  `palette-overrides-tutorial`, `palette-primers-tutorial`, `porytiles-anim-tutorial`,
  `porytiles-primary-tutorial`, `porytiles-secondary-tutorial` → snake_case
  equivalents. Same for tutorial sub-containers (`primary-with`, `primary-without`,
  `secondary-with`, `secondary-without`).
- [x] **2 `precision-loss-test` container dirs** under `resources/doctests/` →
  `precision_loss_test`, `precision_loss_test_2`.
- [x] **Critical cascade caught, reverted: `palette-overrides/` and `palette-primers/`
  leaf fixture dirs kept kebab.** These leaf names mirror the legacy tool's
  user-facing INPUT PROTOCOL — the directory names that downstream Pokemon decomp
  projects use (per the porymap convention). The initial Tier 2 sweep blindly
  renamed them to snake, which cascaded into 4 string literals in
  `legacy/include/porytiles_legacy/legacy/types.h`'s `CompilerSourcePaths` (the
  runtime path-builder for `primaryPalettePrimers()`, `secondaryPalettePrimers()`,
  `primaryPaletteOverrides()`, `secondaryPaletteOverrides()`) AND 4 lines of CLI
  help text in `legacy/lib/legacy/cli_parser.cpp` documenting the protocol to users.
  That's a breaking change to the legacy binary's input contract, not a
  test-asset normalization. **Decision: revert.** Reverted the 12 leaf fixture dirs
  (`palette_overrides/` → `palette-overrides/`, `palette_primers/` → `palette-primers/`
  under each of the 9 doctest cases + 3 example sub-containers), reverted the
  protocol literals in `types.h` and `cli_parser.cpp` help text, reverted the ~50
  doctest path string literals in `legacy/lib/legacy/compiler.cpp` to use the
  snake-container/kebab-leaf form (e.g.
  `resources/doctests/palette_override_1/palette-overrides/00.pal`). The plan author
  flagged this kind of cascade only after the fact — worth recording for future
  fixture-rename phases.
- [x] **Active integration tests had zero kebab-leaf references** — they don't
  consume the legacy input protocol. `porytiles/tests/integration/**/*.cpp` references
  to `resources/` (10 files) all use snake container names or top-level paths.

### H5 — End-to-end verification

- [x] `rm -rf porytiles-build-debug && cmake -B porytiles-build-debug -S .` exit 0
  (clean configure took 79s including FetchContent populate of doxygen-awesome-css).
- [x] `cmake --build porytiles-build-debug -j7` exit 0; zero `error:` / `undefined
  symbol` / `fatal error` matches in log.
- [x] `PorytilesAllTests`: 1144 tests across 99 suites, all pass (957ms). Unchanged
  count from Phase A6.
- [x] `LegacyTests`: 73 doctest cases, **2,689,245 assertions** all pass — also
  unchanged from Phase A6, confirming the kebab-leaf revert preserved the legacy
  doctest paths correctly.
- [x] `cmake --install porytiles-build-debug --prefix ~/.local` exit 0; both
  `~/.local/bin/porytiles --version` and `~/.local/bin/porytiles-legacy --version`
  run successfully. Output still the static `default_build_version
  1970.01.01T00:00:00+00:00` fallback — Phase C will replace this.
- [x] **Regen idempotent.** First `uv run scripts/generate_config.py` produces a diff
  vs HEAD: (a) the swept `Scripts/` → `scripts/` auto-gen banner one-liner
  in 2 generated config headers (`app_config.hpp`, `primary_pairing_mode.hpp`),
  matching what the swept Jinja template now emits; (b) a few stray blank-line
  additions that look like pre-existing template-vs-output drift unrelated to
  Phase H. A SECOND run produces zero new diff (true idempotency). Build + tests
  re-run against the regen output also green.
- [x] **Residual PascalCase grep**: `git grep -nE '(Porytiles|Legacy|Documentation|
  Resources|Scripts)/'` across tracked source returns exactly one match:
  `docs/Doxyfile.in:1539` — the macOS system-path comment deliberately preserved
  in H2. Identifier-form grep `git grep -nE '\b(Doctests|Examples|Readme|Wiki)\b'`
  in scope returns empty.
- [x] **macOS case-insensitivity post-mortem (caught by CI on Ubuntu).** Initial
  H5 reported green on local macOS, but Ubuntu amd64 CI failed in
  `scripts/generate_config.py` because the path-component string `"Porytiles" /
  "config_templates"` (constructed via `pathlib`, no trailing slash to anchor my
  regex on) hadn't been swept. APFS resolved the capital-P path to the lowercase
  dir transparently; ext4 did not. Follow-up sweep caught 11 more identifier-form
  path-component holes in `scripts/{generate_config,coverage,format,new_class,tidy,
  todo}.py` (all `project_root / "Porytiles" / ...` or `default="Porytiles"`
  patterns) and lowercased them. Remaining capital-P refs in scripts (~10) are
  prose in docstrings, `--help` text, and module-level comments referring to the
  product name; those stay. Workflow `name:` fields ("Checkout Porytiles
  repository") and CMake-comment refs likewise are product-brand prose.
  **Lesson for future Phase A-style renames: a sweep regex that requires a
  trailing `/` must be paired with a separate identifier-form audit
  (`"Porytiles"` quoted as a path component, `default="Porytiles"`, etc.),
  ideally run against a case-sensitive filesystem.**

**Cross-cutting notes**
- Same disposition as Phase A for open feature branches (`bug/anim-tiles`,
  `bug/issue-0060/key-frame-bug`, `feature/issue-0047/compiled-paired-primary`):
  hand-port if still live, close if not. Not a Phase H prerequisite.
- External user-facing docs (`porytiles-user-docs`, `porytiles-dev-docs`) may
  reference `porytiles/` paths in tutorials; Phase F should sweep those repos
  during its initial-sync step.
- CHANGELOG entry: Phase H is release-prep structure, not user-facing behavior.
  Same framing as B and G — no CHANGELOG entry needed; the bundled PR's diff
  touches `CHANGELOG.md` incidentally so the gate passes without `no-changelog`.

---

## End-to-end verification (after all phases)

- [x] Clean build produces `porytiles` + `porytiles-legacy`; `--version` reports `1.0.0` + date.
- [x] `cmake --install` writes both binaries to `<prefix>/bin/`.
- [x] Develop push → exactly one `snapshot` release; zips contain both binaries; brew snapshot re-pulls. (Phase D5.)
- [x] First `v1.0.0` tag → versioned release; brew stable re-pulls. (E3 / 2026-06-05.) Subsequent `v1.0.1` would leave `v1.0.0` untouched per the immutable-release pipeline design.
- [x] `brew install grunt-lucas/porytiles/porytiles` and `...porytiles-snapshot` both work, both binaries.
- [x] `CHANGELOG.md` has a date-stamped 1.0.0 section.
- [ ] Branch protection rejects direct push to `master`. (E4 pending.)
- [x] CHANGELOG workflow blocks a PR missing a CHANGELOG entry (unless `no-changelog`). (Verified via B's recursive bootstrap.)
- [x] `AI_POLICY.md` exists at repo root; `CONTRIBUTING.md` links it; legacy AI-free
  stance unambiguous. (File was `AI-POLICY.md` pre-Phase H3.)
- [x] `RELEASE_PROCESS.md`'s "Choosing the version number" subsection documents
  version-bump philosophy in lieu of a separate `STABILITY.md`.
- [x] Top-level directory names are all lowercase (`porytiles/`, `legacy/`,
  `scripts/`, plus the H1-decided slugs for docs/resources). Residual greps in H5
  return zero hits in tracked source.
