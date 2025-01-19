import random

def guess_the_number():
    number = random.randint(1, 100)
    guesses_left = 7
    print("Gondoltam egy számra 1 és 100 között. 7 tipped van.")
    while guesses_left > 0:
        try:
            guess = input(f"Még {guesses_left} tipped van. Tippeld meg a számot, vagy írd be, hogy 'kilépés': ")
            if guess.lower() == "kilépés":
                print("Kilépés a játékból.")
                return
            guess = int(guess)
        except ValueError:
            print("Nem számot adtál meg!")
            continue
        if guess < number:
            print("Nagyobb!")
        elif guess > number:
            print("Kisebb!")
        else:
            print(f"Gratulálok! A szám {number} volt.")
            return
        guesses_left -= 1
    print(f"Sajnos elfogytak a tippjeid. A szám {number} volt.")

guess_the_number()

while True:
    play_again = input("Játszani szeretnél még? (igen/nem): ")
    if play_again.lower() == "igen":
        guess_the_number()
    else:
        break
