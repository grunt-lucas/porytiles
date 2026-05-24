# Porytiles 1.0.0 Release Prep — Sequenced Task Plan

## Context

Porytiles is preparing its first official versioned release (1.0.0). Today the
repo carries two parallel codebases under `Porytiles1/` (legacy) and
`Porytiles2/` (active, DDD-architected). The active codebase is mature enough to
be promoted to the canonical name, the legacy codebase remains useful but should
be clearly badged as legacy, and the release infrastructure (CI, homebrew,
versioning, branching model) needs to be production-grade before tagging 1.0.0.

This plan organizes the work into ordered phases that respect a critical
include-path collision (Porytiles1's include directory is already `porytiles/`,
so the Porytiles2 rename to `porytiles/` cannot happen until Porytiles1's
include subdir is renamed first), produce a rolling snapshot release on every
push to `develop`, preserve permanent versioned releases on every `v*` tag from
`master`, ship both binaries via Homebrew with separate stable/snapshot
formulas, and adopt proper gitflow with the first 1.0.0 tag cut from a
`release/1.0.0` branch.

The user will work through these phases by directing the assistant to execute
specific phases or steps. This file is the master reference for the work.

---

## Locked-in decisions

- **Renames**:
  - `Porytiles2/` → `Porytiles/`; namespace `porytiles2` → `porytiles`; include prefix `porytiles2/` → `porytiles/`; executable `porytiles2` → `porytiles`.
  - `Porytiles1/` → `Legacy/`; namespace `porytiles1` → `porytiles_legacy`; **include prefix `porytiles/` (inside Porytiles1) → `porytiles_legacy/`**; executable `porytiles` → `porytiles-legacy`.
  - CMake targets: `Porytiles2Lib` → `PorytilesLib`, `Porytiles2Driver` → `PorytilesDriver`, `Porytiles2{Unit,Integration,All}Tests` → `Porytiles{Unit,Integration,All}Tests`. Mirror for legacy: `Porytiles1Lib` → `LegacyLib`, etc.
- **CHANGELOG.md**: simplified Keep A Changelog (flat list, no Added/Removed/Modified split). Lives at repo root. Starts fresh at `## [1.0.0]`; ongoing work accumulates under `## [Unreleased]`.
- **Snapshot release**: rolling tag `snapshot`; force-replaced on every push to `develop`; build version string `1.0.0-snapshot.YYYYMMDDHHMMSS.<short-sha>` (full SHA in `--version` output for bug reports); both binaries bundled per platform; concurrency-grouped so back-to-back develop pushes serialize.
- **Versioned release**: triggered on tag matching `v[0-9]+.[0-9]+.[0-9]+` pushed to `master`; preserved permanently; same four-platform matrix; both binaries bundled.
- **Homebrew** (in `grunt-lucas/homebrew-porytiles` tap): two formulas. `porytiles.rb` tracks latest `v*` tag, updated by versioned-release workflow. `porytiles@snapshot.rb` tracks rolling snapshot, updated by snapshot workflow. Both install both binaries.
- **Gitflow**: `develop` = integration, push triggers snapshot. `master` = production, created at 1.0.0 cut, tag pushes trigger versioned release. `release/<v>` cut from develop for stabilization. `hotfix/<v>` cut from master for emergency fixes. Both `release/*` and `hotfix/*` merge back to BOTH `master` and `develop`.
- **Version source of truth**: top-level `CMakeLists.txt` `project(Porytiles VERSION 1.0.0 ...)`. Sub-CMakeLists read `${CMAKE_PROJECT_VERSION}` (NOT `${PROJECT_VERSION}` — sub-projects shadow it). Snapshot/CI builds override via existing `-DPORYTILES_BUILD_VERSION_=...` flag.
- **Pre-1.0 tags** (`0.0.1`–`0.0.7`, `nightly-3a9d31c...`): preserved as historical artifacts; do not delete.
- **Existing open feature branches** (`bug/anim-tiles`, `bug/issue-0060/key-frame-bug`, `feature/issue-0047/compiled-paired-primary`): cannot be rebased through the rename — their commits modify pre-split file paths that no longer exist. Hand-port (or close) is a separate effort, NOT a prerequisite of release prep. Flagged for user decision before rename PRs merge.
- **Documentation repositories versioning**: both `porytiles-user-docs` and `porytiles-dev-docs` adopt the same gitflow model as the main repo (`develop` for in-progress, `master` for stable, `release/*` and `hotfix/*` cut from those, tagged `v*` on master). GitHub Pages on each docs repo deploys ONLY from `master` HEAD — so the public site always shows the latest stable release docs. Users wanting docs for older versions or for the develop snapshot must clone the docs repo locally, check out the desired tag/branch, and run `cd docsrc && uv run make html`. Docs repo versioned tags mirror main repo tags exactly (e.g., main repo `v1.0.0` ⇔ both docs repos `v1.0.0`). Each release cut tags all three repos in lockstep.
- **Main repo Doxygen GitHub Pages site** (`grunt-lucas.github.io/porytiles/`, separate from the two Sphinx docs sites): adopts the same stable-only deploy model. The existing `build_pages.yml` workflow is rewritten to trigger on `push: branches: [master]` instead of `develop`. Snapshot Doxygen viewing is local-only: `cmake --build porytiles-build-debug --target doxygen` then open the generated HTML. All three GH Pages sites (user-docs Sphinx, dev-docs Sphinx, main-repo Doxygen) follow identical stable-only conventions for consistency.
- **AI contribution policy**: new file `AI-POLICY.md` at repo root, two tracks. **Legacy (`Legacy/`) is AI-free** — the codebase was written without AI assistance and will accept no AI-generated or AI-assisted contributions going forward. **Active Porytiles (`Porytiles/`) accepts AI-assisted contributions** at maintainer discretion, with explicit rejection of "slop" (hallucinated APIs, generic boilerplate, padded prose, AI-tells like excessive em dashes). AI-assisted documentation is welcome on the same quality bar. Enforcement is subjective and lives with the maintainer at review time — no automated detector, no PR-template disclosure checkbox. See dedicated "AI contribution policy" section below for the detail that Phase G's file should codify.

---

## Critical sequencing invariant

**A1 (Legacy include-prefix rename) MUST land before A2 (Porytiles2 rename) merges.** Otherwise both codebases simultaneously claim `<root>/<dir>/include/porytiles/` and any TU including a bare `porytiles/...` header has ambiguous resolution. This dependency is non-negotiable; everything else in the plan can be re-ordered or parallelized.

---

## Open design question: 1.0.0 public API surface

**Status: UNRESOLVED. Must be settled before tagging `v1.0.0` (Phase E3).**

