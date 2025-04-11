#!/bin/bash

echo "Deleting bin folder..."
# Get the directory where bin folder is located
BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/bin"
echo "$BIN_DIR"
rm -rf "$BIN_DIR" && echo "OK"