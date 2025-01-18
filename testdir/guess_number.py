import random
number = random.randint(1, 100)
guess = 0
while guess != number:
    try:
        guess = int(input('Guess a number between 1 and 100: '))
        if guess < number:
            print('Too low!')
        elif guess > number:
            print('Too high!')
    except ValueError:
        print('Invalid input. Please enter a number.')
print('Congratulations! You guessed the number.')
