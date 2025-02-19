# ---- ADDING DEPENDENCIES ----
echo "Adding dependencies..."

# nlohmann/json
git clone https://github.com/nlohmann/json libs/nlohmann/json

# yhirose/cpp-linenoise
git clone https://github.com/yhirose/cpp-linenoise.git libs/yhirose/cpp-linenoise

# ggerganov/whisper.cpp
git clone https://github.com/ggerganov/whisper.cpp.git libs/ggerganov/whisper.cpp
cd libs/ggerganov/whisper.cpp
# (Build without CUDA (default is CPU-only))
mkdir build
cd build
cmake ..
make -j4
cd ..
./models/download-ggml-model.sh base  # Supported models: tiny, base, small, medium, large.
./build/bin/quantize models/ggml-base.bin models/ggml-base-q8_0.bin q8_0  # Quantize the model to 5-bit (q5_0)
cd ../../..
sudo ldconfig libs/ggerganov/whisper.cpp/build/src/

# curl
curl/curl.h: sudo apt-get install libcurl4-openssl-dev

# portaudio
portaudio.h: sudo apt-get install portaudio19-dev



# ---- CREATE USER FOR BOT ----
echo "Create user for AI..."


# Prompt the user for a username, default to 'promptuser1' if empty
read -p "Enter the username (default: promptuser1): " username
username=${username:-promptuser1}
# Prompt the user for a hostname, default to 'localhost' if empty
read -p "Enter the hostname (default: localhost): " hostname
hostname=${hostname:-localhost}
# Add the user
sudo adduser "$username"
# Generate SSH key
ssh-keygen -t rsa -b 2048 -f "/home/$username/.ssh/id_rsa" -N ""
# Copy the SSH key to the user's authorized_keys on the specified host
ssh-copy-id "$username@$hostname"
# Optionally, you can SSH into the user's account
# ssh "$username@$hostname"

echo "User '$username' created and SSH key configured for host '$hostname'."

# ---- CONFIG SETUP ----
echo "Config setup..."

cp example.config.json config.json
# Use sed to replace placeholders in the file
sed -i "s/TOOL_BASH_COMMAND_SSH_USERNAME/$username/g" config.json
sed -i "s/TOOL_BASH_COMMAND_SSH_HOSTNAME/$hostname/g" config.json
# nano config.json

cp example.config.gemini.json config.gemini.json
# TODO ask user and replace with sed
nano config.gemini.json

cp browse/example.google-search-secret.json browse/google-search-secret.json
# TODO ask user and replace with sed
nano browse/google-search-secret.json


# ---- COMPILE ----
echo "Compile executable..."

./compile.sh
