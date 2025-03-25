#!/bin/bash

# Get the directory where build.sh is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

eval "$SCRIPT_DIR/build.sh --mode=test"