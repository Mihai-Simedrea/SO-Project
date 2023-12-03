#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

file="$1"

if [ ! -f "$file" ]; then
    echo "File does not exist or is not a regular file."
    exit 1
fi

counter=0

while IFS= read -r line || [ -n "$line" ]; do 
    ((counter++))
done < "$file"

echo $counter
