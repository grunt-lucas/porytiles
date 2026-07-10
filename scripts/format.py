#!/usr/bin/env python3
# /// script
# requires-python = ">=3.14"
# dependencies = []
# ///
"""
Format Porytiles C++ source files using clang-format.

Usage:
    uv run scripts/format.py                          # Format all Porytiles sources
    uv run scripts/format.py file1.cpp file2.hpp      # Format specific files
    uv run scripts/format.py --check                  # Dry-run (CI mode), exit 1 if changes needed

Environment:
    CLANG_FORMAT    Override the clang-format binary (default: "clang-format").
                    Useful when only versioned binaries are installed,
                    e.g. CLANG_FORMAT=clang-format-21.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SOURCE_EXTENSIONS = {"*.cpp", "*.hpp", "*.h", "*.ipp"}

# Highest-supported versions first, so newer installs win when multiple coexist.
CLANG_FORMAT_VERSION_CANDIDATES = [f"clang-format-{v}" for v in range(25, 17, -1)]


def collect_sources(project_root):
    """Collect all C++ source files under porytiles/."""
    porytiles_dir = project_root / "porytiles"
    files = []
    for ext in SOURCE_EXTENSIONS:
        files.extend(porytiles_dir.rglob(ext))
    return sorted(files)


def _clang_format_major_version(binary):
    """Return the major version of a clang-format binary, or None if it cannot
    be determined."""
    try:
        result = subprocess.run([binary, "--version"], capture_output=True, text=True)
    except OSError:
        return None
    if result.returncode != 0:
        return None
    match = re.search(r"version (\d+)", result.stdout)
    return int(match.group(1)) if match else None


def find_clang_format():
    """Find a usable clang-format binary. A CLANG_FORMAT override is always
    honored as-is. Otherwise, checks plain clang-format and versioned aliases
    like clang-format-21 and returns the newest one found: systems often carry
    an outdated plain clang-format alongside a newer versioned install (e.g.
    GitHub runners ship clang-format 18 while CI installs clang-format-22),
    and an old binary can fail outright on newer .clang-format options.
    Returns None if no binary is found."""
    override = os.environ.get("CLANG_FORMAT")
    if override:
        return override

    best = None
    best_version = -1
    for candidate in ["clang-format", *CLANG_FORMAT_VERSION_CANDIDATES]:
        if shutil.which(candidate):
            version = _clang_format_major_version(candidate) or 0
            if version > best_version:
                best = candidate
                best_version = version

    return best


def resolve_clang_format():
    """Like find_clang_format(), but exits with an error if no binary is found."""
    clang_format = find_clang_format()
    if clang_format is None:
        print(
            "Error: Could not find a clang-format binary on PATH. Install clang-format "
            "or set CLANG_FORMAT to the binary you want to use.",
            file=sys.stderr,
        )
        sys.exit(1)
    return clang_format


def main():
    project_root = Path(__file__).resolve().parent.parent

    if not (project_root / ".porytiles-marker-file").exists():
        print("Error: Could not find .porytiles-marker-file at project root.", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Format Porytiles C++ source files using clang-format."
    )
    parser.add_argument(
        "files", nargs="*",
        help="Specific files to format. If omitted, formats all Porytiles sources."
    )
    parser.add_argument(
        "--check", action="store_true",
        help="Dry-run mode: exit 1 if any files would be changed (useful for CI)."
    )

    args = parser.parse_args()

    if args.files:
        files = [Path(f).resolve() for f in args.files]
        for f in files:
            if not f.exists():
                print(f"Error: File not found: {f}", file=sys.stderr)
                sys.exit(1)
    else:
        files = collect_sources(project_root)

    if not files:
        print("No source files found.")
        return

    clang_format = resolve_clang_format()
    cmd = [clang_format, "-style=file"]

    if args.check:
        cmd.extend(["--dry-run", "--Werror"])
    else:
        cmd.append("-i")

    cmd.extend(str(f) for f in files)

    result = subprocess.run(cmd)

    if result.returncode != 0:
        if args.check:
            print(f"Formatting check failed: {len(files)} files checked.", file=sys.stderr)
        sys.exit(result.returncode)
    else:
        if args.check:
            print(f"All {len(files)} files are properly formatted.")
        else:
            print(f"Formatted {len(files)} files.")


if __name__ == "__main__":
    main()
