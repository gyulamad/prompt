#!/bin/bash

# ./clean.sh
# cppcheck . -i fltk && \
# g++ compile.cpp -o compile && \
# ./compile tests/test --debug --coverage && \
# ./tests/test && \
# gcov -i tests/test.cpp && \
# lcov --capture --directory . --output-file coverage.info && \
# php lcov-fixer.php coverage.info && \
# lcov --remove coverage.info '/usr/*' '/opt/*' '/system/*' --output-file coverage.info && \
# genhtml coverage.info --output-directory coverage --dark-mode && \
# brave-browser coverage/index.html

#!/bin/bash

# Check if at least source file and executable are provided
if [ $# -lt 2 ]; then
    echo "Usage: $0 <source_file> <executable> [<output_folder>] [<exclusions>]"
    echo "Example: $0 tests/test.cpp builds/test coverage 'tests/*,third_party/*'"
    exit 1
fi

SOURCE_FILE="$1"
EXECUTABLE="$2"
OUTPUT_FOLDER="${3:-coverage}"  # Default to "coverage" if not provided
EXCLUSIONS="${4}"  # Optional exclusions (comma-separated patterns)

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Verify source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file '$SOURCE_FILE' does not exist."
    exit 1
fi

# Verify executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable '$EXECUTABLE' does not exist."
    exit 1
fi

# Derive the directory containing the executable (for .gcda and .gcno files)
EXEC_DIR=$(dirname "$EXECUTABLE")

# Create the output folder if it doesn't exist
mkdir -p "$OUTPUT_FOLDER" || {
    echo "Failed to create output folder '$OUTPUT_FOLDER'"
    exit 1
}

# Step 1: Run gcov on the source file and move output to the specified folder
echo "Processing $SOURCE_FILE with gcov, outputting to $OUTPUT_FOLDER..."
gcov -j "$SOURCE_FILE" -o "$EXEC_DIR" || {
    echo "gcov failed"
    exit 1
}
# Move .gcov files to the output folder
mv *.gcov* "$OUTPUT_FOLDER/" 2>/dev/null || {
    echo "Warning: No .gcov files generated or failed to move them"
}

# Step 2: Capture coverage data from the executable's directory
echo "Capturing coverage data with lcov..."
lcov --capture --directory "$EXEC_DIR" --output-file "$OUTPUT_FOLDER/coverage.info" || {
    echo "lcov capture failed"
    exit 1
}

# Step 3: Apply user-specified exclusions (if provided)
if [ -n "$EXCLUSIONS" ]; then
    echo "Applying exclusions: $EXCLUSIONS..."
    # Convert comma-separated exclusions to space-separated for lcov
    EXCLUDE_PATTERNS=$(echo "$EXCLUSIONS" | tr ',' ' ')
    lcov --remove "$OUTPUT_FOLDER/coverage.info" $EXCLUDE_PATTERNS --output-file "$OUTPUT_FOLDER/coverage.info" || {
        echo "lcov exclusion failed"
        exit 1
    }
fi

# Step 4: Fix coverage data using the script's directory for lcov-fixer.php
echo "Fixing coverage data with lcov-fixer.php..."
php "$SCRIPT_DIR/lcov-fixer.php" "$OUTPUT_FOLDER/coverage.info" || {
    echo "lcov-fixer.php failed"
    exit 1
}

# Step 5: Remove system paths
echo "Filtering system paths from "$OUTPUT_FOLDER/coverage.info"..."
lcov --remove "$OUTPUT_FOLDER/coverage.info" '/usr/*' '/opt/*' '/system/*' --output-file "$OUTPUT_FOLDER/coverage.info" || {
    echo "lcov remove failed"
    exit 1
}

# Step 6: Generate HTML report
echo "Generating HTML report with genhtml..."
genhtml "$OUTPUT_FOLDER/coverage.info" --output-directory "$OUTPUT_FOLDER" --dark-mode || {
    echo "genhtml failed"
    exit 1
}

# Step 7: Open the report in Brave browser
echo "Opening coverage report in Brave browser..."
brave-browser "$OUTPUT_FOLDER/index.html" || {
    echo "Failed to open browser (report still generated in '$OUTPUT_FOLDER/')"
    exit 1
}

echo "Coverage report generated successfully in '$OUTPUT_FOLDER/'"