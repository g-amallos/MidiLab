#!/bin/bash


if [ -z "$1" ]; then
    echo "Usage: $0 <search_term> [directory]"
    echo "Example: $0 InitWindow modules"
    exit 1
fi

T_DIR="../../modules"
SEARCH_TERM="$1"
TARGET_DIR="${2:-$T_DIR}"

echo "Searching for anything containing '$SEARCH_TERM' in .c files inside '$TARGET_DIR'..."
echo "--------------------------------------------------------"


grep --include=\*.c -rn "$TARGET_DIR" -e "$SEARCH_TERM" --color=always

if [ $? -ne 0 ]; then
    echo "No matches found."
fi