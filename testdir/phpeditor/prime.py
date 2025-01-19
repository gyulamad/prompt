def is_prime(n):
  """Ellenőrzi, hogy egy szám prím-e."""
  if n <= 1:
    return False
  for i in range(2, int(n**0.5) + 1):
    if n % i == 0:
      return False
  return True

try:
  num = int(input("Adj meg egy egész számot: "))
  if is_prime(num):
    print(f"{num} prím szám.")
  else:
    print(f"{num} nem prím szám.")
except ValueError:
  print("Hibás bemenet. Kérlek adj meg egy egész számot.")
