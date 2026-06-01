#!/usr/bin/env python3
# /// script
# requires-python = ">=3.14"
# dependencies = []
# ///
"""
Find and replace strings in C++ source files.

Usage:
    uv run scripts/find_and_replace.py <target> <find> <replace>
    uv run scripts/find_and_replace.py --dry-run <target> <find> <replace>
"""

import argparse
import sys
from pathlib import Path

SOURCE_EXTENSIONS = {".cpp", ".hpp", ".h", ".ipp", ".cpp.jinja2", ".hpp.jinja2"}


def has_source_extension(path):
    """Check if a file has one of the target source extensions."""
    name = path.name
    for ext in SOURCE_EXTENSIONS:
        if name.endswith(ext):
            return True
    return False


def collect_files(target):
    """Collect source files from the target path."""
    target = Path(target)
    if target.is_file():
        return [target]
    elif target.is_dir():
        return sorted(f for f in target.rglob("*") if f.is_file() and has_source_extension(f))
    else:
        print(f"Error: '{target}' is neither a file nor a directory.", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Find and replace strings in C++ source files."
    )
    parser.add_argument("target", help="File or directory to process.")
    parser.add_argument("find", help="String to find.")
    parser.add_argument("replace", help="Replacement string.")
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Report what would change without writing files."
    )

    args = parser.parse_args()

    target = Path(args.target)
    if not target.exists():
        print(f"Error: '{target}' does not exist.", file=sys.stderr)
        sys.exit(1)

    files = collect_files(target)
    if not files:
        print("No source files found.")
        return

    total_replacements = 0
    files_changed = 0

    for filepath in files:
        content = filepath.read_text()
        count = content.count(args.find)

        if count == 0:
            continue

        total_replacements += count
        files_changed += 1

        if args.dry_run:
            print(f"  {filepath}: {count} replacement(s)")
        else:
            new_content = content.replace(args.find, args.replace)
            filepath.write_text(new_content)
            print(f"  {filepath}: {count} replacement(s)")

    if args.dry_run:
        print(f"\nDry run: {total_replacements} replacement(s) in {files_changed} file(s) would be made.")
    else:
        print(f"\nDone: {total_replacements} replacement(s) in {files_changed} file(s).")


if __name__ == "__main__":
    main()
