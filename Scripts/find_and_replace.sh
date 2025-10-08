#!/usr/bin/env bash

set -e

# Check if we have exactly 3 arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <file_or_directory> <find_string> <replace_string>"
    exit 1
fi

TARGET="$1"
FIND_STRING="$2"
REPLACE_STRING="$3"

# Check if target exists
if [ ! -e "$TARGET" ]; then
    echo "Error: '$TARGET' does not exist"
    exit 1
fi

# Detect OS for sed compatibility
if [[ "$OSTYPE" == "darwin"* ]]; then
    SED_INPLACE=(-i '')
else
    SED_INPLACE=(-i)
fi

# Handle file or directory
if [ -f "$TARGET" ]; then
    # Single file
    sed "${SED_INPLACE[@]}" "s|$FIND_STRING|$REPLACE_STRING|g" "$TARGET"
    echo "Replacement complete: '$FIND_STRING' -> '$REPLACE_STRING' in $TARGET"
elif [ -d "$TARGET" ]; then
    # Directory - process recursively
    find "$TARGET" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -exec sed "${SED_INPLACE[@]}" "s|$FIND_STRING|$REPLACE_STRING|g" {} +
    echo "Replacement complete: '$FIND_STRING' -> '$REPLACE_STRING' in $TARGET"
else
    echo "Error: '$TARGET' is neither a file nor a directory"
    exit 1
fi
