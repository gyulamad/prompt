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
        FLAGS+=" -g -O0 -DTEST -DTEST_ONLY -fprofile-arcs -ftest-coverage"
        ;;
    *)
        echo "Error: Unknown build mode '$BUILD_MODE'. Supported modes: debug, fast, test"
        exit 1
        ;;
esac

# Ensure the build directory exists
mkdir -p "$BUILD_PATH"

# Array to store object files
OBJ_FILES=()

# Function to compile a single dependency
dep() {
    local src="$1"
    local obj="$BUILD_PATH/$(basename "${src%.cpp}.o")"
    
    # Compile the dependency into an object file
    local cmd="g++ -c \"$src\" -o \"$obj\" $FLAGS"
    echo "$cmd"
    eval "$cmd" || { echo "Error compiling $src"; exit 1; }
    
    # Add object file to the list
    OBJ_FILES+=("$obj")
}

# Specify dependencies
# dep "$SCRIPT_DIR/../files/filemtime_duration.cpp"
# dep "$SCRIPT_DIR/../files/file_get_contents.cpp"
# dep "$SCRIPT_DIR/../files/file_put_contents.cpp"
dep "$SCRIPT_DIR/../str/explode.cpp"
dep "$SCRIPT_DIR/../str/implode.cpp"
dep "$SCRIPT_DIR/../str/remove_path.cpp"
dep "$SCRIPT_DIR/../str/remove_extension.cpp"
dep "$SCRIPT_DIR/../str/replace_extension.cpp"
dep "$SCRIPT_DIR/../str/str_starts_with.cpp"
dep "$SCRIPT_DIR/../str/str_replace.cpp"
dep "$SCRIPT_DIR/../str/str_cut_end.cpp"
dep "$SCRIPT_DIR/../str/compare_diff_vectors.cpp"
dep "$SCRIPT_DIR/../str/str_get_diffs.cpp"
dep "$SCRIPT_DIR/../str/str_show_diff.cpp"
dep "$SCRIPT_DIR/../str/str_diffs_show.cpp"
dep "$SCRIPT_DIR/../str/parse.cpp"
dep "$SCRIPT_DIR/../str/get_hash.cpp"
dep "$SCRIPT_DIR/../str/str_contains.cpp"
dep "$SCRIPT_DIR/../utils/ANSI_FMT.cpp"
dep "$SCRIPT_DIR/../utils/Arguments.cpp"
dep "$SCRIPT_DIR/../utils/ERROR.cpp"
dep "$SCRIPT_DIR/../utils/JSON.cpp"
dep "$SCRIPT_DIR/../utils/JSONExts.cpp"
dep "$SCRIPT_DIR/../utils/Settings.cpp"
dep "$SCRIPT_DIR/../utils/io.cpp"
dep "$SCRIPT_DIR/../utils/Test.cpp"

# Compile with the specified or default build path and flags
CMD="g++ \"$SCRIPT_DIR/compile.cpp\" -o \"$BUILD_PATH/compile\" ${OBJ_FILES[*]} $FLAGS"
echo "$CMD"
eval "$CMD"

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Build successful: $BUILD_PATH/compile (mode: $BUILD_MODE)"
    if [ "$BUILD_MODE" = "test" ]; then
        echo "Executing $BUILD_PATH/compile as test..."
        "$BUILD_PATH/compile"
        # Check if the previous command succeeded (exit status 0)
        if [ $? -eq 0 ]; then
            echo "Generating coverage report..."
            eval "$SCRIPT_DIR/gencov.sh $SCRIPT_DIR/compile.cpp $BUILD_PATH/compile"
        else
            echo "Skipping coverage report due to test failure."
        fi
    fi
else
    echo "Build failed (mode: $BUILD_MODE)"
    exit 1
fi