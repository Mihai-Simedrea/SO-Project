#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: ./script <c>"
    exit 1
fi

character="$1"
correct_sentences=0

while IFS= read -r line; do
    if [[ $line =~ ^[A-Z][a-zA-Z0-9\ \,]*[\?\!\.]$ && ! $line =~ ,\ (si) ]]; then
        if [[ $line == *"$character"* ]]; then
            ((correct_sentences++))
        fi
    fi
done

echo "$correct_sentences"
