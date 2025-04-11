#!/bin/bash
./clean.sh
# --verbose && \
src/tools/build/bin/compile src/prompt.cpp --config=coverage --buildcache=false && \
builds/prompt && \
src/tools/build/gencov.sh ./src/prompt.cpp ./builds/prompt coverage "/mnt/windows/llm/prompt/libs/K-Adam/SafeQueue/SafeQueue.hpp,/mnt/windows/llm/prompt/libs/nlohmann/json/single_include/nlohmann/*,/mnt/windows/llm/prompt/libs/yhirose/cpp-linenoise/*,/mnt/windows/llm/prompt/src/*"
