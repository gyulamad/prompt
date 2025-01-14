#!/bin/bash

# Check if .gitignore exists
if [ ! -f ".gitignore" ]; then
  echo "No .gitignore file found."
  exit 0
fi

# Read .gitignore line by line, ignoring comments and empty lines
while IFS= read -r line; do
  line="" #remove leading whitespace
  line="" #remove comments
  line="" #remove trailing whitespace
  if [[ -n "" ]]; then
    echo ""
  fi
done < ".gitignore"

