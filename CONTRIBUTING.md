# Contributions
Contributions to Porytiles are welcome.
To contribute, fork Porytiles,
make your changes on a branch in your fork,
and submit a cross-repository PR to the main Porytiles repo.
Note that pushes to `develop` produce rolling snapshot builds for users,
so please only submit work that's feature-complete and ready for end-user consumption!

If you plan to use AI tooling on your contribution,
please review [`AI_POLICY.md`](./AI_POLICY.md) first.

PR type and topic are tracked via GitHub labels.
There is no required branch-name convention for contributor branches.
Pick something descriptive;
the PR target (`develop` for new work, `hotfix` for hotfixes, etc.) carries the meaning.

- [Contributions](#contributions)
- [Labels](#labels)
- [Branching](#branching)
- [Branch Cleanup](#branch-cleanup)
- [Changelog](#changelog)
- [Documentation](#documentation)


# Labels
Issues and pull requests are organized with a small set of GitHub labels.

If you're looking for somewhere to start,
browse the issue tracker filtered by:

- [`good first issue`](https://github.com/grunt-lucas/porytiles/issues?q=is%3Aopen+label%3A%22good+first+issue%22)
  — well-scoped, approachable work that's a good entry point to the codebase.
- [`help wanted`](https://github.com/grunt-lucas/porytiles/issues?q=is%3Aopen+label%3A%22help+wanted%22)
  — issues where outside contributions are especially welcome.

Each issue and PR also carries a *type* label describing the kind of change:

- `bug` — fixes incorrect behavior.
- `feature` — new end-user functionality.
- `refactoring` — code cleanup that preserves behavior.
- `tests` — adds or fixes tests and coverage.
- `documentation` — documentation changes.
- `repo-housekeeping` — build, CI, tooling, scripts, or repo configuration.
- `breaking-change` — changes that break backward compatibility (CLI, config, or output).

Maintainers apply these labels during triage and review,
so you don't need to set them yourself
(fork-based PRs can't add labels anyway).
Just describe your change clearly in the PR,
and flag it if it's a breaking change
or qualifies for the `no-changelog` exemption (see [Changelog](#changelog)).

The remaining labels
(`triage-blocked`, `duplicate`, `wont-do`, `support`, `legacy-porytiles`, `release`)
are for maintainer triage and release bookkeeping,
and rarely concern contributors directly.


# Branching
Porytiles follows a Gitflow branching model.
A short summary follows;
the operational runbook lives in [`RELEASE_PROCESS.md`](./RELEASE_PROCESS.md).

- **`develop`** is the integration branch.
  Open feature and bugfix PRs against `develop`.
  Every push to `develop` triggers a rolling snapshot build that publishes
  the `snapshot` GitHub release and updates the `porytiles-snapshot`
  Homebrew formula.
- **`master`** is the production branch.
  Direct pushes are blocked;
  it only receives merges from `release/X.Y.Z` and `hotfix/X.Y.Z` branches.
- **`release/X.Y.Z`** branches are cut from `develop` to stabilize a release.
  Bugfixes and release-prep edits land there;
  new features wait for the next cycle.
- **`hotfix/X.Y.Z`** branches are cut from `master` for urgent patches to a
  released version, then merged back to both `master` and `develop`.

Tags follow the `vX.Y.Z` pattern (lowercase `v`, no suffix).
Pushing a tag matching `v[0-9]+.[0-9]+.[0-9]+` triggers the versioned release
pipeline, which builds the binaries, publishes a permanent GitHub release,
and updates the `porytiles` Homebrew formula.

The `VERSION` file at the repo root is the single source of truth for the
project version.
CMake reads it at configure time and CI compares it to the pushed tag;
mismatches abort the release workflow.


# Branch Cleanup
Please use `git rebase --interactive` to clean up your branch before submitting a PR.
If you have a ton of commits with tiny changes, WIP descriptions, or bugs,
you can use the interactive rebase to pick and squash them into a coherent branch history.
This way, viewers of the repository can clearly see the changes you made.


# Changelog
Every PR that targets `develop` must add an entry to `CHANGELOG.md`
under the `## [Unreleased]` heading,
or carry the `no-changelog` label.
The `Porytiles Changelog Check` workflow enforces this on every PR.

Use the `no-changelog` label sparingly,
e.g. for typo fixes, CI tweaks, or infrastructure-only PRs
that do not affect end-user behavior.

At each release cut,
the accumulated `[Unreleased]` entries migrate
under a new `## [X.Y.Z] - YYYY-MM-DD` heading.
The format follows a simplified [Keep a Changelog](https://keepachangelog.com)
convention: a flat list of changes per version,
without Added/Removed/Modified subsections.


# Documentation
Porytiles spans three repositories that are tagged in lockstep:

- [`grunt-lucas/porytiles`](https://github.com/grunt-lucas/porytiles) — the compiler.
- [`grunt-lucas/porytiles-user-docs`](https://github.com/grunt-lucas/porytiles-user-docs) — user-facing tutorials and CLI reference.
- [`grunt-lucas/porytiles-dev-docs`](https://github.com/grunt-lucas/porytiles-dev-docs) — architecture and contributing guide.

A fourth repo,
[`grunt-lucas/homebrew-porytiles`](https://github.com/grunt-lucas/homebrew-porytiles),
hosts the Homebrew formulas and is updated by CI, not tagged.

When a contribution has user-visible impact (new CLI flags, new YAML keys,
changed semantics, etc.), the corresponding documentation update should occur
in the same release cycle.
For larger changes, open the docs PR alongside the code PR and link them.

The three-repo tag choreography is documented in
[`RELEASE_PROCESS.md`](./RELEASE_PROCESS.md).
