#!/usr/bin/env python3
# /// script
# requires-python = ">=3.14"
# dependencies = []
# ///
"""
Interactive driver for the Porytiles regular versioned release process.

Executes the runbook in RELEASE_PROCESS.md step by step. Steps marked
[confirm] in the runbook pause and require an explicit "y" before running
anything outward-facing or hard to reverse (pushing to master, pushing tags,
deleting branches). Informational pauses accept ENTER.

Usage:
    uv run scripts/release.py 2.1.0                # Full release of 2.1.0
    uv run scripts/release.py 2.1.0 --from tag     # Resume a partial release at the tag step
    uv run scripts/release.py 2.1.0 --dry-run      # Print every command without executing
    uv run scripts/release.py --list-steps         # Show step names for --from
"""

import argparse
import datetime
import re
import subprocess
import sys
import time
from pathlib import Path

BUILD_DIR = "porytiles-build-release"
DOCS_REPOS = ["porytiles-user-docs", "porytiles-dev-docs"]
INSTALL_PREFIX = Path.home() / ".local"
LOG_DIR = Path("/tmp")

DRY_RUN = False
REPO_ROOT = None  # set in main()
SCRIPT_START_UTC = datetime.datetime.now(datetime.timezone.utc)


def die(msg):
    print(f"\nrelease.py: FATAL: {msg}", file=sys.stderr)
    sys.exit(1)


def info(msg):
    print(f"\n==> {msg}")


def run(cmd, cwd=None, capture=False, check=True):
    """Echo and execute a command. In dry-run mode, echo only."""
    where = f" (in {cwd})" if cwd else ""
    print(f"  $ {' '.join(cmd)}{where}")
    if DRY_RUN:
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")
    result = subprocess.run(cmd, cwd=cwd, capture_output=capture, text=True)
    if check and result.returncode != 0:
        if capture:
            print(result.stdout, file=sys.stderr)
            print(result.stderr, file=sys.stderr)
        die(f"command failed with exit {result.returncode}: {' '.join(cmd)}")
    return result


def query(cmd, cwd=None):
    """Run a read-only command and return stripped stdout. Executes even in dry-run."""
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        die(f"query failed with exit {result.returncode}: {' '.join(cmd)}\n{result.stderr}")
    return result.stdout.strip()


def confirm(prompt):
    """A [confirm] gate from the runbook. Requires an explicit y. Aborts otherwise."""
    if DRY_RUN:
        print(f"  [confirm skipped in dry-run] {prompt}")
        return
    answer = input(f"\n[confirm] {prompt} [y/N] ").strip().lower()
    if answer != "y":
        die("aborted at confirmation gate. Re-run with --from <step> to resume.")


def pause(prompt):
    """An informational pause. ENTER continues."""
    if DRY_RUN:
        print(f"  [pause skipped in dry-run] {prompt}")
        return
    input(f"\n[review] {prompt} Press ENTER to continue. ")


def parse_semver(text):
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", text)
    if not match:
        die(f"'{text}' is not a plain X.Y.Z semver string")
    return tuple(int(part) for part in match.groups())


def check_no_competing_builds():
    """Refuse to build while an IDE or another CMake owns any build tree.

    Two CMake runs sharing a tree race over the FetchContent populate steps and
    the loser's git clone dies mid-transfer (see RELEASE_PROCESS.md step 3).
    """
    if DRY_RUN:
        print("  [dry-run] would refuse to proceed while IDE/CMake processes are running")
        return
    while True:
        # Match process names only (no -f): full-command matching drags in every
        # IDE helper process whose path contains the IDE name.
        found = subprocess.run(
            ["pgrep", "-il", "clion|cmake|gmake"], capture_output=True, text=True
        ).stdout.strip()
        if not found:
            return
        print("\nThese build-related processes are still running:")
        print(found)
        pause("Quit the IDE / wait for builds to finish, then")


# Step implementations. Each is self-contained about branch context so a
# resume via --from lands in a sane state.


def step_preflight(version):
    """Verify clean develop, green snapshot CI, and a populated [Unreleased] section."""
    info("Step 0: pre-flight")
    if query(["git", "status", "--porcelain"], cwd=REPO_ROOT):
        if DRY_RUN:
            print("  [dry-run] working tree is not clean; a real run stops here")
        else:
            die("working tree is not clean")
    run(["git", "checkout", "develop"], cwd=REPO_ROOT)
    run(["git", "pull"], cwd=REPO_ROOT)

    current = (REPO_ROOT / "VERSION").read_text().strip()
    if parse_semver(version) <= parse_semver(current):
        die(f"target version {version} is not greater than current VERSION {current}")

    conclusion = query([
        "gh", "run", "list", "--workflow", "snapshot_release.yml", "--branch", "develop",
        "--limit", "1", "--json", "conclusion", "--jq", ".[0].conclusion",
    ], cwd=REPO_ROOT)
    if conclusion != "success":
        die(f"latest snapshot run on develop concluded '{conclusion}', expected success")
    print(f"  latest develop snapshot run: success. Current VERSION: {current}")

    unreleased = query(["scripts/extract_changelog.sh", "Unreleased"], cwd=REPO_ROOT)
    if not unreleased:
        die("the [Unreleased] CHANGELOG section is empty; nothing to release")
    print("\n--- [Unreleased] section that will become the release notes ---")
    print(unreleased)
    print("---")
    pause(f"Review the section above. It ships as {version}'s release notes.")


