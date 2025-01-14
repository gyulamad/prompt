#!/bin/bash

i=1
while IFS= read -r todo; do
  echo "|"
  i=1
done > todos.txt
echo "todos.txt updated successfully!"
