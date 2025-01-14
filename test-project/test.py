#!/usr/bin/env python3
import random

secret_number = random.randint(1, 100)
guesses_left = 7

print("Welcome to the Number Guessing Game!")
print("I'm thinking of a number between 1 and 100.")

while guesses_left > 0:
    print(f"\nYou have {guesses_left} guesses left.")
    try:
        guess = int(input("Take a guess: "))
    except ValueError:
        print("Invalid input. Please enter a number.")
        continue

    if guess < secret_number:
        print("Too low!")
    elif guess > secret_number:
        print("Too high!")
    else:
        print(f"\nCongratulations! You guessed the number in {7 - guesses_left} tries!")
        break

    guesses_left -= 1

if guesses_left == 0:
    print(f"\nYou ran out of guesses. The number was {secret_number}.")
