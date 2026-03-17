#!/usr/bin/env python3
"""
Code coverage tooling for Porytiles2 using LLVM coverage instruments.

Usage:
    uv run Scripts/coverage.py build                                       # Configure + build + run tests
    uv run Scripts/coverage.py build --build-dir porytiles-build-coverage  # Custom build dir
    uv run Scripts/coverage.py show file1.cpp                              # Line-by-line coverage
    uv run Scripts/coverage.py report                                      # Summary report
    uv run Scripts/coverage.py report --html /tmp/cov                      # HTML report
    uv run Scripts/coverage.py clean                                       # Remove coverage build dir
"""

# The `build` subcommand handles CMake configuration automatically. For manual setup, the equivalent command is:
#
#   cmake -B porytiles-build-coverage -DCMAKE_BUILD_TYPE=Debug \
#     -DCMAKE_CXX_FLAGS="-fcoverage-mapping -fprofile-instr-generate" \
#     -DCMAKE_C_FLAGS="-fcoverage-mapping -fprofile-instr-generate"

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

DEFAULT_BUILD_DIR = "porytiles-build-coverage"


def ignore_deps_flag(build_path):
    """Return --ignore-filename-regex flag to exclude CMake dependency sources."""
    return f"--ignore-filename-regex=.*{build_path.name}/_deps/.*"


def resolve_llvm_tool(tool_name):
    """Resolve an LLVM tool path using env var, xcrun, or bare command."""
    env_path = os.environ.get("LLVM_COV_PATH")
    if env_path:
        return os.path.join(env_path, tool_name)
    if platform.system() == "Darwin":
        return f"xcrun {tool_name}"
    return tool_name


def run_cmd(cmd, description=None, shell=False):
    """Run a command, printing it and checking the result."""
    if description:
        print(f"\n=== {description} ===")
    display = cmd if isinstance(cmd, str) else " ".join(cmd)
    print(f"$ {display}")
    result = subprocess.run(cmd, shell=shell)
    if result.returncode != 0:
        print(f"Error: Command failed with exit code {result.returncode}.", file=sys.stderr)
        sys.exit(result.returncode)


def cmd_build(project_root, build_dir):
    """Configure, build, and run tests with coverage instrumentation."""
    build_path = project_root / build_dir

    # 1. Configure
    run_cmd([
        "cmake", "-B", str(build_path),
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_CXX_FLAGS=-fcoverage-mapping -fprofile-instr-generate",
        "-DCMAKE_C_FLAGS=-fcoverage-mapping -fprofile-instr-generate",
    ], description="Configuring coverage build")

    # 2. Build
    run_cmd([
        "cmake", "--build", str(build_path), "-j7",
    ], description="Building with coverage instrumentation")

    # 3. Run tests
    test_binary = build_path / "Porytiles2" / "tests" / "Porytiles2AllTests"
    if not test_binary.exists():
        print(f"Error: Test binary not found at {test_binary}.", file=sys.stderr)
        sys.exit(1)

    profraw = build_path / "default.profraw"
    env = os.environ.copy()
    env["LLVM_PROFILE_FILE"] = str(profraw)

    print(f"\n=== Running tests (profile -> {profraw}) ===")
    print(f"$ LLVM_PROFILE_FILE={profraw} {test_binary}")
    result = subprocess.run([str(test_binary)], env=env)
    if result.returncode != 0:
        print(f"Warning: Tests exited with code {result.returncode}.", file=sys.stderr)

    # 4. Merge profile data
    profdata = build_path / "testcov.profdata"
    merge_cmd = f"{resolve_llvm_tool('llvm-profdata')} merge -o {profdata} {profraw}"
    run_cmd(merge_cmd, description="Merging profile data", shell=True)

    print(f"\nCoverage build complete. Profile data: {profdata}")


