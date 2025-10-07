#!/usr/bin/env bash

# Check if we have exactly 3 arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <directory> <find_string> <replace_string>"
    exit 1
fi

DIRECTORY="$1"
FIND_STRING="$2"
REPLACE_STRING="$3"

# Check if directory exists
if [ ! -d "$DIRECTORY" ]; then
    echo "Error: Directory '$DIRECTORY' does not exist"
    exit 1
fi

# Detect OS for sed compatibility
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    find "$DIRECTORY" -type f -exec sed -i '' "s|$FIND_STRING|$REPLACE_STRING|g" {} +
else
    # Linux
    find "$DIRECTORY" -type f -exec sed -i "s|$FIND_STRING|$REPLACE_STRING|g" {} +
fi

echo "Replacement complete: '$FIND_STRING' -> '$REPLACE_STRING' in $DIRECTORY"
