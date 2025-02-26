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


# ---- INSTALL TOOLS ----
echo "Preparing tools for bot function calls..."
cd browse
npm i
cd ..
echo 0 | sudo tee /proc/sys/kernel/apparmor_restrict_unprivileged_userns


# ---- CREATE A USER FOR BOT ----
# echo "Create user for AI..."

# # TODO: remove the AI user all together...
# # Prompt the user for a username, default to 'promptuser1' if empty
# read -p "Enter the username (default: ai): " username
# username=${username:-ai}
# # Prompt the user for a hostname, default to 'localhost' if empty
# read -p "Enter the hostname (default: localhost): " hostname
# hostname=${hostname:-localhost}
# # Create AI-user common group for file sharing
# read -p "Enter the AI-user common group for file sharing (default: ai-group): " group
# username=${group:-ai-group}
# # Add the user
# sudo adduser "$username"
# #sudo ssh-keygen -t rsa -b 2048 -f "/home/$username/.ssh/id_rsa" -N ""
# #sudo ssh-copy-id -i ~/.ssh/id_rsa.pub ai@localhost
# #sudo chgrp ai-group /home/ai
# #sudo chown ai:ai /home/ai/.ssh/authorized_keys

# # Setup SSH access for AI

# # # Generate SSH key
# # ssh-keygen -t rsa -b 2048 -f "/home/$username/.ssh/id_rsa" -N ""
# # # Copy the SSH key to the user's authorized_keys on the specified host
# # ssh-copy-id "$username@$hostname"
# # # Optionally, you can SSH into the user's account
# # # ssh "$username@$hostname"


# # As root (or a sudoer):
# sudo groupadd "$group"  # If the group doesn't exist
# sudo usermod -a -G $group $USER
# sudo usermod -a -G $group $username
# sudo chown $username:$group /home/$username
# sudo chmod g+s /home/$username
# sudo chmod 775 /home/$username

# # Add accsess to the AI's home folder
# sudo setfacl -m u:$USER:rwx "/home/$username"


# echo "User '$username' created and SSH key configured for host '$hostname'."

# ---- CONFIG SETUP ----
echo "Config setup..."

cp example.config.json config.json
# Use sed to replace placeholders in the file
# sed -i "s/TOOL_BASH_COMMAND_SSH_USERNAME/$username/g" config.json
# sed -i "s/TOOL_BASH_COMMAND_SSH_HOSTNAME/$hostname/g" config.json
# sed -i "s/TOOL_FILE_MANAGER_GROUP/$group/g" config.json

read -p "Enter default base folder (workspace directory) for file manager tool (default: /home/$USER/.prompt/workspace): " default_base_folder
username=${default_base_folder:-/home/$USER/.prompt/workspace}
sed -i "s/TOOL_FILE_MANAGER_BASE_FOLDER/$default_base_folder/g" config.json
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
