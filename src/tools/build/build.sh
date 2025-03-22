#!/bin/bash

# Get the directory where build.sh is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default values
DEFAULT_BUILD_PATH="$SCRIPT_DIR/bin"
DEFAULT_BUILD_MODE="fast"

# Initialize variables
BUILD_PATH="$DEFAULT_BUILD_PATH"
BUILD_MODE="$DEFAULT_BUILD_MODE"

# Parse named parameters
while [ $# -gt 0 ]; do
    case "$1" in
        --output-folder=*)
            BUILD_PATH="${1#*=}"
            ;;
        --mode=*)
            BUILD_MODE="${1#*=}"
            ;;
        *)
            echo "Error: Unknown parameter '$1'. Use --output-folder=<path> and --mode=<mode>"
            echo "Supported modes: debug, fast, test"
            exit 1
            ;;
    esac
    shift
done

echo "Build mode: $BUILD_MODE"

# Define initial flags
STD="-std=c++20"
FLAGS="$STD -fsanitize=address -pedantic-errors -Werror -Wall -Wextra -Wunused -fno-elide-constructors"

# Define initial flags constant for potential overrides
INITIAL_FLAGS="$FLAGS"

# Add or override flags based on build mode
case "$BUILD_MODE" in
    "debug")
        FLAGS+=" -g -O0"
        ;;
    "fast")
        FLAGS="$STD -Ofast -march=native -funroll-loops"
        ;;
    "test")
        FLAGS+=" -g -O0 -DTEST -DTEST_ONLY"
        ;;
    *)
        echo "Error: Unknown build mode '$BUILD_MODE'. Supported modes: debug, fast, test"
        exit 1
        ;;
esac

# Ensure the build directory exists
mkdir -p "$BUILD_PATH"

# Compile with the specified or default build path and flags
CMD="g++ \"$SCRIPT_DIR/compile.cpp\" -o \"$BUILD_PATH/compile\" $FLAGS"
echo "$CMD"
eval "$CMD"

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Build successful: $BUILD_PATH/compile (mode: $BUILD_MODE)"
    if [ "$BUILD_MODE" = "test" ]; then
        echo "Executing $BUILD_PATH/compile as test..."
        "$BUILD_PATH/compile"
    fi
else
    echo "Build failed (mode: $BUILD_MODE)"
    exit 1
fi