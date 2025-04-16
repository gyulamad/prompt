#!/bin/bash

echo "Clean up..."
./clean.sh

echo "Compile..."
src/tools/build/bin/compile src/prompt.cpp --config=coverage --buildcache=false --verbose && \
builds/prompt && \
src/tools/build/gencov.sh ./src/prompt.cpp ./builds/prompt coverage "**/libs/**,**/src/prompt.cpp"