def validate_profdata(build_path):
    """Validate that profdata exists and return paths."""
    profdata = build_path / "testcov.profdata"
    test_binary = build_path / "Porytiles2" / "tests" / "Porytiles2AllTests"

    if not profdata.exists():
        print(f"Error: Profile data not found at {profdata}.", file=sys.stderr)
        print("  Hint: Run 'uv run Scripts/coverage.py build' first.", file=sys.stderr)
        sys.exit(1)

    if not test_binary.exists():
        print(f"Error: Test binary not found at {test_binary}.", file=sys.stderr)
        print("  Hint: Run 'uv run Scripts/coverage.py build' first.", file=sys.stderr)
        sys.exit(1)

    return profdata, test_binary


def cmd_show(project_root, build_dir, files):
    """Show line-by-line coverage for specific files."""
    build_path = project_root / build_dir
    profdata, test_binary = validate_profdata(build_path)

    file_args = " ".join(str(f) for f in files)
    show_cmd = (
        f"{resolve_llvm_tool('llvm-cov')} show {test_binary}"
        f" -instr-profile={profdata}"
        f" {ignore_deps_flag(build_path)} {file_args}"
    )
    run_cmd(show_cmd, description="Showing coverage", shell=True)


def cmd_report(project_root, build_dir, html_dir=None):
    """Show a coverage summary report, optionally generating HTML."""
    build_path = project_root / build_dir
    profdata, test_binary = validate_profdata(build_path)

    if html_dir:
        html_path = Path(html_dir)
        html_path.mkdir(parents=True, exist_ok=True)
        report_cmd = (
            f"{resolve_llvm_tool('llvm-cov')} show {test_binary}"
            f" -instr-profile={profdata}"
            f" {ignore_deps_flag(build_path)}"
            f" -format=html -output-dir={html_path}"
        )
        run_cmd(report_cmd, description=f"Generating HTML report -> {html_path}", shell=True)
        print(f"\nHTML coverage report generated at: {html_path}")
    else:
        report_cmd = (
            f"{resolve_llvm_tool('llvm-cov')} report {test_binary}"
            f" -instr-profile={profdata}"
            f" {ignore_deps_flag(build_path)}"
        )
        run_cmd(report_cmd, description="Coverage report", shell=True)


def cmd_clean(project_root, build_dir):
    """Remove the coverage build directory."""
    build_path = project_root / build_dir
    if build_path.exists():
        shutil.rmtree(build_path)
        print(f"Removed {build_path}")
    else:
        print(f"Nothing to clean: {build_path} does not exist.")


def main():
    project_root = Path(__file__).resolve().parent.parent

    if not (project_root / ".porytiles-marker-file").exists():
        print("Error: Could not find .porytiles-marker-file at project root.", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Code coverage tooling for Porytiles2 using LLVM coverage instruments."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # build
    build_parser = subparsers.add_parser("build", help="Configure, build, and run tests with coverage.")
    build_parser.add_argument("--build-dir", default=DEFAULT_BUILD_DIR,
                              help=f"Build directory (default: {DEFAULT_BUILD_DIR}).")

    # show
    show_parser = subparsers.add_parser("show", help="Show line-by-line coverage for files.")
    show_parser.add_argument("files", nargs="+", help="Source files to show coverage for.")
    show_parser.add_argument("--build-dir", default=DEFAULT_BUILD_DIR,
                             help=f"Build directory (default: {DEFAULT_BUILD_DIR}).")

    # report
    report_parser = subparsers.add_parser("report", help="Show coverage summary report.")
    report_parser.add_argument("--build-dir", default=DEFAULT_BUILD_DIR,
                               help=f"Build directory (default: {DEFAULT_BUILD_DIR}).")
    report_parser.add_argument("--html", metavar="DIR",
                               help="Generate HTML report in the given directory.")

    # clean
    clean_parser = subparsers.add_parser("clean", help="Remove coverage build directory.")
    clean_parser.add_argument("--build-dir", default=DEFAULT_BUILD_DIR,
                              help=f"Build directory (default: {DEFAULT_BUILD_DIR}).")

    args = parser.parse_args()

    if args.command == "build":
        cmd_build(project_root, args.build_dir)
    elif args.command == "show":
        cmd_show(project_root, args.build_dir, args.files)
    elif args.command == "report":
        cmd_report(project_root, args.build_dir, html_dir=args.html)
    elif args.command == "clean":
        cmd_clean(project_root, args.build_dir)


if __name__ == "__main__":
    main()
