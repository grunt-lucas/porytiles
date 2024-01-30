# Contributions
Contributions to Porytiles are always welcome! To contribute, you are encouraged to fork Porytiles and then make your
changes in a branch in your fork. You may then submit a cross-repository PR to the main Porytiles repo with your
feature. Please see below for guidance on how to name your branch.

# Repository Branch Names
Branch names in the Porytiles repository should follow these conventions:

### Trunk
The main development trunk is called `trunk`. Small or inconsequential changes (e.g. updating `Todo.md`) can be pushed
directly to `trunk`. Larger changes should be developed on a `feature/X` or `bugfix/X` branch that is merged back into
trunk via a well-documented pull request. More on these branches below.

Porytiles will create a nightly release based on the tip of the `trunk` branch, so care should be taken not to break
things on this branch. The unit and integration tests should be comprehensive, and they should abort the release if any
of them fail.

### Release
Releases should be performed on a parallel branch titled `release/<NAME>`, where name is the semantic version number
for this release. The branch should specify a major and minor release, with a placeholder for the revision, since
multiple revisions may be used on the same branch to hotfix issues. E.g. release branch for `1.12` would look like
`release/1.12.x`, where the first release tag on this branch would be `1.12.0`. If users find an issue with this release,
the issue can be fixed on trunk and then cherry picked to create a `1.12.1`, etc release.

### Features
A new feature should be developed on a branch titled `feature/<NAME>`, where `<NAME>` is a hyphenated description of the
feature. E.g. for a feature branch to add a freestanding compilation mode, the branch name could be:
`feature/freestanding-compile-mode`.

### Bugfixes
A Bug fix should be developed on a branch titled `bugfix/<NAME>`, where `<NAME>` is a hyphenated description of the bug
to be fixed. E.g. for a bugfix branch that fixes an attribute issue, the branch name could be:
`bugfix/broken-attr`.

### Issues
Branches that address a filed issue should fall into one of the above categories, but use the `<NAME>` to tag the issue.
E.g. if Issue #12 reports a bug, the branch to fix this could be called `bugfix/issue-0012`. If Issue #27 requests a
feature, the branch to implement this could be called `feature/issue-0027`. If necessary, the title may be extended for
more specificity. E.g. if `issue-0027` contains both a reported bug and a related feature request, we may have a
`bugfix/issue-0027-broken-attr` as well as a `feature/issue-0027-generate-attrs`.