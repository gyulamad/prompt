#!/bin/bash

# Example: ./list_until_marker.sh "src/tools/events" ".hpp" "#ifdef TEST"
# Example: ./list_until_marker.sh "src/tools/events" ".cpp"

directory="$1"
file_extension="$2"
end_marker="$3"

if [ -z "$directory" ] || [ -z "$file_extension" ]; then
  echo "Usage: $0 <directory> <file_extension> [end_marker]"
  exit 1
fi

find "$directory" -type f -name "*$file_extension" -print0 | while IFS= read -r -d $'\0' file; do
  if [ -f "$file" ]; then
    echo "===== $file:"
    if [ -n "$end_marker" ]; then # Check if marker is provided
      found_marker=0
      while IFS= read -r line || [[ -n "$line" ]]; do
        if [[ "$line" == "$end_marker" ]]; then
          found_marker=1
          break
        fi
        echo "$line"
      done < "$file"
      if [ "$found_marker" -eq 0 ]; then
        echo "Marker not found in file: $file"
      fi
    else # No marker, show full file
      cat "$file"
    fi
  fi
done