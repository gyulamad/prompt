#!/bin/bash

echo "Delete coverage infos..."
rm -rf coverage && echo "OK"

# Navigate to the builds/ directory
cd builds/ || { echo "Error: builds/ directory not found"; exit 1; }

# Delete all files and directories except those named .gitkeep or containing .gitkeep
find . -type f -not -name ".gitkeep" -delete
find . -type d -not -name "." -empty -delete

# Print a confirmation message
echo "Cleaned builds/ directory, preserving .gitkeep files"

# Delete all *.o files in src/ recursively
echo "Deleting *.o files in src/..."
find ../src -type f -name "*.o" -delete && echo "OK"