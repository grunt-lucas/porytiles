# Contributions
Contributions to Porytiles are always welcome! To contribute, fork Porytiles and then make your changes in a branch in
your fork. You may then submit a cross-repository PR to the main Porytiles repo with your feature. Please see below for
guidance on how to name your branch.

## Git Workflow and Repository Branch Conventions
Porytiles's Git workflow and branch name conventions follow the Gitflow model. Porytiles-specific idiosyncracies are
outlined below. For more on the Gitflow model, [please see this writeup.](https://nvie.com/posts/a-successful-git-branching-model/)
Contributors should be working on topic branches only. Admins will handle the housekeeping related to the `master`,
`develop`, `release`, and `hotfix` branches.

Note: occasionally, I may push small changes directly to `develop`. This will only happen on occasion, and in cases
where opening a PR would create unnecessary noise for repository watchers. E.g. if a previous `docs` PR had a typo,
I may just fix the typo with a direct `develop` push.

## Topic Branch Conventions
Some conventions for Porytiles topic branches. Please try to keep branch names compact.

### Features
A new feature should be developed on a topic branch titled `feature/<NAME>`, where `<NAME>` is a hyphenated description
of the feature. Following Gitflow, the `feature` branch should be created off the `develop` branch. Please try to keep
feature branch names compact.

E.g. for a `feature` branch to add a freestanding compilation mode, the branch name could be:
`feature/freestand-compile`.

### Bugfixes
A bugfix should be developed on a topic branch titled `bugfix/<NAME>`, where `<NAME>` is a hyphenated description of
the bug to be fixed. Following Gitflow, the `bugfix` branch should be created off the `develop` branch.

E.g. for a `bugfix` branch that fixes a problem with the attribute file emitter, the branch name could be:
`bugfix/fix-attr-emit`.

### Documentation
New documentation should be added on a topic branch titled `docs/<NAME>`, where `<NAME>` is a hyphenated description of
the doc contents. Following Gitflow, the `docs` branch should be created off the `develop` branch.

E.g. for a `docs` branch that updates the README, the branch name could be:
`docs/readme-update`.

### Housekeeping
Repository housekeeping (e.g. Actions changes, general branch management, moving files around, etc) should be done on
a topic branch titled `meta/<NAME>`, where `<NAME>` is a hyphenated description of the housekeeping. Following Gitflow,
the `meta` branch should be created off the `develop` branch.

E.g. for a `meta` branch that updates the nightly Actions workflow, the branch name could be:
`meta/nightly-actions-change`

### Issues
Branches that address a filed issue should fall into one of the above categories, but use the `<NAME>` to tag the issue.
E.g. if Issue #12 reports a bug, the branch to fix this could be called `bugfix/issue-0012`. If Issue #27 requests a
feature, the branch to implement this could be called `feature/issue-0027`. If necessary, the title may be extended for
more specificity. E.g. if `issue-0027` contains both a reported bug and a related feature request, we may have a
`bugfix/issue-0027/broken-attr` as well as a `feature/issue-0027/generate-attrs`.

## Branch Cleanup
Please use `git rebase --interactive` to clean up your branch before submitting a PR. If you have a ton of commits with
tiny changes, WIP descriptions, or bugs, you can use the interactive rebase to pick and squash them into a coherent
branch history. This way, viewers of the repository can clearly see the changes you made.
