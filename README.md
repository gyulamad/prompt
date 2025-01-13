# Project Name

This document provides instructions to build and run the project.

## Prerequisites

* nlohmann/json: Clone using `git clone https://github.com/nlohmann/json` (Install in a suitable location)

## Build Instructions

To build the project, follow these steps:

1. **Compile:** `g++ prompt.cpp -o prompt`
2. **Run:** `./prompt`


## Deployment Instructions

To deploy the executable for easy access:

1. **Copy:** `cp ./prompt ~/bin/prompt`
2. **(Optional) Add to PATH:** Add `~/bin` to your `PATH` environment variable (e.g., in your `.bashrc` file: `export PATH="$PATH:~/bin"`). This allows you to run `prompt` from any directory.


## Testing Instructions

To run the tests:

1. **Compile:** `g++ tests.cpp -o tests`
2. **Run:** `./tests`

**Note:** Address the "Error: Unable to open file for reading (agents/default)" error in `tests.cpp`.

