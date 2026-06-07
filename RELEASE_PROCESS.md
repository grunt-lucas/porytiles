# Porytiles Release Process

- [Porytiles Release Process](#porytiles-release-process)
  - [Mental model](#mental-model)
    - [What is automated vs. what you do by hand](#what-is-automated-vs-what-you-do-by-hand)
    - [Execution legend](#execution-legend)
  - [Conventions and invariants](#conventions-and-invariants)
  - [Regular versioned release](#regular-versioned-release)
    - [0 — Pre-flight `[auto]`](#0--pre-flight-auto)
    - [1 — Create the release branch `[auto]`](#1--create-the-release-branch-auto)
    - [2 — Release-branch edits `[auto]`](#2--release-branch-edits-auto)
    - [3 — Local smoke test `[auto]`](#3--local-smoke-test-auto)
    - [4 — Merge the release branch into `master` `[confirm]`](#4--merge-the-release-branch-into-master-confirm)
    - [5 — Tag `master` and push the tag `[confirm]`](#5--tag-master-and-push-the-tag-confirm)
    - [6 — Watch CI and verify `[auto]`](#6--watch-ci-and-verify-auto)
    - [7 — Merge the release branch back into `develop` `[confirm]`](#7--merge-the-release-branch-back-into-develop-confirm)
    - [8 — Docs repos lockstep `[confirm]`](#8--docs-repos-lockstep-confirm)
    - [9 — Post-release verification `[auto]`](#9--post-release-verification-auto)
  - [Hotfix release](#hotfix-release)
    - [1 — Cut the hotfix branch from `master` `[auto]`](#1--cut-the-hotfix-branch-from-master-auto)
    - [2 — Fix, bump, changelog `[auto]`](#2--fix-bump-changelog-auto)
    - [3 — Merge to `master`, tag, verify `[confirm]`](#3--merge-to-master-tag-verify-confirm)
    - [4 — Merge back to `develop` `[confirm]`](#4--merge-back-to-develop-confirm)
  - [Docs hotfix between releases](#docs-hotfix-between-releases)
  - [Verification and rollback](#verification-and-rollback)
  - [Quick reference](#quick-reference)
  - [Appendix: stable Homebrew formula shape](#appendix-stable-homebrew-formula-shape)

This is the operational runbook for executing a Porytiles release.
It is the *recurring* process used for every release after 1.0.0
(`1.0.1`, `1.1.0`, `2.0.0`, and so on).

---

## Mental model

A release touches **three git repositories** that are tagged in lockstep:

| Repo                              | Role                                           | Local path             |
|-----------------------------------|------------------------------------------------|------------------------|
| `grunt-lucas/porytiles`           | The compiler. Source of truth for the version. | `.`                    |
| `grunt-lucas/porytiles-user-docs` | User-facing Sphinx docs site.                  | `porytiles-user-docs/` |
| `grunt-lucas/porytiles-dev-docs`  | Developer Sphinx docs site.                    | `porytiles-dev-docs/`  |

A fourth repo, the Homebrew tap `grunt-lucas/homebrew-porytiles`
(`homebrew-porytiles/`), is **updated by CI**, not tagged.
You generally do not touch it by hand during a release.

All four follow the gitflow branching model originally described in
Vincent Driessen's [A successful Git branching model](https://nvie.com/posts/a-successful-git-branching-model/), the definitive reference:

<img src="resources/release_process/git_model.png" alt="Gitflow branching model (image from nvie.com)" width="700">

- **`develop`** is integration. Every push to `develop` rebuilds the rolling
  `snapshot` prerelease and the `porytiles@snapshot` Homebrew formula.
- **`master`** is production. A push of a `vX.Y.Z` tag triggers the permanent
  versioned release and updates the `porytiles` Homebrew formula.
- **`release/<v>`** is branched from `develop` for stabilization, then merged into
  **both** `master` and `develop`.
- **`hotfix/*`** is branched from `master` for urgent patches, then propagates
  to **both** `master` and `develop` via merge (see
  [Hotfix release](#hotfix-release) and
  [Docs hotfix between releases](#docs-hotfix-between-releases)).

### What is automated vs. what you do by hand

When you push a `vX.Y.Z` tag,
`.github/workflows/versioned_release.yml` does all of the mechanical work with no further input:

1. Validates the tag against the `VERSION` file (hard-fails on mismatch).
2. Builds `linux-amd64`, `linux-arm64`, and `macos-arm64`.
3. Runs the test suite on each platform.
4. Publishes the GitHub release with notes pulled from `CHANGELOG.md`.
5. Rewrites `Formula/porytiles.rb` in the tap and pushes it.
6. Smoke-tests `brew install porytiles` on Linux and macOS.

Your job is to execute the **gitflow mechanics and the three-repo lockstep** that
leads up to the tag, plus verifying the automated steps succeeded.
This runbook is almost entirely about those mechanics.

### Execution legend

Steps are marked so this runbook can be followed by hand or driven by an agent:

- **`[auto]`** Safe to run unattended (local commands, builds, read-only checks).
- **`[confirm]`** Outward-facing or hard to reverse. Review before running:
  pushing to `master`, pushing a tag (this publishes a release and updates
  Homebrew), tagging the docs repos, deleting branches.

---

## Conventions and invariants

**Version string.** The release version is `X.Y.Z` (semver). It lives in the
repo-root [`VERSION`](./VERSION) file, which is the single source of truth.
CMake reads it (`project(Porytiles VERSION ...)`), and CI reads it to validate
tags. The binary stamps it into `--version` output as `porytiles X.Y.Z <date>`.

**Tag format.** Release tags are `vX.Y.Z` (leading `v`, no suffix). The CI
trigger is the literal pattern `v[0-9]+.[0-9]+.[0-9]+`. Consequences:

- A pre-release tag like `v1.1.0-rc.1` does **not** match and will **not**
  trigger the pipeline. Plain `vX.Y.Z` only.
- The trigger keys off the **tag pattern**, not the branch. CI fires for any
  matching tag on any commit. Tagging on `master` is a gitflow and
  branch-protection convention, not a CI gate. The real gate is the next item.

**The VERSION-must-match-tag invariant.** `versioned_release.yml`'s `prepare`
job runs `cat VERSION` and compares it to the tag with the `v` stripped. If they
differ, the whole workflow aborts before building anything. Therefore the
`VERSION` bump must be committed and merged to the tagged commit **before** the
tag is pushed. Bump first, tag second. This ordering is critical.

**Branch invariants.** Three load-bearing rules that the runbooks below all exist to satisfy:

1. **`master` is merge-only.** No commit lands on `master` except as part of a PR-merged commit from a `release/<v>` branch (regular release) or `hotfix/*` branch (urgent patch). In the main repo a hotfix bumps the patch version and produces a new `vX.Y.Z+1` tag (see [Hotfix release](#hotfix-release)); in the docs repos a hotfix is content-only and does not bump the version or mint a new tag — changes accumulate on docs `master` under the current docs version until the next release cut (see [Docs hotfix between releases](#docs-hotfix-between-releases)). Direct pushes to `master` are blocked by branch protection. Even a one-line CHANGELOG date fix goes through one of these branches.

2. **Every `release/<v>` and `hotfix/*` branch propagates to both `master` AND `develop`.** This happens via merge-back in every repo (see [Regular versioned release](#regular-versioned-release), [Hotfix release](#hotfix-release), and [Docs hotfix between releases](#docs-hotfix-between-releases)). The change must reach develop in some form so the next release doesn't silently regress it.

3. **`develop` is the integration trunk for new work.** All `feature/*`, `bug/*`, and similar branches target `develop` via PR. New work does not go directly to `master`, on a `release/<v>` branch, or on a `hotfix/*` branch (unless it's a genuine hotfix, and is merged back as outlined above).

**Choosing the version number.** Walk the `[Unreleased]` section of
`CHANGELOG.md` and ask, for each entry: does this change something users are
depending on?

- **Patch** (`Z`): refactors; backwards-compatible fixes; bug fixes; no user-visible
  behavior change beyond the fix itself.
- **Minor** (`Y`): new functionality; new CLI flags or YAML keys; renames or
  schema evolution that ship *alongside* a transitional alias (the old shape
  keeps working); deprecation announcements.
- **Major** (`X`): breaking changes to CLI invocations, YAML configuration
  keys, output file formats, or other user-visible commitments that have
  shipped in a prior release without an alias bridge. Also: raising minimum
  compiler/CMake versions in a way that strands current users.

When in doubt, prefer the higher bump.
The CHANGELOG entry for the change is the written record of the rationale.
There are no official stability criteria to consult.

**What each pipeline produces.**

| Trigger           | Workflow                | GitHub release                                 | Homebrew formula        |
|-------------------|-------------------------|------------------------------------------------|-------------------------|
| push to `develop` | `snapshot_release.yml`  | rolling `snapshot` prerelease (force-replaced) | `porytiles-snapshot.rb` |
| push tag `vX.Y.Z` | `versioned_release.yml` | permanent `vX.Y.Z` release, marked latest      | `porytiles.rb`          |

Snapshot version strings are `X.Y.Z-snapshot.<UTCYYYYMMDDHHMMSS>.<short8sha>`.
Build dates are UTC in dotted form: `%Y.%m.%dT%H:%M:%S+00:00`.

---

## Regular versioned release

Use this for a planned release of accumulated work on `develop`
(a patch, minor, or major).
Throughout, replace `X.Y.Z` with the target version (example: `1.1.0`).

### 0 — Pre-flight `[auto]`

```bash
# Be on an up-to-date develop.
git -C . checkout develop && git pull

# Confirm the latest develop snapshot pipeline is green on GitHub
# (the code you are about to release is what snapshot just built).
gh run list --workflow snapshot_release.yml --branch develop --limit 1
```

Decide the version number (see [Choosing the version number](#conventions-and-invariants)).
Skim the `## [Unreleased]` section of [`CHANGELOG.md`](./CHANGELOG.md)
and confirm it reflects everything shipping in this release.

### 1 — Create the release branch `[auto]`

```bash
git checkout -b release/X.Y.Z
git push -u origin release/X.Y.Z
```

Only bugfixes and release-prep edits go on `release/X.Y.Z` from here.
New features wait for the next cycle on `develop`.

### 2 — Release-branch edits `[auto]`

All of these are commits on `release/X.Y.Z`.

1. **Bump `VERSION`.**

   ```bash
   echo "X.Y.Z" > VERSION
   ```

2. **Migrate the CHANGELOG.** Rename `## [Unreleased]` to
   `## [X.Y.Z] - <today, YYYY-MM-DD>` and insert a fresh empty `## [Unreleased]`
   above it. The result looks like:

   ```markdown
   ## [Unreleased]

   ## [X.Y.Z] - 2026-06-15

   (the items that were under Unreleased)
   ```

   The release-notes extractor matches the bracketed version exactly, so the
   heading must read `## [X.Y.Z]` (the ` - DATE` suffix is ignored by the
   matcher but kept for humans). If you skip this rename, the published release
   body falls back to "_No CHANGELOG entry found_".

3. **Resolve any release blockers.** Land any final release-only bugfixes here.

4. Commit each logical change.

### 3 — Local smoke test `[auto]`

```bash
rm -rf porytiles-build-debug
cmake -B porytiles-build-debug -S .
cmake --build porytiles-build-debug -j7 > /tmp/build.log 2>&1            # check exit code
./porytiles-build-debug/porytiles/tests/PorytilesAllTests > /tmp/test.log 2>&1        # check exit code
./porytiles-build-debug/legacy/tests/LegacyTests > /tmp/legacy_test.log 2>&1          # check exit code
cmake --install porytiles-build-debug --prefix ~/.local
~/.local/bin/porytiles --version          # expect: porytiles X.Y.Z <date>
~/.local/bin/porytiles-legacy --version   # expect: porytiles-legacy X.Y.Z <date>
```

Both `--version` lines must show `X.Y.Z` (proving the `VERSION` bump flows
through CMake). If they still show the old version, the bump did not take; stop
and fix before tagging.

### 4 — Merge the release branch into `master` `[confirm]`

```bash
gh pr create --base master --head release/X.Y.Z \
  --title "Release X.Y.Z" --body "Release of X.Y.Z. See CHANGELOG.md."
# Review, then merge with a merge commit (keep history; do not squash a release).
gh pr merge --merge
```

> Branch protection requires this to go through a PR. For a solo workflow you
> still open and merge the PR rather than pushing to `master` directly.

### 5 — Tag `master` and push the tag `[confirm]`

This is the point of no return. Pushing the tag publishes a permanent release
and rewrites the public Homebrew formula.

```bash
git checkout master && git pull
cat VERSION                  # must read exactly X.Y.Z, or CI aborts at the prepare step

git tag -a vX.Y.Z -m "Porytiles X.Y.Z"
git push origin vX.Y.Z
```

### 6 — Watch CI and verify `[auto]`

```bash
gh run watch --workflow versioned_release.yml
```

Confirm, in order:

- `prepare` passed (VERSION matched the tag).
- All three `build` matrix legs passed.
- `publish` created the `vX.Y.Z` release with three `porytiles-<arch>.zip`
  assets, marked as latest.
- `update-homebrew-tap` committed a new `porytiles.rb` to the tap.
- Both `test-brew-tap-*` legs passed.

```bash
gh release view vX.Y.Z          # release exists, correct assets, notes from CHANGELOG
```

### 7 — Merge the release branch back into `develop` `[confirm]`

`master` now has the version bump and CHANGELOG migration; `develop` does not
until you merge back. Skipping this regresses the version on the next cycle.

```bash
git checkout develop && git pull
git merge --no-ff release/X.Y.Z
git push
```

Resolve CHANGELOG conflicts by keeping `develop`'s new empty `## [Unreleased]`
above the dated `## [X.Y.Z]` section.

```bash
git push origin --delete release/X.Y.Z   # [confirm] delete the release branch
git branch -d release/X.Y.Z
```

### 8 — Docs repos lockstep `[confirm]`

Tag the **main repo first** (done above) and verify its release succeeded
**before** tagging the docs repos. This ordering means a mid-sequence failure
never leaves the docs site advertising a version the compiler release lacks.

Repeat identically for `porytiles-user-docs/` and then `porytiles-dev-docs/`:

```bash
cd porytiles-user-docs
git checkout develop && git pull
git checkout -b release/X.Y.Z
git push -u origin release/X.Y.Z

echo "X.Y.Z" > VERSION                       # conf.py reads VERSION via pathlib (Phase F2)
git commit -am "Bump VERSION to X.Y.Z"
git push

gh pr create --base master --head release/X.Y.Z \
  --title "Release X.Y.Z" --body "Docs cut of X.Y.Z."
gh pr merge --merge                          # push to master triggers the GH Pages deploy (Phase F3)

git checkout master && git pull
git tag -a vX.Y.Z -m "Porytiles docs X.Y.Z" # immutable archival snapshot; does not itself deploy
git push origin vX.Y.Z

git checkout develop && git merge --no-ff release/X.Y.Z && git push
git push origin --delete release/X.Y.Z
cd ..
```

### 9 — Post-release verification `[auto]`

```bash
# Stable Homebrew install pulls the new version and both binaries.
brew update
brew install grunt-lucas/porytiles/porytiles
porytiles --version            # porytiles X.Y.Z <date>
porytiles-legacy --version     # porytiles-legacy X.Y.Z <date>
```

- The GitHub release `vX.Y.Z` is marked **Latest**; prior releases are untouched.
- Both docs sites show "Porytiles X.Y.Z documentation".
- A fresh push to `develop` still produces a working `snapshot` (the two
  pipelines do not conflict; they target different tags and formulas).

---

## Hotfix release

Use this for an urgent fix to a **published** release when `develop` has moved
on and you cannot wait for the next regular cut. A hotfix is almost always a
patch bump (`X.Y.Z` → `X.Y.Z+1`).

### 1 — Cut the hotfix branch from `master` `[auto]`

```bash
git checkout master && git pull
git checkout -b hotfix/X.Y.Z+1
git push -u origin hotfix/X.Y.Z+1
```

The defining difference from a regular release: you branch from `master`, not
`develop`, so the fix sits on top of the released code with none of develop's
unreleased work.

### 2 — Fix, bump, changelog `[auto]`

1. Make the minimal fix.
2. `echo "X.Y.Z+1" > VERSION`.
3. Add a new `## [X.Y.Z+1] - <date>` section to `CHANGELOG.md` describing the
   fix. (On `master` the `## [Unreleased]` section is empty, so you are adding a
   fresh dated section, not migrating an accumulated one.)
4. Bump the docs version if the fix is user-visible.
5. Run the local smoke test (step 3 of [Regular versioned release](#regular-versioned-release)).

### 3 — Merge to `master`, tag, verify `[confirm]`

Same as steps 4–6 of [Regular versioned release](#regular-versioned-release), using `hotfix/X.Y.Z+1` and tag `vX.Y.Z+1`.

### 4 — Merge back to `develop` `[confirm]`

This is the easy-to-forget step that defines a hotfix: the fix exists only on
`master` until you bring it back.

```bash
git checkout develop && git pull
git merge --no-ff hotfix/X.Y.Z+1
# CHANGELOG conflict resolution: keep develop's accumulating [Unreleased] AND
# the new dated [X.Y.Z+1] section. VERSION should merge cleanly (develop is
# never bumped independently so the hotfix's value wins).
git push
git push origin --delete hotfix/X.Y.Z+1
```

If the fix is also relevant to the docs, mirror the hotfix in the docs repos
with the same branch-from-master-merge-to-both pattern.

---

## Docs hotfix between releases

Use this for any change to docs `master` between release tags: error
corrections, expansions of placeholder content, additions, or any other
improvement. Same gitflow pattern as [Hotfix release](#hotfix-release) with two differences:
**no version bump**, and **no new tag**. Docs tags are immutable archival
snapshots of what shipped at each release cut; docs `master` is a rolling
head that GitHub Pages serves continuously and accumulates hotfix commits
until the next release.

Docs hotfix branches use the form `hotfix/<short-description>` (e.g.,
`hotfix/fix-install-typo`, `hotfix/fill-in-cli-reference`), since there is
no version number to suffix.
This is the one structural difference from Hotfix release's `hotfix/X.Y.Z+1` naming.

```bash
cd porytiles-user-docs            # or porytiles-dev-docs
git checkout master && git pull
git checkout -b hotfix/<short-description>
# make the change
git commit -am "<description of change>"
git push -u origin hotfix/<short-description>

gh pr create --base master --head hotfix/<short-description> \
  --title "Hotfix: <short description>" --body "..."
gh pr merge --merge  # [confirm]
# GH Pages rebuilds within minutes; the public site updates.

# Carry the same change forward so the next release does not regress it.
git checkout develop && git pull
git merge --no-ff hotfix/<short-description>
git push  # [confirm]
git push origin --delete hotfix/<short-description>
```

Do **not** create a new tag (`vX.Y.Z.1`, `vX.Y.Z-doc.1`)
and do **not** move the existing `vX.Y.Z` tag.
In the docs repos, the tag is simply a record of what docs shipped at `vX.Y.Z`.
The git commit is the primary audit trail.
Ensure commit messages are descriptive.

---

## Verification and rollback

**The versioned-release workflow aborted at `prepare`.** The tag did not match
the `VERSION` file. This is the guard working. Fix the `VERSION` file on
`master`, delete the bad tag, and re-tag:

```bash
git push origin --delete vX.Y.Z       # [confirm] remove the failed tag
# correct VERSION on master via PR, then re-tag the new master HEAD
git tag -a vX.Y.Z -m "Porytiles X.Y.Z" && git push origin vX.Y.Z   # [confirm]
```

This is safe only because the failed run published nothing (it aborts before the
`build` and `publish` jobs).

**Partial three-repo lockstep.** A tag pushed to one repo but not another (for
example main tagged, dev-docs not). Because the docs lockstep step tags
main first and verifies before the docs repos, the common failure is a
missing docs tag. Just complete
the remaining docs tag(s); nothing needs undoing.

**The Homebrew formula did not update or installs the wrong thing.** Check the
`update-homebrew-tap` job log. The most likely cause is `porytiles.rb` not being
in the expected shape (see Prerequisites and the Appendix). The workflow's `sed`
only rewrites the `version` line and the three `sha256` lines; if the URL does
not derive from `version` or a binary is missing from the `install` block, the
formula is structurally wrong and must be hand-corrected in the tap, then the
job re-run.

**A bad release shipped.** Strongly prefer rolling **forward** with a hotfix (`X.Y.Z+1`)
over rewriting history. If you must pull the release, mark it as a draft or
delete it in the GitHub UI and revert the tap formula commit, but understand
that anyone who already ran `brew install` has the bad build. A forward fix is
almost always cleaner than a recall.

---

## Quick reference

The common case (regular release), condensed:

```bash
# 1. branch
git checkout develop && git pull && git checkout -b release/X.Y.Z
git push -u origin release/X.Y.Z

# 2. edit on the branch: VERSION, CHANGELOG, docs version, then smoke test
echo "X.Y.Z" > VERSION
# ...migrate CHANGELOG [Unreleased] -> [X.Y.Z] - DATE, add fresh [Unreleased]...

# 3. to master via PR
gh pr create --base master --head release/X.Y.Z --title "Release X.Y.Z" --body "..."
gh pr merge --merge

# 4. tag (fires CI: build + publish + homebrew)
git checkout master && git pull
git tag -a vX.Y.Z -m "Porytiles X.Y.Z" && git push origin vX.Y.Z
gh run watch --workflow versioned_release.yml

# 5. back to develop, delete branch
git checkout develop && git merge --no-ff release/X.Y.Z && git push
git push origin --delete release/X.Y.Z

# 6. docs repos lockstep (per repo, AFTER main release verified)
#    release/X.Y.Z -> master -> tag vX.Y.Z -> back to develop

# 7. verify
brew update && brew install grunt-lucas/porytiles/porytiles && porytiles --version
```

---

## Appendix: stable Homebrew formula shape

`versioned_release.yml` updates `Formula/porytiles.rb` with `sed`, so the file
must already have this structure (it mirrors `porytiles@snapshot.rb` but points
at `v#{version}` release assets and installs both binaries). The workflow only
rewrites the `version` line and the three `sha256` lines.

```ruby
class Porytiles < Formula
  desc "Overworld tileset compiler for Pokémon Generation III decompilation projects"
  homepage "https://github.com/grunt-lucas/porytiles"
  version "X.Y.Z"

  if OS.linux? && Hardware::CPU.intel?
    url "https://github.com/grunt-lucas/porytiles/releases/download/v#{version}/porytiles-linux-amd64.zip"
    sha256 "..."
  elsif OS.linux? && Hardware::CPU.arm?
    url "https://github.com/grunt-lucas/porytiles/releases/download/v#{version}/porytiles-linux-arm64.zip"
    sha256 "..."
  elsif OS.mac? && Hardware::CPU.arm?
    url "https://github.com/grunt-lucas/porytiles/releases/download/v#{version}/porytiles-macos-arm64.zip"
    sha256 "..."
  end

  def install
    bin.install "porytiles"
    bin.install "porytiles-legacy"
  end

  test do
    system "#{bin}/porytiles", "--version"
    system "#{bin}/porytiles-legacy", "--version"
  end
end
```

`macos-amd64` (Intel) is deliberately absent across the whole pipeline because
the active codebase's C++23 dependencies do not build on `macos-13`'s Apple Clang.
Do not add an Intel-mac branch here until we have a toolchain-compatible fix.
