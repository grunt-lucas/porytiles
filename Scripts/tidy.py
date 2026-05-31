#!/usr/bin/env python3
"""
Run clang-tidy on Porytiles C++ source files.

Usage:
    uv run Scripts/tidy.py                                  # All Porytiles .cpp files
    uv run Scripts/tidy.py file1.cpp file2.cpp              # Specific files
    uv run Scripts/tidy.py --build-dir porytiles-build-debug    # Override build dir
    uv run Scripts/tidy.py --fix                            # Apply auto-fixes
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path


def collect_sources(project_root):
    """Collect all .cpp translation units from Porytiles lib/ and tools/."""
    dirs = [
        project_root / "Porytiles" / "lib",
        project_root / "Porytiles" / "tools",
    ]
    files = []
    for d in dirs:
        if d.exists():
            files.extend(sorted(d.rglob("*.cpp")))
    return files


def main():
    project_root = Path(__file__).resolve().parent.parent

    if not (project_root / ".porytiles-marker-file").exists():
        print("Error: Could not find .porytiles-marker-file at project root.", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Run clang-tidy on Porytiles C++ source files."
    )
    parser.add_argument(
        "files", nargs="*",
        help="Specific .cpp files to check. If omitted, checks all Porytiles translation units."
    )
    parser.add_argument(
        "--build-dir", default="porytiles-build-debug",
        help="Build directory containing compile_commands.json (default: porytiles-build-debug)."
    )
    parser.add_argument(
        "--fix", action="store_true",
        help="Apply clang-tidy auto-fixes."
    )
    parser.add_argument(
        "--tidy-path",
        help="Path to clang-tidy executable (overrides TIDY_PATH env var)."
    )

    args = parser.parse_args()

    # Resolve clang-tidy executable
    tidy_cmd = args.tidy_path or os.environ.get("TIDY_PATH", "clang-tidy")

    # Validate compile_commands.json
    build_dir = project_root / args.build_dir
    compile_commands = build_dir / "compile_commands.json"
    if not compile_commands.exists():
        print(f"Error: compile_commands.json not found at {compile_commands}", file=sys.stderr)
        print(f"  Hint: Build the project first with: cmake -B {args.build_dir} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON", file=sys.stderr)
        sys.exit(1)

    # Collect files
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

    print(f"Running clang-tidy on {len(files)} file(s)...")

    cmd = [tidy_cmd, f"-p={build_dir}"]
    if args.fix:
        cmd.append("--fix")
    cmd.extend(str(f) for f in files)

    result = subprocess.run(cmd)
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
