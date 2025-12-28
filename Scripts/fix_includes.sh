#!/usr/bin/env bash

set -Eeuo pipefail

usage() {
    cat <<EOF
Usage: fix_includes.sh
       fix_includes.sh --help

Spawns a Claude Code session to fix relative include paths in Porytiles2 source
files. This is useful after using IDE refactoring tools that convert nice
absolute-style includes into ugly relative paths.

Examples of what gets fixed:
  Bad:  #include "../../infra/models/foo.hpp"
  Good: #include "porytiles2/infra/models/foo.hpp"

  Bad:  #include "../../../include/porytiles2/domain/bar.hpp"
  Good: #include "porytiles2/domain/bar.hpp"

Options:
    -h, --help      Print this help and exit.
    --dry-run       Show the prompt without executing Claude Code.

EOF
}

usage_exit_ok() {
    usage
    exit
}

DRY_RUN=false

parse_params() {
    while :; do
        case "${1-}" in
            -h | --help) usage_exit_ok ;;
            --dry-run) DRY_RUN=true ;;
            -?*) echo "Unknown option: $1"; usage; exit 1 ;;
            *) break ;;
        esac
        shift
    done
    return 0
}

if [[ ! -f .porytiles-marker-file ]]; then
    echo "Script must run in main Porytiles directory"
    exit 1
fi

parse_params "$@"

PROMPT='Fix all relative include paths in Porytiles2 source files (.hpp and .cpp).

PROBLEM:
IDE refactoring tools sometimes convert absolute-style includes into relative paths.
For example:
  - "#include \"../../infra/models/foo.hpp\"" should be "#include \"porytiles2/infra/models/foo.hpp\""
  - "#include \"../../../include/porytiles2/domain/bar.hpp\"" should be "#include \"porytiles2/domain/bar.hpp\""

TASK:
1. Search for all #include directives in Porytiles2/**/*.hpp and Porytiles2/**/*.cpp (and .ipp) that use relative paths (containing "../")
2. For each file with relative includes, convert them to the proper absolute-style format:
   - All includes should start with "porytiles2/" followed by the appropriate path
   - Remove any "../" or "../../" prefixes
   - Remove any "include/porytiles2/" patterns and replace with just "porytiles2/"
3. Do NOT modify includes for external libraries (like fmt, gsl, etc.) or standard library headers

RULES:
- Only fix includes that match the relative path pattern (containing "../")
- Preserve the order of includes within each file
- Do not add or remove any includes, only fix the paths
- After fixing, run the format script: ./Scripts/format.sh

Report which files were modified when done.'

if [[ "$DRY_RUN" == true ]]; then
    echo "=== DRY RUN - Would execute Claude Code with this prompt ==="
    echo ""
    echo "$PROMPT"
    exit 0
fi

echo "Spawning Claude Code to fix relative include paths..."
echo ""

claude -p "$PROMPT" --dangerously-skip-permissions
