git clone https://github.com/nlohmann/json libs/nlohmann/json
git clone https://github.com/yhirose/cpp-linenoise.git libs/yhirose/cpp-linenoise
git clone https://github.com/ggerganov/whisper.cpp.git libs/ggerganov/whisper.cpp
cd libs/ggerganov/whisper.cpp
# Build without CUDA (default is CPU-only)
mkdir build
cd build
cmake ..
make -j4
cd ..
./models/download-ggml-model.sh base  # Supported models: tiny, base, small, medium, large.
./build/bin/quantize models/ggml-base.bin models/ggml-base-q8_0.bin q8_0  # Quantize the model to 5-bit (q5_0)
cd ../..
pwd
sudo ldconfig libs/ggerganov/whisper.cpp/build/src/
cp example.config.gemini.json config.gemini.json
cp browse/example.google-search-secret.json browse/google-search-secret.json
cp example.config.json config.json
nano config.gemini.json
nano browse/google-search-secret.json
nano config.json
chmod +x compile.sh
./compile.sh
