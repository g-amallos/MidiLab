#!/bin/bash


if [ -z "$1" ]; then
    echo "Usage: $0 <search_term>"
    echo "Example: $0 InitWindow"
    exit 1
fi

SEARCH_TERM="$1"

echo "Searching for word '$SEARCH_TERM' in .c and .h files inside '../../modules' and '../../include'..."
echo "--------------------------------------------------------"


grep --include='*.h' --include='*.c' -rnw -e "$SEARCH_TERM" --color=always ../../modules ../../include

if [ $? -ne 0 ]; then
    echo "No matches found."
fi