def step_branch(version):
    """Cut release/X.Y.Z from develop and push it."""
    info(f"Step 1: create release/{version}")
    run(["git", "checkout", "develop"], cwd=REPO_ROOT)
    run(["git", "pull"], cwd=REPO_ROOT)
    run(["git", "checkout", "-b", f"release/{version}"], cwd=REPO_ROOT)
    run(["git", "push", "-u", "origin", f"release/{version}"], cwd=REPO_ROOT)


def step_edits(version):
    """Bump VERSION, migrate the CHANGELOG, commit and push after review."""
    info("Step 2: bump VERSION and migrate CHANGELOG")
    today = datetime.date.today().isoformat()

    if DRY_RUN:
        print(f"  [dry-run] would write VERSION={version} and migrate CHANGELOG under {today}")
        return
    (REPO_ROOT / "VERSION").write_text(f"{version}\n")

    changelog = REPO_ROOT / "CHANGELOG.md"
    text = changelog.read_text()
    heading = "## [Unreleased]"
    if text.count(heading) != 1:
        die(f"expected exactly one '{heading}' heading in CHANGELOG.md")
    if f"## [{version}]" in text:
        die(f"CHANGELOG.md already has a [{version}] section")
    text = text.replace(heading, f"{heading}\n\n## [{version}] - {today}", 1)
    changelog.write_text(text)

    # The release-notes extractor must see a populated section and an empty Unreleased.
    if not query(["scripts/extract_changelog.sh", version], cwd=REPO_ROOT):
        die(f"extract_changelog.sh returns nothing for {version} after migration")
    if query(["scripts/extract_changelog.sh", "Unreleased"], cwd=REPO_ROOT):
        die("the fresh [Unreleased] section is not empty after migration")

    run(["git", "--no-pager", "diff", "--stat"], cwd=REPO_ROOT)
    pause("Review the diff (VERSION + CHANGELOG). Hand-edit now if anything is off, then")
    run(["git", "add", "VERSION", "CHANGELOG.md"], cwd=REPO_ROOT)
    run(["git", "commit", "-m", f"Bump VERSION to {version} and migrate CHANGELOG"], cwd=REPO_ROOT)
    run(["git", "push"], cwd=REPO_ROOT)


