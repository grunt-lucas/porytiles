# Contributions
Contributions to Porytiles are welcome.
To contribute, fork Porytiles,
make your changes on a branch in your fork,
and submit a cross-repository PR to the main Porytiles repo.
Note that pushes to `develop` produce nightly snapshot builds for users,
so please only submit work that's feature-complete and ready for end-user consumption! 

If you plan to use AI tooling on your contribution,
please review [`AI_POLICY.md`](./AI_POLICY.md) first.

PR type and topic are tracked via GitHub labels.
There is no required branch-name convention.
Pick something descriptive.

- [Contributions](#contributions)
- [Branch Cleanup](#branch-cleanup)
- [Changelog](#changelog)


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
