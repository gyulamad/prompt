ctags --output-format=json -R src/tools | grep -Ev '"(name|scope)": "test_' | grep '"kind": "function"' > ctags.json
