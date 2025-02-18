#!/bin/bash

# Get the username from the command line argument
username="$1"

# Check if a username was provided
if [ -z "$username" ]; then
  echo "Usage: $0 <username>"
  exit 1
fi


# User doesn't exist, create the user
sudo adduser "$username"

# Set home directory ownership
sudo chown "$username":"$username" /home/"$username"

# Remove from sudo group (if present)
sudo deluser "$username" sudo || true # Ignore errors if not in sudo

echo "User '$username' created with limited access."


./compile.sh
sudo mkdir /home/"$username"/prompt
sudo mkdir /home/"$username"/prompt/builds
sudo mkdir /home/"$username"/prompt/libs
sudo mkdir /home/"$username"/prompt/libs/ggerganov
sudo mkdir /home/"$username"/prompt/libs/ggerganov/whisper.cpp
sudo mkdir /home/"$username"/prompt/libs/ggerganov/whisper.cpp/build
sudo mkdir /home/"$username"/prompt/libs/ggerganov/whisper.cpp/build/src
sudo mkdir /home/"$username"/prompt/libs/ggerganov/whisper.cpp/models
sudo mkdir /home/"$username"/prompt/.prompt
sudo mkdir /home/"$username"/prompt/.prompt/models
sudo mkdir /home/"$username"/prompt/sounds
sudo mkdir /home/"$username"/prompt/sounds/r2d2
sudo cp ./config.json /home/"$username"/prompt/config.json
sudo cp ./config.gemini.json /home/"$username"/prompt/config.gemini.json
sudo cp ./builds/prompt /home/"$username"/prompt/builds/prompt
sudo cp ./sounds/r2d2/*.wav /home/"$username"/prompt/sounds/r2d2/
sudo cp ./libs/ggerganov/whisper.cpp/build/src/libwhisper.so /home/"$username"/prompt/libs/ggerganov/whisper.cpp/build/src/libwhisper.so
sudo cp ./libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1 /home/"$username"/prompt/libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1
sudo cp ./libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4 /home/"$username"/prompt/libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4
sudo cp ./libs/ggerganov/whisper.cpp/models/ggml-base-q8_0.bin /home/"$username"/prompt/libs/ggerganov/whisper.cpp/models/ggml-base-q8_0.bin

# Add whisper.cpp
sudo ldconfig /home/"$username"/prompt/libs/ggerganov/whisper.cpp/build/src/

# Set permissions on home directory
sudo chmod -R 777 /home/"$username"

# echo "Switch to the new user...: $ su - $username"
# echo "Or SSH into..............: $ ssh $username@localhost"
# echo "And run..................: $ ./prompt/prompt"

# echo -e '\npcm.!default {\n    type plug\n    slave.pcm "plughw:0,0"\n}\nctl.!default {\n    type hw\n    card 0\n}\n' >> /home/"$username"/.asoundrc
# sudo modprobe snd_hda_intel
# echo "Run: ./prompt/prompt --voice --model prompt1"
su - $username