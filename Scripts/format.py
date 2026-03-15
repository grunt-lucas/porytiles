#!/usr/bin/env python3
"""
Format Porytiles2 C++ source files using clang-format.

Usage:
    uv run Scripts/format.py                          # Format all Porytiles2 sources
    uv run Scripts/format.py file1.cpp file2.hpp      # Format specific files
    uv run Scripts/format.py --check                  # Dry-run (CI mode), exit 1 if changes needed
"""

import argparse
import subprocess
import sys
from pathlib import Path

SOURCE_EXTENSIONS = {"*.cpp", "*.hpp", "*.h", "*.ipp"}


def collect_sources(project_root):
    """Collect all C++ source files under Porytiles2/."""
    porytiles2_dir = project_root / "Porytiles2"
    files = []
    for ext in SOURCE_EXTENSIONS:
        files.extend(porytiles2_dir.rglob(ext))
    return sorted(files)


def main():
    project_root = Path(__file__).resolve().parent.parent

    if not (project_root / ".porytiles-marker-file").exists():
        print("Error: Could not find .porytiles-marker-file at project root.", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Format Porytiles2 C++ source files using clang-format."
    )
    parser.add_argument(
        "files", nargs="*",
        help="Specific files to format. If omitted, formats all Porytiles2 sources."
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

    cmd = ["clang-format", "-style=file"]

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