def step_smoke(version):
    """Clean Release-config build, both test suites, install, version+date check."""
    info(f"Step 3: local smoke test in {BUILD_DIR} (Release config)")
    run(["git", "checkout", f"release/{version}"], cwd=REPO_ROOT)
    check_no_competing_builds()
    build = REPO_ROOT / BUILD_DIR

    run(["rm", "-rf", str(build)], cwd=REPO_ROOT)
    for name, cmd in [
        ("configure", ["cmake", "-B", str(build), "-S", ".", "-DCMAKE_BUILD_TYPE=Release"]),
        ("build", ["cmake", "--build", str(build), "-j7"]),
        ("porytiles tests", [str(build / "porytiles/tests/PorytilesAllTests")]),
        ("legacy tests", [str(build / "legacy/tests/LegacyTests")]),
        ("install", ["cmake", "--install", str(build), "--prefix", str(INSTALL_PREFIX)]),
    ]:
        log = LOG_DIR / f"release_{name.replace(' ', '_')}.log"
        print(f"  $ {' '.join(cmd)}  > {log}")
        if DRY_RUN:
            continue
        with open(log, "w") as handle:
            result = subprocess.run(cmd, cwd=REPO_ROOT, stdout=handle, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            die(f"smoke '{name}' failed with exit {result.returncode}; see {log}")
        print(f"  {name}: OK")

    for binary in ["porytiles", "porytiles-legacy"]:
        check_installed_binary(binary, version)


def check_installed_binary(binary, version):
    """Verify the installed binary reports the target version AND a fresh build date.

    --version reports on whatever sits in the install prefix, so a stale binary
    from an earlier install answers with a plausible version and exit 0. The
    build-date stamp is what proves this run produced it.
    """
    if DRY_RUN:
        print(f"  [dry-run] would verify {binary} --version reports {version} with a fresh date")
        return
    output = query([str(INSTALL_PREFIX / "bin" / binary), "--version"])
    print(f"  {output}")
    parts = output.split()  # e.g.: porytiles 2.0.0 2026.08.27T18:24:52+00:00
    if len(parts) != 3 or parts[1] != version:
        die(f"{binary} reports '{output}', expected version {version}")
    built = datetime.datetime.strptime(parts[2], "%Y.%m.%dT%H:%M:%S%z")
    if built < SCRIPT_START_UTC - datetime.timedelta(minutes=5):
        die(f"{binary} build date {parts[2]} predates this run: stale binary, install did not take")


def step_pr(version):
    """Open and merge the release PR into master (checks mergeability first)."""
    info("Step 4: PR the release branch into master")
    run(["git", "checkout", f"release/{version}"], cwd=REPO_ROOT)
    run(["git", "fetch", "origin"], cwd=REPO_ROOT)

    clean = DRY_RUN or subprocess.run(
        ["git", "merge-tree", "--write-tree", "origin/master", f"release/{version}"],
        cwd=REPO_ROOT, capture_output=True,
    ).returncode == 0
    if not clean:
        print(
            "\nThe merge into master will CONFLICT. GitHub's 'Resolve conflicts' button\n"
            "merges master into the release branch, which gitflow forbids. Resolve it\n"
            "locally in the sanctioned direction instead:\n"
            f"  1. gh pr create --base master --head release/{version} --title 'Release {version}' ...\n"
            "  2. git checkout master && git pull\n"
            f"  3. git merge --no-ff release/{version}   # resolve, keeping the release side\n"
            "  4. git push origin master                 # the PR flips to merged\n"
            f"Then resume with: uv run scripts/release.py {version} --from tag"
        )
        die("conflicting merge requires manual resolution")

    confirm(f"Open the PR 'Release {version}' against master?")
    run([
        "gh", "pr", "create", "--base", "master", "--head", f"release/{version}",
        "--title", f"Release {version}",
        "--body", f"Release of {version}. See CHANGELOG.md.",
    ], cwd=REPO_ROOT)
    pause("Review the PR on GitHub if you want, then")
    confirm("Merge the PR into master with a merge commit?")
    run(["gh", "pr", "merge", "--merge"], cwd=REPO_ROOT)


def step_tag(version):
    """Tag vX.Y.Z on master and push it. Point of no return."""
    info(f"Step 5: tag v{version} on master (point of no return)")
    run(["git", "checkout", "master"], cwd=REPO_ROOT)
    run(["git", "pull"], cwd=REPO_ROOT)
    on_master = query(["git", "show", "HEAD:VERSION"], cwd=REPO_ROOT)
    if not DRY_RUN and on_master != version:
        die(f"VERSION on master reads '{on_master}', expected '{version}'; CI would abort at prepare")
    confirm(
        f"Push tag v{version}? This publishes the permanent release and rewrites "
        "the public Homebrew formula."
    )
    run(["git", "tag", "-a", f"v{version}", "-m", f"Porytiles {version}"], cwd=REPO_ROOT)
    run(["git", "push", "origin", f"v{version}"], cwd=REPO_ROOT)
    run(["git", "checkout", f"release/{version}"], cwd=REPO_ROOT)


def step_ci(version):
    """Watch versioned_release.yml and verify the published release."""
    info("Step 6: watch the versioned release workflow")
    if DRY_RUN:
        print("  [dry-run] would watch versioned_release.yml and verify the release")
        return
    time.sleep(10)  # give the tag push a moment to register a run
    run_id = query([
        "gh", "run", "list", "--workflow", "versioned_release.yml",
        "--limit", "1", "--json", "databaseId", "--jq", ".[0].databaseId",
    ], cwd=REPO_ROOT)
    result = subprocess.run(["gh", "run", "watch", run_id, "--exit-status"], cwd=REPO_ROOT)
    if result.returncode != 0:
        die(
            f"versioned release run {run_id} failed. Inspect with:\n"
            f"  gh run view {run_id} --log-failed\n"
            f"If only a verification leg failed, fix it and resume with --from backmerge."
        )
    asset_count = query([
        "gh", "release", "view", f"v{version}", "--json", "assets", "--jq", ".assets | length",
    ], cwd=REPO_ROOT)
    if asset_count != "3":
        die(f"release v{version} has {asset_count} assets, expected 3")
    print(f"  release v{version}: 3 assets published")


def step_backmerge(version):
    """Merge the release branch back into develop and delete it."""
    info("Step 7: merge the release branch back into develop")
    run(["git", "checkout", "develop"], cwd=REPO_ROOT)
    run(["git", "pull"], cwd=REPO_ROOT)
    run(["git", "merge", "--no-ff", f"release/{version}"], cwd=REPO_ROOT)
    run(["git", "push"], cwd=REPO_ROOT)
    confirm(f"Delete branch release/{version} (remote and local)?")
    run(["git", "push", "origin", "--delete", f"release/{version}"], cwd=REPO_ROOT)
    run(["git", "branch", "-d", f"release/{version}"], cwd=REPO_ROOT)


def step_docs(version):
    """Release both docs repos: bump, rebuild site, PR, tag, back-merge."""
    info("Step 8: docs repos lockstep")
    for repo in DOCS_REPOS:
        repo_path = REPO_ROOT / repo
        if not (repo_path / ".git").exists():
            die(f"{repo} is not cloned at {repo_path}")
        if not DRY_RUN:
            answer = input(f"\nRelease {repo} now? [Y/n] ").strip().lower()
            if answer == "n":
                print(f"  skipping {repo}")
                continue
        release_docs_repo(repo_path, version)


def release_docs_repo(repo_path, version):
    """Delegate one docs repo's cut to the scripts/release.py that repo carries."""
    script = repo_path / "scripts" / "release.py"
    if not script.exists():
        die(f"{repo_path.name} has no scripts/release.py; update its develop checkout")
    cmd = ["uv", "run", "scripts/release.py", version]
    if DRY_RUN:
        cmd.append("--dry-run")
    run(cmd, cwd=repo_path)


def step_verify(version):
    """Verify the Homebrew install and clean up the build tree."""
    info("Step 9: post-release verification")
    run(["brew", "update"], cwd=REPO_ROOT)
    # The fully qualified form is auto-trusted under Homebrew 6.0's Tap Trust gate.
    result = run(
        ["brew", "install", "grunt-lucas/porytiles/porytiles"], cwd=REPO_ROOT, check=False
    )
    if not DRY_RUN and result.returncode != 0:
        print("  install failed (perhaps already installed); trying upgrade")
        run(["brew", "upgrade", "grunt-lucas/porytiles/porytiles"], cwd=REPO_ROOT)
    for binary in ["porytiles", "porytiles-legacy"]:
        if DRY_RUN:
            continue
        output = query(["brew", "--prefix"])
        version_line = query([f"{output}/bin/{binary}", "--version"])
        print(f"  {version_line}")
        if f" {version} " not in f"{version_line} ":
            die(f"brew-installed {binary} reports '{version_line}', expected {version}")

    print(
        "\nManual checks remaining:\n"
        "  - https://grunt-lucas.github.io/porytiles-user-docs shows the new version\n"
        "  - https://grunt-lucas.github.io/porytiles-dev-docs shows the new version\n"
        f"  - gh release view v{version} is marked Latest with notes from the CHANGELOG"
    )
    confirm(f"Remove the release build tree {BUILD_DIR}?")
    run(["rm", "-rf", str(REPO_ROOT / BUILD_DIR)], cwd=REPO_ROOT)
    info(f"Release {version} complete.")


STEPS = [
    ("preflight", step_preflight),
    ("branch", step_branch),
    ("edits", step_edits),
    ("smoke", step_smoke),
    ("pr", step_pr),
    ("tag", step_tag),
    ("ci", step_ci),
    ("backmerge", step_backmerge),
    ("docs", step_docs),
    ("verify", step_verify),
]


def main():
    global DRY_RUN, REPO_ROOT

    parser = argparse.ArgumentParser(description="Drive a Porytiles versioned release.")
    parser.add_argument("version", nargs="?", help="target version, e.g. 2.1.0")
    parser.add_argument("--from", dest="from_step", metavar="STEP",
                        help="resume from this step (see --list-steps)")
    parser.add_argument("--dry-run", action="store_true",
                        help="print every command without executing anything")
    parser.add_argument("--list-steps", action="store_true", help="list step names and exit")
    args = parser.parse_args()

    if args.list_steps:
        for name, func in STEPS:
            print(f"  {name:10s} {func.__doc__.splitlines()[0] if func.__doc__ else ''}")
        return

    if not args.version:
        parser.error("version is required (e.g. 2.1.0)")
    parse_semver(args.version)
    DRY_RUN = args.dry_run

    REPO_ROOT = Path(query(["git", "rev-parse", "--show-toplevel"]))
    if not (REPO_ROOT / "RELEASE_PROCESS.md").exists():
        die(f"{REPO_ROOT} does not look like the porytiles repo root")

    names = [name for name, _ in STEPS]
    start = 0
    if args.from_step:
        if args.from_step not in names:
            die(f"unknown step '{args.from_step}'; valid: {', '.join(names)}")
        start = names.index(args.from_step)

    for name, func in STEPS[start:]:
        func(args.version)


if __name__ == "__main__":
    main()
