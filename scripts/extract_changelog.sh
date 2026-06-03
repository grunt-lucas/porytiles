#!/usr/bin/env bash
# Extract a section's body from CHANGELOG.md for use as a release-notes body.
#
# Usage:
#   scripts/extract_changelog.sh Unreleased
#   scripts/extract_changelog.sh 1.0.0
#
# Prints the lines between the matching "## [<section>]" heading and the next
# "## [..." heading. Leading and trailing blank lines are trimmed. Inner
# blank lines are preserved. An empty section emits empty output and exits 0.
#
# Override the input file via the CHANGELOG env var (defaults to ./CHANGELOG.md).

set -euo pipefail

SECTION="${1:?Usage: $0 <section>  (e.g. Unreleased or 1.0.0)}"
CHANGELOG="${CHANGELOG:-CHANGELOG.md}"

if [ ! -f "$CHANGELOG" ]; then
    echo "extract_changelog.sh: $CHANGELOG not found" >&2
    exit 1
fi

awk -v section="$SECTION" '
    function get_section_name(line,    parts, n) {
        n = split(line, parts, /[][]/)
        return (n >= 2) ? parts[2] : ""
    }
    /^## \[/ {
        if (in_section) { exit }
        if (get_section_name($0) == section) {
            in_section = 1
            next
        }
        next
    }
    in_section {
        if (NF == 0) {
            buffered_blanks = buffered_blanks "\n"
            next
        }
        if (printed) printf "%s", buffered_blanks
        buffered_blanks = ""
        print
        printed = 1
    }
' "$CHANGELOG"
