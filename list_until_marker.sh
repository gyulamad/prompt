#!/bin/bash

# Example:  ./list_until_marker.sh "src/tools/events/*.hpp" "#ifdef TEST"

pattern="$1"
end_marker="$2"

if [ -z "$pattern" ] || [ -z "$end_marker" ]; then
  echo "Usage: $0 <file_pattern> <end_marker>"
  exit 1
fi

for file in $pattern; do
  if [ -f "$file" ]; then
    echo "===== $file:"
    while IFS= read -r line; do
      if [[ "$line" == "$end_marker" ]]; then
        break
      fi
      echo "$line"
    done < "$file"
  fi
done