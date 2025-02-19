#!/bin/bash

# echo "Compile executable..."
g++ -std=c++23 ./src/prompt.cpp -o ./builds/prompt -g -O0 -I./libs/ggerganov/whisper.cpp/include -I./libs/ggerganov/whisper.cpp/ggml/include ./libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4 -Wl,-rpath,./libs/ggerganov/whisper.cpp/build/src -lrt -lm -lasound -lportaudio -lcurl -pthread
