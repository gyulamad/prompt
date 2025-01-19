import sys

try:
    number = float(sys.argv[1])
    result = number * 2
    print(f"A szám kétszerese: {result}")
except (IndexError, ValueError):
    print("Hibás bemenet. Kérlek adj meg egy számot argumentumként.")
