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


def find_clang_format():
    """Find a usable clang-format binary, preferring CLANG_FORMAT, then plain
    clang-format, then versioned aliases like clang-format-21. Returns None if
    no binary is found."""
    override = os.environ.get("CLANG_FORMAT")
    if override:
        return override

    for candidate in ["clang-format", *CLANG_FORMAT_VERSION_CANDIDATES]:
        if shutil.which(candidate):
            return candidate

    return None


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
