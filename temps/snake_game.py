import pygame
import random

# Inicializálás
pygame.init()

# Ablak mérete
SIZE = 20
WIDTH = 600
HEIGHT = 400
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Kígyó Játék")

# Színek
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
RED = (255, 0, 0)
GREEN = (0, 255, 0)

# Kígyó kezdő pozíciója és hossza
snake_pos = [[WIDTH//2, HEIGHT//2]]
snake_dir = "RIGHT"
snake_speed = 5

# Kaja
food_pos = [random.randrange(1, WIDTH // SIZE) * SIZE, random.randrange(1, HEIGHT // SIZE) * SIZE]
food_spawn = True

# Fő loop
clock = pygame.time.Clock()
score = 0
game_over = False

def game_screen():
    # Kígyó mozgatása
    global snake_dir, food_pos, food_spawn, score, game_over

    new_head = list(snake_pos[0])
    if snake_dir == "RIGHT":
        new_head[0] += SIZE
    if snake_dir == "LEFT":
        new_head[0] -= SIZE
    if snake_dir == "UP":
        new_head[1] -= SIZE
    if snake_dir == "DOWN":
        new_head[1] += SIZE

    snake_pos.insert(0, new_head)

    # Kaja evés
    if abs(snake_pos[0][0] - food_pos[0]) < SIZE and abs(snake_pos[0][1] - food_pos[1]) < SIZE:
        food_spawn = False
        score += 1
    else:
        snake_pos.pop()

    # Kaja új helyre
    if not food_spawn:
        food_pos = [random.randrange(1, WIDTH // SIZE) * SIZE, random.randrange(1, HEIGHT // SIZE) * SIZE]
    food_spawn = True


    # Falütközés
    if snake_pos[0][0] < 0 or snake_pos[0][0] > WIDTH - SIZE or snake_pos[0][1] < 0 or snake_pos[0][1] > HEIGHT - SIZE:
        game_over = True

    # Önmaga beleütközés
    for block in snake_pos[1:]:
        if abs(snake_pos[0][0] - block[0]) < SIZE and abs(snake_pos[0][1] - block[1]) < SIZE:
            game_over = True
            break

    # Rajzolás
    screen.fill(BLACK)
    for pos in snake_pos:
        pygame.draw.rect(screen, GREEN, pygame.Rect(pos[0], pos[1], SIZE, SIZE))
    pygame.draw.rect(screen, RED, pygame.Rect(food_pos[0], food_pos[1], SIZE, SIZE))

    # Pontszám
    font = pygame.font.Font(None, 36)
    text = font.render(f"Pontszám: {score}", True, WHITE)
    screen.blit(text, (10, 10))

    # Game over üzenet
    if game_over:
        game_over_font = pygame.font.Font(None, 72)
        game_over_text = game_over_font.render("Game Over", True, WHITE)
        text_rect = game_over_text.get_rect(center=(WIDTH // 2, HEIGHT // 2))
        screen.blit(game_over_text, text_rect)
        
    pygame.display.flip()

    

while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            quit()
        if not game_over:
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_UP and snake_dir != "DOWN":
                    snake_dir = "UP"
                if event.key == pygame.K_DOWN and snake_dir != "UP":
                    snake_dir = "DOWN"
                if event.key == pygame.K_LEFT and snake_dir != "RIGHT":
                   snake_dir = "LEFT"
                if event.key == pygame.K_RIGHT and snake_dir != "LEFT":
                     snake_dir = "RIGHT"

    if not game_over:
       game_screen()
    else:
       screen.fill(BLACK)
       font = pygame.font.Font(None, 72)
       text = font.render(f"Game Over - Pontszám: {score}", True, WHITE)
       text_rect = text.get_rect(center=(WIDTH // 2, HEIGHT // 2))
       screen.blit(text, text_rect)
       pygame.display.flip()


    clock.tick(snake_speed)