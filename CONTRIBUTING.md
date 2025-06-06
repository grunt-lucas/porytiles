# Contributions
Contributions to Porytiles are always welcome!
To contribute, fork Porytiles and then make your changes in a branch in your fork.
You may then submit a cross-repository PR to the main Porytiles repo with your feature.
Please see below for guidance on how to name your branch.

- [Contributions](#contributions)
- [Git Workflow and Repository Branch Conventions](#git-workflow-and-repository-branch-conventions)
- [Topic Branch Conventions](#topic-branch-conventions)
  - [Bugfixes](#bugfixes)
  - [Documentation](#documentation)
  - [Features](#features)
  - [Refactoring](#refactoring)
  - [Repository Housekeeping](#repository-housekeeping)
  - [Tests](#tests)
- [Issues](#issues)
- [Branch Cleanup](#branch-cleanup)


# Git Workflow and Repository Branch Conventions
Porytiles follows a Continuous Delivery workflow,
[as outlined here.](./README.md#release-cadence)
Changes should not hit the `develop` branch
until they are ready for release to the userbase.
Changes that are still baking should remain on a topic branch
or be disabled via a feature flag.
Topic branch name conventions are outlined below.

# Topic Branch Conventions
Some conventions for Porytiles topic branches.
These follow the labels in the repo.
Please try to keep branch names compact.
The topic branches should be created off the `develop` branch.
The topic branch name should follow the format `<TOPIC>/<DESCRIPTION>`,
where `<TOPIC>` is one of the topics below,
and `<DESCRIPTION>` is a very brief description of the change.
Multi-word branch names should use kebab-case, not snake_case.

## Bugfixes
A bugfix should be made on a topic branch titled `bug/<DESCRIPTION>`.

E.g. for a branch that fixes a problem with the attribute file emitter,
the branch name could be: `bug/fix-attr-emit`.

## Documentation
New documentation should be added on a topic branch titled `docs/<DESCRIPTION>`,

E.g. for a branch that updates the README,
the branch name could be: `docs/readme-update`.

Documentation and doc comments should use [semantic linebreaks.](https://sembr.org/)

## Features
A new feature should be developed on a topic branch titled `feature/<DESCRIPTION>`.

E.g. for a branch to add a freestanding compilation mode,
the branch name could be: `feature/freestand-compile`.

## Refactoring
Refactors should be done on a topic branch titled `refactoring/<DESCRIPTION>`.

E.g. for a branch that refactors the diagnostic system,
the branch name could be: `refactoring/diagnostics`.

## Repository Housekeeping
Repository housekeeping
(e.g. CI/CD changes, general branch management, moving files around, etc.)
should be done on a topic branch titled either
`meta/<DESCRIPTION>` or `repo-housekeeping/<DESCRIPTION>`.

E.g. for a branch that adds a new Ubuntu ARM build type the nightly build workflow,
the branch name could be:
`meta/nightly-build-linux-arm` or `repo-housekeeping/nightly-build-linux-arm`

## Tests
New tests or test updates should be made on a topic branch titled `tests/<DESCRIPTION>`.

E.g. for a branch that adds tests for palette primers,
the branch name could be: `tests/palette-primers`.

# Issues
Branches that address a filed issue should fall into one of the above categories,
but use the `<DESCRIPTION>` to tag the issue.

E.g. if Issue #12 reports a bug, the branch to fix this could be called `bug/issue-0012`.
If Issue #27 requests a feature, the branch to implement this could be called `feature/issue-0027`.
If necessary, the title may be extended with an additional `/` for more specificity.

E.g. if `issue-0027` contains both a reported bug with the attribute system,
but the bug is too complex to fix in one pull request, the branches could be:
`bug/issue-0027/add-missing-attr` as well as a `bug/issue-0027/fix-emitter`.

# Branch Cleanup
Please use `git rebase --interactive` to clean up your branch before submitting a PR.
If you have a ton of commits with tiny changes, WIP descriptions, or bugs,
you can use the interactive rebase to pick and squash them into a coherent branch history.
This way, viewers of the repository can clearly see the changes you made.
