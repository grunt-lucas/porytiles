#!/usr/bin/env python3
# /// script
# requires-python = ">=3.14"
# dependencies = []
# ///
"""
Search for TODO-style comments in Porytiles source files.

Usage:
    uv run scripts/todo.py all                        # All categories
    uv run scripts/todo.py todo                       # Just TODOs
    uv run scripts/todo.py fixme|note|feature|tests   # Other categories
    uv run scripts/todo.py all --path porytiles/lib  # Scope to directory
    uv run scripts/todo.py all --count                # Just counts per category
"""

import argparse
import shutil
import subprocess
import sys

CATEGORIES = {
    "todo": "TODO :",
    "fixme": "FIXME :",
    "note": "NOTE :",
    "feature": "FEATURE :",
    "tests": "TODO tests :",
}


def run_rg(pattern, path, count_only=False):
    """Run ripgrep with the given pattern and return the result."""
    cmd = ["rg", pattern, str(path)]
    if count_only:
        cmd.append("--count")

    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout, result.returncode


def search_category(name, pattern, path, count_only=False):
    """Search for a single category and print results."""
    output, returncode = run_rg(pattern, path, count_only)

    if count_only:
        total = 0
        for line in output.strip().splitlines():
            if ":" in line:
                count_str = line.rsplit(":", 1)[-1]
                try:
                    total += int(count_str)
                except ValueError:
                    pass
        print(f"  {name}: {total}")
        return total
    else:
        if output.strip():
            print(f"--- {name} ---")
            print(output, end="")
            print()
        return 0 if returncode != 0 else 1


def main():
    parser = argparse.ArgumentParser(
        description="Search for TODO-style comments in Porytiles source files."
    )
    parser.add_argument(
        "category",
        choices=["all", "todo", "fixme", "note", "feature", "tests"],
        type=str.lower,
        help="Category to search for (case-insensitive)."
    )
    parser.add_argument(
        "--path", default="porytiles",
        help="Directory to search in (default: Porytiles)."
    )
    parser.add_argument(
        "--count", action="store_true",
        help="Show only counts per category."
    )

    args = parser.parse_args()

    if not shutil.which("rg"):
        print("Error: ripgrep (rg) is required but not found in PATH.", file=sys.stderr)
        sys.exit(1)

    if args.category == "all":
        categories = CATEGORIES
    else:
        categories = {args.category: CATEGORIES[args.category]}

    if args.count:
        print("Counts:")

    for name, pattern in categories.items():
        search_category(name, pattern, args.path, count_only=args.count)


if __name__ == "__main__":
    main()
