import os
import re
from pathlib import Path

def parse_gitignore(gitignore_path):
    ignored_patterns = []
    if gitignore_path.exists():
        with gitignore_path.open('r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    ignored_patterns.append(line)
    return ignored_patterns

def is_ignored(filepath, ignored_patterns):
    filepath = str(filepath) # Convert Path object to string for regex
    for pattern in ignored_patterns:
        if re.search(pattern, filepath):
            return True
    return False

def find_todos(directory, ignored_patterns):
    todos = []
    for path in Path(directory).rglob('*'):
        if path.is_file() and not is_ignored(path, ignored_patterns):
            try:
                with path.open('r') as f:
                    for i, line in enumerate(f, 1):
                        match = re.search(r'TODO\s*(.*)', line)
                        if match:
                            todos.append(f"{path}:{i} {match.group(1).strip()}")
            except UnicodeDecodeError:
                print(f"Skipping binary file: {path}")
    return todos

def update_todos_file(todos):
    with open('todos.txt', 'w') as f:
        for i, todo in enumerate(todos, 1):
            f.write(f"{i}|{todo}\n")

if __name__ == "__main__":
    gitignore_path = Path('.gitignore')
    ignored_patterns = parse_gitignore(gitignore_path)
    todos = find_todos('.', ignored_patterns)
    update_todos_file(todos)
    print("todos.txt updated successfully!")