Tagging 1.0.0 implies a semver contract: subsequent `1.x.y` releases must preserve backwards compatibility on the "public" surface, while `2.0.0` is the next opportunity to break it. We have not yet decided what counts as the public surface. This is a one-time definition exercise — getting it wrong locks us into either unwanted compatibility burden or premature major-version bumps.

Categories to classify as **stable** (semver-protected), **experimental** (subject to change in any release with a CHANGELOG note), or **internal** (no guarantees):

- **CLI flag surface for `porytiles`**: all subcommands, all flags, all flag short forms, default values, environment variable names. Are any flags currently marked experimental? Should new flags be experimental by default until a future release promotes them?
- **CLI flag surface for `porytiles-legacy`**: probably frozen as-is (it's legacy — no new features); but is its behavior under 1.0.0 considered stable? Likely yes — legacy users depend on it not breaking.
- **YAML config schema** (`porytiles.example.yaml` and the `config_schema.yaml` it derives from): all top-level keys, value types, enum string values, override precedence rules. The config-generation system makes this surface large and easy to grow accidentally.
- **Output file formats**: `metatiles.bin`, `metatile_attributes.bin`, `tiles.png`, `palettes/*.pal` — these are dictated by Porymap and the decomp toolchain, NOT by Porytiles. They're effectively externally-stable; document that constraint.
- **Project layout on disk** (the `porytiles/` directory inside a user's decomp project, with `tilesets/`, `assets/`, etc.): is the directory layout part of the contract? Renaming `porytiles/` inside user projects would break every existing user.
- **Exit codes**: do users script around specific exit codes? If yes, define which codes mean what.
- **Diagnostic message text**: almost certainly **NOT** stable — error messages get rewritten constantly. Declare explicitly so users don't grep them.
- **Diagnostic codes / error tags**: if diagnostics have stable identifiers (e.g., `PT-E001`), THOSE are part of the contract even though the message text is not.
- **C++ library API** (`PorytilesLib` headers under `Porytiles/include/porytiles/`): is anyone consuming Porytiles as a library, or is it `porytiles` (the binary) only? If library-only-internal, declare so loudly — otherwise you owe ABI/API guarantees on hundreds of headers.
- **CMake target names** (for downstream CMake consumers using `find_package(Porytiles)`): tied to library question above.
- **Build requirements** (C++23, CMake 3.20+): these are looser semver concerns but should be documented — bumping the minimum CMake version mid-1.x would surprise people.
- **Generated code locations / config templates**: definitely internal.

Suggested deliverable when this gets worked out: a top-level `STABILITY.md` (or a section of `README.md` / `CONTRIBUTING.md`) that lists each category and its stability class, plus a deprecation policy (e.g., "deprecated flags warn for one minor version before removal in the next major"). The CHANGELOG enforcement workflow (Phase B2) could optionally check for the `BREAKING:` keyword in `CHANGELOG.md` entries and gate them behind a major-version bump.

This question is called out explicitly in Phase E2 as a release-cut prerequisite.

---

## Documentation lifecycle: handling doc-only fixes between code releases

A real scenario: we tag `v1.2.0` in main + both docs repos. A week later, someone spots an error in `porytiles-user-docs`. How does the fix get published without disturbing the lockstep versioning?

**Decision: tags are immutable historical snapshots; docs `master` is a rolling head that GitHub Pages always serves.**

Concretely, when a doc-only fix is needed for a published version:

1. Branch off the affected docs repo's `master`: `git checkout -b docfix/<short-description> master`.
2. Fix the error, PR, merge to `master`. (For trivial typo fixes, the team may relax PR-review requirements via the standing branch-protection config — but never skip the PR mechanism itself.)
3. GH Pages workflow rebuilds and redeploys. Public site is correct within minutes.
4. Cherry-pick the same fix to `develop` so the next release doesn't regress: `git checkout develop && git cherry-pick <master-fix-sha> && git push`.
5. **DO NOT** create a new tag (`v1.2.0.1`, `v1.2.0-doc.1`, etc.). DO NOT move the existing `v1.2.0` tag.

**Implications of this policy:**
- The Sphinx config still shows "Porytiles 1.2.0 documentation" on the public site — the docs are *describing* 1.2.0, just doing it more accurately than they were on release day.
- `git checkout v1.2.0` in a docs repo retrieves the docs as they existed at tag time, NOT the current best 1.2.0 docs. The tag is for archival/audit ("what did we publish the day 1.2.0 shipped?"), not the recommended consumption path.
- The public GH Pages site is always the authoritative "current best docs for the latest stable release."
- This mirrors the Read the Docs convention: tags are point-in-time snapshots, the "latest" alias tracks master.

**When this policy is NOT enough** (escape hatches, for future consideration):
- If porytiles ever maintains two major versions in parallel (e.g., `1.x` LTS users + `2.x` current users, each getting independent doc fixes), the single-master model breaks down. At that point, introduce per-major-version maintenance branches in the docs repos (`support/1.x` etc.) — but only when there's an actual second major version to support. Don't pre-build this.
- If a doc error is severe enough that auditability of "exactly what changed and when" matters (e.g., a corrected security-relevant instruction), the CHANGELOG.md in the affected docs repo should record the fix with date — but still no version-tag mutation. The git commit + CHANGELOG entry together provide the audit trail without breaking immutability of tags.

**Where this policy applies**: all three GH Pages sites (user-docs Sphinx, dev-docs Sphinx, main-repo Doxygen) follow the same lifecycle. Doxygen doc fixes that aren't code fixes are rare (the .h header doc comments ARE the source), but if a Doxyfile config tweak or generated-page CSS fix is needed between releases, the same pattern applies: fix on `master`, GH Pages rebuilds, no new tag.

---

## AI contribution policy

**Status: stance locked in, document yet to be drafted (Phase G).**

Porytiles will ship 1.0.0 with an explicit `AI-POLICY.md` at the repo root so contributors know the project's posture before opening a PR. The policy has two distinct tracks, reflecting the two codebases the repo carries.

**Legacy (`Legacy/` directory) — AI-free zone**

- The legacy codebase was developed without AI assistance and will remain AI-free going forward.
- No AI-generated code, AI-assisted edits, or AI-generated documentation will be accepted in `Legacy/`.
- Rationale: legacy is in maintenance mode; the original author understands every line; introducing AI-assisted changes there breaks that property and adds review burden for low-value patch traffic.
- Practical implication: PRs touching `Legacy/` files must affirm the change is human-authored. Maintainer may close AI-assisted PRs to `Legacy/` without detailed review.

**Active Porytiles (`Porytiles/` directory) — AI-assisted OK, slop rejected**

- AI-assisted contributions (code and docs) are accepted at maintainer discretion.
- "Slop" will be rejected. The bar is subjective and the maintainer is the judge.
- Heuristics that mark a contribution as slop (non-exhaustive — these are what to lift into `AI-POLICY.md`):
  - Hallucinated APIs, missing or wrong includes, fabricated function signatures, references to libraries the project doesn't use.
  - Generic boilerplate that doesn't engage with this codebase's patterns (e.g., raw `std::shared_ptr` everywhere when the project uses value semantics and `gsl::owner` / `gsl::not_null`).
  - Tautological comments that restate what well-named code already says (`STYLE.md`'s "default to no comments" rule applies regardless of authorship).
  - Over-engineered abstractions where a few lines of straight code would do; speculative class hierarchies; premature factories.
  - Padded prose in docs and commit messages. Specific AI-tells: overuse of em dashes (see existing project preference), "comprehensive", "robust", "delve into", "moreover", "it's important to note that".
  - Failure to match `STYLE.md` conventions: snake_case mismatches, missing `[[nodiscard]]`, Doxygen tag misuse, trailing-underscore convention violations.
  - Tests that pass without exercising the code paths they claim to test (coverage gate from `Scripts/coverage.py` makes this auditable).
- AI-assisted documentation is welcome on the same bar — well-edited AI output that reads naturally and is technically accurate is fine; the "AI essay voice" is not.
- No formal disclosure requirement. The maintainer reads PRs carefully regardless of authorship; an AI-assisted PR that meets the bar is indistinguishable from a human PR that meets the bar, and a checkbox doesn't help when the gate is the diff itself.

**Open questions to settle when drafting the policy file:**

- Should the PR template add a checkbox/dropdown for AI-assist disclosure? Argument for: helps the maintainer calibrate review intensity. Argument against: subjective bar makes the answer non-actionable, invites gaming, adds friction for honest contributors. Current lean: against.
- Should the policy reference specific tools (Claude Code, Copilot, Cursor, ChatGPT) or stay tool-agnostic? Current lean: tool-agnostic — the bar is on the output, not the source.
- Should there be an annual revisit clause baked into the document? The AI landscape will shift between 1.0.0 and 2.0.0; pinning a review date avoids the policy drifting out of relevance silently. Current lean: yes, "to be revisited annually."

This section is the planning artifact; `AI-POLICY.md` is the operational artifact contributors see (Phase G).

---

## Phase 0: Document this plan in the repo

Write `RELEASE_PREP_1_0_0.md` at the repo root containing the locked-in decisions, the phase outline, and a checklist the user can tick through. This file is the working document the user references when directing future sessions ("execute Phase A2 from RELEASE_PREP_1_0_0.md").

The plan file you're reading right now (`.claude/plans/...`) is the planning artifact; `RELEASE_PREP_1_0_0.md` is the operational artifact committed to the repo.

---

## Phase A: Renames (lands on `develop` via incremental PRs)

### A1 — Legacy include-prefix rename (collision unblocker)

This phase is small but critical: it must complete and merge to `develop` before A2 begins. Scope:

- Rename `Porytiles1/include/porytiles/` → `Porytiles1/include/porytiles_legacy/`.
- Update all 55 Porytiles1 TUs to use `#include "porytiles_legacy/..."` instead of `#include "porytiles/..."`.
- Update `Porytiles1/lib/CMakeLists.txt` — `CANONICAL_LIB_NAME` (currently `"porytiles"`) → `"porytiles_legacy"`. This also changes the library output filename, which is fine since the legacy library is not currently installed.
- Update `Porytiles1/CMakeLists.txt` include dir variable (`PORYTILES1_INCLUDE_DIR`) usage to point at the new subdir.
- Update namespace declarations `namespace porytiles1` → `namespace porytiles_legacy` in 6 files: `panic.hpp`, `diagnostics.hpp`, `diagnostic_engine.hpp`, `porytiles_context.cpp`, `emitter.cpp` (and one other).
- Verify `Porytiles1Driver`, `Porytiles1Tests` still build and run.

This phase intentionally leaves the Porytiles1 *directory* name alone (still `Porytiles1/`). Renaming the top-level dir is part of A3.

### A2 — Porytiles2 → Porytiles (the big rename)

Once A1 is merged, perform the bulk rename. Treat the surface in sub-bullets so this can be split into multiple PRs if review burden is high:

- **A2a — Directory + CMake skeleton**:
  - Rename `Porytiles2/` → `Porytiles/`.
  - Update root `CMakeLists.txt` `add_subdirectory(Porytiles2)` → `add_subdirectory(Porytiles)`.
  - Rename CMake variable `PORYTILES2_INCLUDE_DIR` → `PORYTILES_INCLUDE_DIR` everywhere.
  - Rename CMake targets: `Porytiles2Lib` → `PorytilesLib`, `Porytiles2Driver` → `PorytilesDriver`, `Porytiles2{Unit,Integration,All}Tests` → `Porytiles{Unit,Integration,All}Tests`.
  - In `Porytiles/lib/CMakeLists.txt`: update `project(Porytiles2Lib CXX)` → `project(PorytilesLib CXX)`; `CANONICAL_LIB_NAME "porytiles2"` → `"porytiles"`. Update `export(TARGETS ...)` file name.
  - In `Porytiles/tools/driver/CMakeLists.txt`: rename `project(Porytiles2Driver CXX)` → `project(PorytilesDriver CXX)`; `OUTPUT_NAME "porytiles2"` → `"porytiles"`.
  - In `Porytiles/tests/CMakeLists.txt`: update all three test-target names.
  - In `Porytiles/CMakeLists.txt`: update all 27 generated-file paths from `Porytiles2/include/porytiles2/...` to `Porytiles/include/porytiles/...`. Update `${CMAKE_SOURCE_DIR}/Porytiles2/config_templates/` references.
  - In `Documentation/CMakeLists.txt`: update `Porytiles2Lib`, `PORYTILES2_PUBLIC_HEADER*`, and the three `${PROJECT_SOURCE_DIR}/Porytiles2/{include,lib,tools}` paths.

- **A2b — Include directory + namespace sweep**:
  - Rename `Porytiles/include/porytiles2/` → `Porytiles/include/porytiles/`.
  - Mechanical search-and-replace across all 391 source files: `#include "porytiles2/` → `#include "porytiles/` (1,670 occurrences).
  - Mechanical search-and-replace: `namespace porytiles2` → `namespace porytiles` (722 occurrences).
  - Mechanical search-and-replace: `porytiles2::` → `porytiles::` for qualified references.
  - Update `} // namespace porytiles2` closing comments.

- **A2c — Build-version macro updates** (per-binary defaults so `--version` is correct without CI overrides):
  - `Porytiles/include/porytiles/build_version.h`: `#define PORYTILES_EXECUTABLE_ porytiles` (was already `porytiles`, now correctly matches the active binary).
  - Already covered in A1 for legacy: update `Legacy/include/porytiles_legacy/build_version.h` to `#define PORYTILES_EXECUTABLE_ porytiles-legacy`.

- **A2d — Jinja2 templates + generator script**:
  - Update all 29 templates under `Porytiles/config_templates/*.jinja2`: replace `porytiles2` with `porytiles` in include paths and namespace declarations.
  - Update `Scripts/generate_config.py`: hardcoded `Porytiles2/config_templates/` paths and the GENERATED_CONFIG_FILES list (28 paths).
  - Run `uv run Scripts/generate_config.py` to regenerate; verify output matches the renamed sweep results from A2b.
  - **Verify consistency**: the GENERATED_CONFIG_FILES list in `Scripts/generate_config.py` and the corresponding list in `Porytiles/CMakeLists.txt` must stay in lockstep — there is a pre-existing inconsistency around `header_define_provider.cpp` worth fixing while we're in this code.

### A3 — Porytiles1 → Legacy directory rename

After A1 + A2 are stable on develop:

- Rename `Porytiles1/` → `Legacy/`.
- Update root `CMakeLists.txt` `add_subdirectory(Porytiles1)` → `add_subdirectory(Legacy)`.
- Rename CMake variable `PORYTILES1_INCLUDE_DIR` → `PORYTILES_LEGACY_INCLUDE_DIR`.
- Rename CMake targets:
  - `Porytiles1Lib` → `LegacyLib`
  - `Porytiles1Driver` → `LegacyDriver`
  - `Porytiles1Tests` → `LegacyTests`
  - `Porytiles1LibTests` → `LegacyLibTests`
  - `Porytiles1TestSuite` → `LegacyTestSuite`
  - `AllPorytiles1Tests` (CTest test) → `AllLegacyTests`
- In `Legacy/tools/driver/CMakeLists.txt`: `OUTPUT_NAME "porytiles"` → `"porytiles-legacy"`. Add an `install(TARGETS LegacyDriver RUNTIME DESTINATION bin)` rule (legacy currently has no install rule; for the release we need both binaries installable).
- In `Legacy/lib/legacy/cli_parser.cpp`: replace the 37 hardcoded `porytiles` literals in help/usage text with `porytiles-legacy`. Verify with `porytiles-legacy --help`.

### A4 — Scripts, configs, IDE files, docs

Mechanical updates to tracked files outside the source trees:

- **Scripts** (all under `Scripts/`):
  - `generate_config.py` (covered in A2d)
  - `format.py` — `Porytiles2` path reference
  - `new_class.py` — namespace template + include path template
  - `coverage.py` — `Porytiles2 / tests / Porytiles2AllTests` paths
  - `tidy.py` — `Porytiles2/lib`, `Porytiles2/tools` paths
  - `todo.py` — default `--path Porytiles2` and description
- **Config files**:
  - `.clang-tidy` — `HeaderFilterRegex: 'porytiles2/.*'` → `'porytiles/.*'`
  - `pyproject.toml` — description string `"Configuration code generation for Porytiles2"` → `"Porytiles"`
- **IDE files** (tracked):
  - `.vscode/c_cpp_properties.json` — 4 Porytiles2 paths + 2 Porytiles1 paths
  - `.vscode/launch.json` — 4 Porytiles2/porytiles2 refs
  - `.idea/*` (check which files are tracked; update those)
- **Docs**:
  - `CLAUDE.md` — ~30 refs across architecture section, build commands, agent paths, testbed paths
  - `README.md` — Porytiles1 path refs at the legacy-binary sections
  - `STYLE.md` — 7 namespace/include refs (lines 3, 14, 18, 19, 84, 247, 251, 257, 259)
  - `.claude/agents/{architect,build-expert,code-reviewer,debugger}.md`
  - `.claude/skills/fix-includes.md`
  - `Porytiles/ARCHITECTURE.md`, `Porytiles/README.md`, `Legacy/README.md`
  - All 9 files in `Porytiles/Notes/` (now-renamed)

### A5 — GitHub Actions hardcoded paths

Update workflow action files:

- `.github/workflows/build_linux_clang/action.yml`
- `.github/workflows/build_linux_gcc/action.yml`
- `.github/workflows/build_macos_clang/action.yml`
- `.github/workflows/create_release_package/action.yml`
- `.github/workflows/run_test_suite/action.yml`
- `.github/workflows/build_pages.yml` (verify Documentation target paths)
- `.github/workflows/build_jobs_template.yml`
- `.github/workflows/dev_build.yml`, `pr_dev_build.yml`

Replace `Porytiles1` → `Legacy`, `Porytiles2` → `Porytiles`, `porytiles2` (binary refs) → `porytiles`, `./build/Porytiles1/tools/driver/porytiles` → `./build/Legacy/tools/driver/porytiles-legacy`, etc.

### A6 — Phase A verification

Run end-to-end:
- Fresh `rm -rf porytiles-build-debug/`
- `cmake -B porytiles-build-debug -S .`
- `cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1` — check exit code
- `./porytiles-build-debug/Porytiles/tests/PorytilesAllTests > /tmp/test.log 2>&1` — check exit code
- `./porytiles-build-debug/Legacy/tests/LegacyTests > /tmp/legacy_test.log 2>&1` — check exit code
- `cmake --install porytiles-build-debug --prefix ~/.local`
- `~/.local/bin/porytiles --version` — verify outputs `porytiles 1.0.0 <date>` (after Phase C wires the version) or the placeholder pre-C
- `~/.local/bin/porytiles-legacy --version` — verify outputs `porytiles-legacy ...`
- `uv run Scripts/generate_config.py` — verify clean rerun produces no diff

---

## Phase B: CHANGELOG infrastructure

Can land in parallel with Phase A (no file overlap). Best landed early so Phase A PRs each contribute Unreleased entries.

### B1 — Create CHANGELOG.md

At repo root with this skeleton:

```markdown
# Changelog

All notable changes to Porytiles are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com),
simplified to a single flat list of changes per version.

## [Unreleased]

(in-progress changes go here)

## [1.0.0] - YYYY-MM-DD

(populated at release-cut time from accumulated Unreleased items)
```

### B2 — CHANGELOG enforcement CI check

Add `.github/workflows/changelog_check.yml`:
- Trigger: `pull_request: branches: [develop, master]`
- Logic: fail if `CHANGELOG.md` is not in the PR diff
- Opt-out: skip when PR has label `no-changelog` (for typo fixes, CI tweaks, infrastructure-only PRs)

### B3 — Document the convention

Add to `CONTRIBUTING.md` a section stating: every PR to `develop` must add an entry under `## [Unreleased]` in `CHANGELOG.md`, or carry the `no-changelog` label. At release-cut time, `[Unreleased]` items are migrated under a new `## [X.Y.Z] - YYYY-MM-DD` heading.

---

## Phase C: Versioning system

Depends on Phase A2 complete (CMake target names need to match). Independent of Phase B.

### C1 — Single source of truth in root CMakeLists.txt

Change root `CMakeLists.txt` `project(Porytiles CXX)` → `project(Porytiles VERSION 1.0.0 CXX)`.

This sets `CMAKE_PROJECT_VERSION` to `1.0.0` for the entire build. Sub-CMakeLists must read `${CMAKE_PROJECT_VERSION}`, NOT `${PROJECT_VERSION}` (which is shadowed by the sub-`project()` calls). Validate by adding `message(STATUS "PROJECT_VERSION=${PROJECT_VERSION} CMAKE_PROJECT_VERSION=${CMAKE_PROJECT_VERSION}")` to one sub-CMakeLists and confirming.

### C2 — Wire build-version macro

In `Porytiles/lib/CMakeLists.txt` and `Legacy/lib/CMakeLists.txt`, add:

```cmake
target_compile_definitions(PorytilesLib PRIVATE
  PORYTILES_BUILD_VERSION_=${CMAKE_PROJECT_VERSION}
)
```

This makes the default `--version` output reflect the project version when CI doesn't override `-DPORYTILES_BUILD_VERSION_=...`. CI workflows continue to override for snapshot builds (with the SHA-encoded prerelease version) and versioned release builds (with the tag).

### C3 — Consistent `--version` output across binaries

Update both binaries to render `<exec-name> <version> <date>` in a uniform format. Confirm `Porytiles/tools/driver/main.cpp`'s version handler and `Legacy/lib/legacy/cli_parser.cpp`'s `parseGlobalOptions` `VERSION_VAL` handler produce identical formatting. The existing `PORYTILES_EXECUTABLE` macro already differs per binary (from C2 build_version.h defaults), so the format string can be shared.

### C4 — Phase C verification

- Local build (no CI overrides): `porytiles --version` outputs `porytiles 1.0.0 <date>`.
- Local build: `porytiles-legacy --version` outputs `porytiles-legacy 1.0.0 <date>`.
- Build with `-DPORYTILES_BUILD_VERSION_=1.0.0-snapshot.20260524.abc123`: verify both binaries echo the override.

---

## Phase D: CI/Release pipeline overhaul

Depends on Phases A and C complete. Each step here is an isolated workflow file change.

### D1 — Convert `nightly_release.yml` into `snapshot_release.yml`

- Rename file. Update trigger to `on: push: branches: [develop]` (replacing the current manual-dispatch).
- Add concurrency: `concurrency: { group: snapshot-release, cancel-in-progress: false }` to serialize back-to-back develop pushes.
- Drop the existing "check already built" gate (force-replace semantics make it obsolete).
- Pre-build step: `gh release delete snapshot --yes --cleanup-tag || true` then `git push --delete origin snapshot || true`. The `|| true` handles the first-ever run where no snapshot exists.
- Build-version flag: `-DPORYTILES_BUILD_VERSION_=1.0.0-snapshot.$(date -u +%Y%m%d%H%M%S).${GITHUB_SHA::8}` — timestamp ensures monotonic ordering (critical for Homebrew upgrade resolution); short SHA identifies the source; the user can run `porytiles --version` and report the SHA in bug reports. (Full SHA is also acceptable; short SHA keeps the version string readable.)
- Package step (in `create_release_package/action.yml`): bundle BOTH binaries — `porytiles` and `porytiles-legacy` — into each platform zip.
- Release-create step: `softprops/action-gh-release` with `tag_name: snapshot`, `prerelease: true`, `name: "Snapshot (latest develop)"`, body containing date + SHA + the `[Unreleased]` section of `CHANGELOG.md`.
- Homebrew update step: update `porytiles@snapshot.rb` in the tap repo. Set `version "1.0.0-snapshot.YYYYMMDDHHMMSS.shortsha"`, update four `sha256` per platform asset. Tap formula's `install` block must run `bin.install "porytiles", "porytiles-legacy"`. The first time this formula is created in the tap, the install block is written by hand; thereafter only version/sha256 are touched by automation.

### D2 — Create `versioned_release.yml`

- Trigger: `on: push: tags: ['v[0-9]+.[0-9]+.[0-9]+']`.
- Concurrency: `concurrency: { group: versioned-release-${{ github.ref }}, cancel-in-progress: false }`.
- Build-version flag: `-DPORYTILES_BUILD_VERSION_=${GITHUB_REF#refs/tags/v}` (strips the `v` prefix to get `1.0.0`).
- Four-platform build matrix (reuses the existing build_* actions).
- Package step: bundle both binaries (same as snapshot).
- Release-create step: `tag_name: ${{ github.ref_name }}`, `prerelease: false`, `name: "Porytiles ${{ github.ref_name }}"`, body extracted from `CHANGELOG.md` (the section matching the version).
- Homebrew update step: rewrite `porytiles.rb` (NOT `porytiles@snapshot.rb`) in the tap repo. Set `version` to the tag, update `sha256` per platform.
- Do NOT delete prior versioned releases.

### D3 — CHANGELOG-extraction helper

A small action or inline script that, given a version string (`1.0.0` or `Unreleased`), pulls the corresponding section from `CHANGELOG.md` and emits it for use as the release body. Shared by D1 and D2.

### D4 — Audit interaction with existing workflows

- `dev_build.yml` (push to develop): currently does a CI build. After D1, every push to develop fires both `dev_build.yml` AND `snapshot_release.yml`. Decide whether to consolidate (have snapshot_release call dev_build's logic) or accept the duplicate spend. Recommendation: consolidate by deleting `dev_build.yml` and letting snapshot_release be the single develop-push pipeline.
- `pr_dev_build.yml`: keep — PR builds are essential.
- `build_pages.yml`: currently fires on push to develop. Consider scoping to tag pushes (versioned releases) to keep docs in sync with stable releases — the snapshot pipeline shouldn't deploy docs since snapshot changes are too fluid. Open question for user.

### D5 — Phase D verification

Push a test commit to `develop`:
- Confirm `snapshot_release.yml` fires, deletes prior snapshot release, builds four platforms, creates new `snapshot` release with both binaries in each zip, updates `porytiles@snapshot.rb` in tap repo.
- `brew tap grunt-lucas/porytiles && brew install porytiles@snapshot` on macOS: confirm install succeeds, `porytiles --version` and `porytiles-legacy --version` both work.
- Push a `v0.0.0-test` tag (delete after): confirm `versioned_release.yml` fires, creates permanent release. Delete the test tag/release.

---

## Phase E: Gitflow adoption + 1.0.0 cut

Final phase. Happens when Phases A–D are stable on `develop`.

### E1 — Pre-cut: handle open feature branches

Decide each of:
- `bug/anim-tiles`
- `bug/issue-0060/key-frame-bug`
- `feature/issue-0047/compiled-paired-primary`

These touch pre-split file paths and can't be rebased through the rename. For each: confirm if the underlying issue is still live; if yes, hand-port the change onto a new branch off post-rename `develop`; if no, close the branch. This is NOT a release-prep prerequisite — open issues can ship in 1.0.x patches. But the user should at least audit and decide before tagging 1.0.0.

### E2 — Cut `release/1.0.0` from `develop`

- `git checkout develop && git pull`
- `git checkout -b release/1.0.0`
- `git push -u origin release/1.0.0`

On `release/1.0.0`, only bugfixes and release-prep tasks land:
- Verify root `CMakeLists.txt` `project(Porytiles VERSION 1.0.0 ...)` is set.
- In `CHANGELOG.md`: rename `## [Unreleased]` → `## [1.0.0] - <today's date>` and add a fresh empty `## [Unreleased]` above it.
- **Resolve the open design question on 1.0.0 public API surface** (see "Open design question" section near the top of this doc). Produce `STABILITY.md` (or equivalent) classifying each surface as stable / experimental / internal. This is a release-cut blocker — do not tag `v1.0.0` until the stability contract is written down.
- Final smoke-test build/install/`--version` check on macOS and Linux.

### E3 — Tag and merge to master (and back to develop) — main + both docs repos in lockstep

Main repo:
- From `release/1.0.0`: push final commits.
- Create `master` from `release/1.0.0`: `git checkout -b master release/1.0.0 && git push -u origin master`.
- Tag on master: `git tag -a v1.0.0 -m "Porytiles 1.0.0" && git push origin v1.0.0`. This triggers `versioned_release.yml`.
- Merge `release/1.0.0` back to `develop`: `git checkout develop && git merge --no-ff release/1.0.0 && git push`.
- Delete `release/1.0.0` after merges complete.

Both docs repos (`porytiles-user-docs` and `porytiles-dev-docs`), repeated identically per repo:
- On the repo's pre-existing `release/1.0.0` branch (cut earlier per Phase F): push any final doc-fix commits.
- Create `master` from `release/1.0.0` (or merge into existing master if Phase F created it earlier).
- Tag `v1.0.0` on master and push the tag. This triggers the docs-repo GH Pages deploy workflow → public site updates to 1.0.0 docs.
- Merge `release/1.0.0` back to `develop`.
- Delete the docs `release/1.0.0` branch.

Verify after all three repos are tagged: `grunt-lucas.github.io/porytiles-user-docs/` displays "Porytiles 1.0.0 documentation" prominently (per Phase F2's version-display config).

### E4 — Configure branch + tag protection

- `master`: require PR review, no direct pushes, restrict pushers.
- `develop`: require PR (optional review for solo workflow), no direct pushes once gitflow is established.
- Tag protection rule for `v[0-9]+.[0-9]+.[0-9]+` so only maintainers can push release tags.

### E5 — Document gitflow conventions

In `CONTRIBUTING.md`:
- Branch model diagram.
- How to start a feature (`feature/<topic>` off develop).
- How a release is cut (`release/<version>` off develop, merged to both master and develop, tagged on master).
- How a hotfix is cut (`hotfix/<version>` off master, merged to both, tagged on master).
- Snapshot vs versioned release lifecycle.

### E6 — Announce / housekeeping

- Update README.md install section to reference Homebrew tap commands and direct-download links.
- Confirm GitHub repo default branch remains `develop` (gitflow expects feature work integrates there).
- Optionally: archive or close pre-1.0 `0.0.x` tag releases on GitHub (leave tags in git history; just collapse the GitHub UI presence).
- Notify any downstream consumers (Discord, decomp forums) of the binary rename and 1.0.0 availability.

---

## Phase F: Documentation repos gitflow alignment

Applies to BOTH `porytiles-user-docs` and `porytiles-dev-docs` repos (separate from the main repo, cloned locally at `porytiles-user-docs/` and `porytiles-dev-docs/`). Can land in parallel with Phases A–D; F's release-tagging step is performed at Phase E3.

Per CLAUDE.md these are separate git repositories, not submodules. All Phase F changes must be committed and pushed in those repos, not the main porytiles repo.

### F1 — Adopt gitflow branches in each docs repo

For each of the two docs repos:
- Inspect current branch layout: most likely `main` exists as the only branch. If `develop` does not exist, create it from `main` HEAD (`git checkout -b develop && git push -u origin develop`). Set `develop` as the new default branch in GitHub repo settings.
- Do NOT create `master` yet — like the main repo, `master` is created at the first release cut (Phase E3). If `main` exists, decide: rename to `master` now (preserves history, simplest), OR leave `main` and create `master` at cut time (slightly more confusing). Recommendation: rename `main` → `master` now to match main repo's conventions, then immediately re-establish `develop` as default-branch.
- All work post-Phase-F1 lands on `develop` via PRs; release/hotfix branches follow same pattern as main repo.

### F2 — Parameterize Sphinx version display

In each docs repo's `docsrc/conf.py`:
- Set `version` and `release` from a single source. Two options to decide between:
  - **Option A (recommended for simplicity)**: read from a tracked `VERSION` file at the repo root. Each release cut bumps the file alongside other release-prep tasks. Bulletproof, no CI checkout-depth concerns.
  - **Option B (DRY but fragile)**: derive from `git describe --tags`. Requires CI to use `fetch-depth: 0` on checkout. Risk: shallow clones in some contributor workflows produce empty output.
- Confirm the rendered theme (Read the Docs theme per CLAUDE.md) displays the version prominently — most RTD themes already show `version` in the sidebar/header. If not visible, add a custom banner via `html_theme_options` or a template override.
- The display should read e.g. "Porytiles 1.0.0 documentation" on stable, "Porytiles 1.1.0-dev documentation" on develop (or similar; choose the snapshot-version string convention to match the main repo's `1.0.0-snapshot.YYYYMMDD.<sha>` pattern as closely as Sphinx allows).

### F3 — GitHub Pages deploy from `master` only (all three GH Pages sites)

**For each Sphinx docs repo** (`porytiles-user-docs`, `porytiles-dev-docs`):
- Configure GH Pages source to be the `master` branch's `/docs` folder (or use the GH Actions deploy-pages flow — current setup per CLAUDE.md uses `make github` which writes to `docs/`).
- Add a GH Actions workflow that runs on `push: branches: [master]`: checks out master, runs `cd docsrc && uv run make html`, commits the generated `docs/` if changed, pushes. This automates what currently happens manually per the CLAUDE.md workflow ("After running `make github`, commit and push the changes").
- Optionally: a `pull_request: branches: [develop, master]` workflow that builds Sphinx (without deploying) so PRs catch RST/MyST syntax errors before merge.
- Explicitly do NOT auto-deploy from `develop` — the public GH Pages site must only ever show stable docs.

**For the main repo's Doxygen GH Pages** (`grunt-lucas.github.io/porytiles/`):
- Rewrite `.github/workflows/build_pages.yml` trigger from `push: branches: [develop]` to `push: branches: [master]`.
- Verify the rest of the workflow (build target paths, deploy step) continues to work after the Phase A rename — paths like `./build/Documentation/doxygen/html` need to be confirmed against the post-rename CMake layout.
- Add the same optional PR-build workflow if catching Doxygen warnings in PRs is valuable. Doxygen failures are usually less impactful than Sphinx ones (the source code itself doesn't break), but a broken Doxyfile or missing-file reference is still worth catching.
- Like the Sphinx repos, snapshot Doxygen viewing is local-only: developers run `cmake --build porytiles-build-debug --target doxygen` and open the generated HTML directly. Document this in `CLAUDE.md`.

### F4 — Local-only viewing instructions

In each docs repo's `README.md`, add a section like:

```markdown
## Viewing docs for older versions or the in-progress snapshot

The public site at <https://grunt-lucas.github.io/porytiles-user-docs/> always shows
the latest stable release's docs. To view docs for a different version:

1. Clone this repo: `git clone https://github.com/grunt-lucas/porytiles-user-docs.git`
2. Check out the desired version:
   - Older stable: `git checkout v0.9.0` (any released tag)
   - Snapshot / in-progress: `git checkout develop`
3. Build locally: `cd docsrc && uv run make html`
4. Open `docsrc/_build/html/index.html` in your browser.
```

Mirror the same block in `porytiles-dev-docs/README.md`.

### F5 — Docs/code coordination convention

In the main repo's `CONTRIBUTING.md` (created/updated as part of Phase B3/E5):
- Add a section stating that any PR landing on the main repo's `develop` whose change is user-visible (CLI flag, config key, output format, etc.) requires a corresponding docs PR on the matching docs repo's `develop` branch before merge. Internal-only changes that affect dev docs (architecture, contributor guides) require a corresponding `porytiles-dev-docs` PR.
- No tooling enforcement at first — PR-template prompt for "linked docs PR (if any): ___" is sufficient. Revisit if drift becomes a problem.
- During the rename in Phase A, both docs repos likely contain stale references to `porytiles2`, `Porytiles2/...` paths, the binary name `porytiles2`, etc. Sweep both repos as part of Phase A4's doc-update work — but commit those changes to the docs repos, NOT the main repo.

### F6 — Branch protection on docs repos

Mirror the main repo's protection (per Phase E4):
- `master`: require PR, restrict pushers, no direct pushes.
- `develop`: require PR, no direct pushes once gitflow is established.
- Tag protection rule for `v[0-9]+.[0-9]+.[0-9]+`.

### F7 — Initial sync: bring docs repos to a buildable, content-accurate `develop`

Before the first 1.0.0 cut, both docs repos' `develop` branches must reflect the post-rename state of the main repo:
- Update all references from `Porytiles2` / `porytiles2` to `Porytiles` / `porytiles`.
- Update legacy references from `Porytiles1` / the legacy `porytiles` binary to `Legacy` / `porytiles-legacy`.
- Update CLI examples that show `porytiles2 ...` invocations to `porytiles ...`.
- Update install instructions to reference Homebrew tap (`brew install grunt-lucas/porytiles/porytiles` for stable, `porytiles@snapshot` for snapshot).
- Update any architecture diagrams in `porytiles-dev-docs` that reference the old directory structure.

### F8 — Phase F verification

Per docs repo:
- `cd docsrc && uv run make html` succeeds on `develop`.
- `cd docsrc && uv run make html` succeeds on `master` (after first release).
- Pushing to `master` triggers the new GH Pages workflow; the public site updates within minutes.
- Pushing to `develop` does NOT update the public site.
- A user can clone the repo, `git checkout v1.0.0`, and build the 1.0.0 docs locally.
- The rendered HTML shows the correct version string in the header/sidebar.

---

## Phase G: AI policy documentation

Can land in parallel with Phases A–F. Best landed early so the first PRs that use AI assistance (including the ones writing this very plan) operate under a written bar. Single small file at repo root, no code impact.

### G1 — Draft `AI-POLICY.md`

Write at repo root following the contours from the "AI contribution policy" section above. The file should cover:

- The two-track stance: Legacy is AI-free; Porytiles is AI-OK-with-slop-rejection.
- Maintainer discretion as the enforcement mechanism, stated plainly so contributors aren't surprised.
- The slop-tell heuristics from the planning section, written as concrete examples a contributor can self-check before opening a PR.
- A pointer to `STYLE.md` and `CONTRIBUTING.md` as the implicit code-quality gate — any AI-assisted contribution that doesn't already satisfy those is slop by definition.
- The explicit "no disclosure required" stance, so contributors aren't forced to self-tag.
- A "revisit annually" line so the policy doesn't silently drift out of relevance.

Keep the document short — one screen if possible. A long AI policy is itself an AI-tell.

### G2 — Reference from `CONTRIBUTING.md` and `README.md`

- `CONTRIBUTING.md` (created/expanded in Phase B3 / E5): add a one-line "AI-assisted contributions: see `AI-POLICY.md`" near the top, before the changelog and gitflow rules.
- `README.md`: optionally surface the legacy-is-AI-free aspect in the legacy-binary section, since that's where a contributor considering a Legacy fix would land first.

### G3 — Phase G verification

- `AI-POLICY.md` exists at repo root; references from `CONTRIBUTING.md` resolve.
- Maintainer reads the document end-to-end and confirms voice matches the project — the policy is the first thing it will be measured against.
- Document is short and concrete, not itself slop.

---

## Cross-cutting risks to call out before starting

- **Local build caches**: After any of A1, A2, A3 lands, every developer (including the user) must `rm -rf porytiles-build-*` before rebuilding — stale CMake caches reference old target names and will fail or relink incorrectly.
- **Deprecated binary name `porytiles2`**: Existing user shell history, scripts, and the user's testbed paths (`./pokeemerald-expansion/porytiles2`, `.claude/settings.local.json` references to `~/.local/bin/porytiles2`) will break. Decide: install a `porytiles2 → porytiles` symlink for a deprecation window, OR document as a breaking change in the 1.0.0 release notes.
- **Pre-1.0 nightly release**: The existing `nightly-3a9d31c...` GitHub release and tag are harmless to keep. New `snapshot_release.yml` only deletes the `snapshot` tag, not `nightly-*` tags.
- **Shell completion scripts**: `Porytiles/tools/driver/command_completion.hpp` generates bash/zsh/fish completions. Confirm completion code uses `argv[0]` at runtime (so it self-rebrands) and not a hardcoded `porytiles2` literal.
- **Homebrew version ordering**: Brew compares versions via its `Version#<=>` operator, NOT semver. Bare commit SHAs would sort lexicographically and could skip upgrades. The timestamp prefix `YYYYMMDDHHMMSS.<sha>` guarantees monotonic ordering — keep this pattern.
- **Workflow race on first 1.0.0 cut**: When `release/1.0.0` is merged into `master` and the same merge is also merged back to `develop`, both workflows (snapshot from develop-push, versioned from tag-push) may fire close together. With distinct concurrency groups (`snapshot-release` vs `versioned-release-v1.0.0`) they don't serialize against each other, but they produce different releases (`snapshot` vs `v1.0.0`) and don't conflict on output.
- **Three-repo lockstep at release time**: Phase E3 tags three repos in succession (main, user-docs, dev-docs). If a tag push succeeds on one repo and fails on another (network glitch, missing permissions, etc.), the published state is inconsistent — e.g., GH Pages shows 1.0.0 user docs but `v1.0.0` doesn't exist in dev-docs. Mitigation: do the tags in a strict order with verification between each (tag main first, verify versioned_release.yml fires, THEN tag docs repos). At minimum, have a checklist for E3 that runs explicit `gh release view v1.0.0` and equivalent checks on the docs repos after pushing tags.
- **Docs/code drift between releases**: between snapshot deploys and release cuts, docs `develop` can drift from code `develop` because docs PRs are easy to defer. The Phase F5 convention (linked docs PR per user-visible main PR) is convention-only at first — if drift becomes painful, add a `docs-needed` label workflow that flags main PRs lacking a linked docs PR. Don't pre-build this; wait for the pain.
- **Main repo's Doxygen GH Pages site is separate**: the existing `.github/workflows/build_pages.yml` in the main repo deploys Doxygen-generated API docs to `grunt-lucas.github.io/porytiles/` — that's the main-repo's GH Pages site, distinct from the two Sphinx docs repos. Per locked-in decision, it adopts the same stable-only model (trigger rewrites to `master` push). All three GH Pages sites follow identical conventions, simplifying mental model.

---

## Verification (end-to-end after all phases)

1. Clean checkout, fresh build: produces `porytiles` and `porytiles-legacy` binaries; `--version` reports `1.0.0` plus build date.
2. `cmake --install` writes both binaries to `<prefix>/bin/`.
3. Push to develop: snapshot release exists at exactly one tag (`snapshot`); zips contain both binaries; Homebrew snapshot formula re-pulls on subsequent develop push.
4. Push `v1.0.1` tag to master: versioned release created; existing `v1.0.0` release untouched; Homebrew stable formula re-pulls.
5. `brew install grunt-lucas/porytiles/porytiles` installs stable; `brew install grunt-lucas/porytiles/porytiles@snapshot` installs snapshot; both provide both binaries.
6. CHANGELOG.md is non-empty and has a date-stamped 1.0.0 section.
7. Branch protection rejects direct push to master.
8. CHANGELOG enforcement workflow blocks a PR that doesn't touch CHANGELOG.md (unless `no-changelog` label is applied).
9. `AI-POLICY.md` exists at repo root; `CONTRIBUTING.md` links to it; legacy AI-free stance is stated unambiguously.

---

## Critical files (full paths)

**Top-level**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/CMakeLists.txt` — root project version + subdirectory ordering
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/CHANGELOG.md` — new file (Phase B1)
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/RELEASE_PREP_1_0_0.md` — new file (Phase 0)
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/AI-POLICY.md` — new file (Phase G)
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/CONTRIBUTING.md` — gitflow + changelog convention docs
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/README.md` — install/usage rewrite
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/CLAUDE.md` — heavy path/name updates
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/STYLE.md` — include path + namespace examples
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/pyproject.toml` — description string

**Active codebase (Porytiles2 → Porytiles)**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/lib/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/tools/driver/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/tests/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/include/porytiles2/build_version.h`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/tools/driver/main.cpp`
- 29 `.jinja2` templates under `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/config_templates/`
- 391 source files under `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles2/` (sweep target)

**Legacy codebase (Porytiles1 → Legacy)**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/lib/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/tools/driver/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/tests/CMakeLists.txt`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/include/porytiles/build_version.h`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/lib/legacy/cli_parser.cpp` — help-text literals
- 55 source files under `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Porytiles1/`

**Documentation build**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Documentation/CMakeLists.txt`

**Scripts**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Scripts/generate_config.py`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Scripts/format.py`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Scripts/new_class.py`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Scripts/coverage.py`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Scripts/tidy.py`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/Scripts/todo.py`

**Config / IDE**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.clang-tidy`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.vscode/c_cpp_properties.json`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.vscode/launch.json`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.claude/agents/*.md`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.claude/skills/fix-includes.md`

**CI / release**:
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/nightly_release.yml` — rename + rewrite to `snapshot_release.yml`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/versioned_release.yml` — new file
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/changelog_check.yml` — new file
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/build_{linux_clang,linux_gcc,macos_clang}/action.yml`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/create_release_package/action.yml`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/run_test_suite/action.yml`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/build_jobs_template.yml`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/dev_build.yml`, `pr_dev_build.yml`
- `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/.github/workflows/build_pages.yml`

**Homebrew tap** (separate repo at `grunt-lucas/homebrew-porytiles`, cloned locally at `/Users/lucas/Projects/github.com/grunt-lucas/porytiles/homebrew-porytiles/`):
- `Formula/porytiles.rb` — rework install block for both binaries; updated by `versioned_release.yml`
- `Formula/porytiles@snapshot.rb` — new file, similar structure; updated by `snapshot_release.yml`
