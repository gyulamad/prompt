#!/bin/bash

# Get the username from the command line argument
username="$1"

# Check if a username was provided
if [ -z "$username" ]; then
  echo "Usage: $0 <username>"
  exit 1
fi

# Check if the user exists
if id -u "$username" >/dev/null 2>&1; then
  # User exists, delete the user
  sudo deluser "$username"
  echo "User '$username' deleted."
else
  echo "User '$username' does not exist."
fi