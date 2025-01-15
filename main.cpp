#include <stdio.h>
#include <string.h>
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"

#define BLOCK_SIZE 8 // 8x8 pixel block - default block for characters and snake segments
#define SCREEN_WIDTH 640 // [px], 80 blocks wide - when changed value should be divisible by BLOCK_SIZE
#define SCREEN_HEIGHT 480 // [px], 60 blocks high - when changed value should be divisible by BLOCK_SIZE
#define STATUSBAR_HEIGHT 40 // [px], 5 blocks high - status bar at the top of the screen

#define BLUE 0x0000FF
#define GREEN 0x00FF00
#define RED 0xFF0000
#define BLACK 0x000000

#define INITIAL_SNAKE_LENGTH 5
#define SNAKE_SPEED 0.5 // [s] per move

enum direction_t {
	UP,
	DOWN,
	LEFT,
	RIGHT
};

typedef struct {
	int headX, headY;
	int length;
	int *tailX, *tailY;
	int colour;
	direction_t direction;
} snake_t;

void RefreshScreen(SDL_Surface* screen, SDL_Texture* screenTexture, SDL_Renderer* renderer)
{
	SDL_UpdateTexture(screenTexture, NULL, screen->pixels, screen->pitch);
	SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

// Draw a rectangle on the screen starting at (x, y) with the specified width, height and color
void DrawRect(SDL_Surface* screen, int x, int y, int width, int height, int color)
{
	SDL_Rect rect = { x, y, width, height };
	SDL_FillRect(screen, &rect, color);
}

void DrawChar(SDL_Surface* screen, int x, int y, char ch, SDL_Surface* charset)
{
	SDL_Rect srect = { (ch % 16) * 8, (ch / 16) * 8, 8, 8 };
	SDL_Rect drect = { x, y, 8, 8 };
	SDL_BlitSurface(charset, &srect, screen, &drect);
}

void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset)
{
	int origX = x;
	while (*text)
	{
		if (*text == '\n')
		{
			x = origX;
			y += BLOCK_SIZE;
		}
		else
		{
			DrawChar(screen, x, y, *text, charset);
			x += BLOCK_SIZE;
		}
		text++;
	}
}

void DrawFrame(SDL_Surface* screen, int colour)
{
	DrawRect(screen, 0, 0, SCREEN_WIDTH, STATUSBAR_HEIGHT, colour); // status bar
	DrawRect(screen, 0, STATUSBAR_HEIGHT, BLOCK_SIZE, SCREEN_HEIGHT, colour); // left border
	DrawRect(screen, SCREEN_WIDTH - BLOCK_SIZE, STATUSBAR_HEIGHT, BLOCK_SIZE, SCREEN_HEIGHT, colour); // right border
	DrawRect(screen, 0, SCREEN_HEIGHT - BLOCK_SIZE, SCREEN_WIDTH, BLOCK_SIZE, colour); // bottom border
}

snake_t* SnakeInit(int colour)
{
	snake_t* snake;
	snake = (snake_t*)malloc(sizeof(snake_t));
	snake->colour = colour;
	snake->direction = UP;
	snake->headX = SCREEN_WIDTH / 2;
	snake->headY = SCREEN_HEIGHT / 2;
	snake->length = INITIAL_SNAKE_LENGTH;
	snake->tailX = (int*)malloc(INITIAL_SNAKE_LENGTH * sizeof(int));
	snake->tailY = (int*)malloc(INITIAL_SNAKE_LENGTH * sizeof(int));
	snake->tailX[0] = snake->headX;
	snake->tailY[0] = snake->headY + BLOCK_SIZE;
	for (int i = 1; i < INITIAL_SNAKE_LENGTH; i++)
	{
		snake->tailX[i] = snake->headX;
		snake->tailY[i] = snake->tailY[i - 1] + BLOCK_SIZE;
	}
	return snake;
}

void SnakeDraw(SDL_Surface* screen, snake_t* snake)
{
	DrawRect(screen, snake->headX, snake->headY, BLOCK_SIZE, BLOCK_SIZE, snake->colour);
	for (int i = 0; i < snake->length - 1; i++)
	{
		DrawRect(screen, snake->tailX[i], snake->tailY[i], BLOCK_SIZE, BLOCK_SIZE, snake->colour);
	}
	DrawRect(screen, snake->tailX[snake->length - 1], snake->tailY[snake->length - 1], BLOCK_SIZE, BLOCK_SIZE, BLACK);
}

direction_t Turn(direction_t direction, direction_t rotation)
{
	if (rotation == LEFT)
	{
		switch (direction)
		{
		case UP:
			return LEFT;
		case DOWN:
			return RIGHT;
		case LEFT:
			return DOWN;
		case RIGHT:
			return UP;
		}
	}
	else if (rotation == RIGHT)
	{
		switch (direction)
		{
		case UP:
			return RIGHT;
		case DOWN:
			return LEFT;
		case LEFT:
			return UP;
		case RIGHT:
			return DOWN;
		}
	}
}

void WallCheck(snake_t* snake)
{
	// top wall:
	if (snake->headY == STATUSBAR_HEIGHT && snake->direction == UP)
	{
		// check if right turn is possible:
		if (snake->headX == SCREEN_WIDTH - 2 * BLOCK_SIZE) // snake head is next to right wall (right turn is impossible)
		{
			snake->direction = Turn(snake->direction, LEFT);
		}
		else // right turn is possible
		{
			snake->direction = Turn(snake->direction, RIGHT);
		}
	}
	// right wall:
	else if (snake->headX == SCREEN_WIDTH - 2 * BLOCK_SIZE && snake->direction == RIGHT)
	{
		if (snake->headY == SCREEN_HEIGHT - 2 * BLOCK_SIZE)
		{
			snake->direction = Turn(snake->direction, LEFT);
		}
		else
		{
			snake->direction = Turn(snake->direction, RIGHT);
		}
	}
	// bottom wall:
	else if (snake->headY == SCREEN_HEIGHT - 2 * BLOCK_SIZE && snake->direction == DOWN)
	{
		if (snake->headX == 2 * BLOCK_SIZE)
		{
			snake->direction = Turn(snake->direction, LEFT);
		}
		else
		{
			snake->direction = Turn(snake->direction, RIGHT);
		}
	}
	// left wall:
	else if (snake->headX == BLOCK_SIZE && snake->direction == LEFT)
	{
		if (snake->headY == STATUSBAR_HEIGHT)
		{
			snake->direction = Turn(snake->direction, LEFT);
		}
		else
		{
			snake->direction = Turn(snake->direction, RIGHT);
		}
	}
}

void SnakeMove(snake_t* snake)
{
	for (int i = snake->length - 1; i > 0; i--)
	{
		snake->tailX[i] = snake->tailX[i - 1];
		snake->tailY[i] = snake->tailY[i - 1];
	}
	snake->tailX[0] = snake->headX;
	snake->tailY[0] = snake->headY;
	switch (snake->direction)
	{
	case UP:
		snake->headY -= BLOCK_SIZE;
		break;
	case DOWN:
		snake->headY += BLOCK_SIZE;
		break;
	case LEFT:
		snake->headX -= BLOCK_SIZE;
		break;
	case RIGHT:
		snake->headX += BLOCK_SIZE;
		break;
	}
}

void MainLoop(snake_t* snake, SDL_Surface* screen, SDL_Surface* charset, SDL_Texture* screenTexture, SDL_Renderer* renderer)
{
	int quit = 0;

	// time management setup:
	int t1, t2, frames;
	double delta, worldTime, fpsTimer, fps, moveTimer;
	t1 = SDL_GetTicks();
	frames = 0; fpsTimer = 0; fps = 0; worldTime = 0, moveTimer = 0;
	
	// game loop:
	while (!quit)
	{
		// time management:
		t2 = SDL_GetTicks();
		delta = (t2 - t1) * 0.001; // delta time in seconds
		t1 = t2;
		worldTime += delta; fpsTimer += delta; moveTimer += delta; // update timing variables
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		}

		// status bar:
		char text[128];
		DrawFrame(screen, BLUE); // clear status bar
		sprintf(text, "elapsed time = %.1lf s     %.0lf frames / s", worldTime, fps);
		DrawString(screen, BLOCK_SIZE, BLOCK_SIZE, text, charset);
		sprintf(text, "implemented requirements: 12");
		DrawString(screen, BLOCK_SIZE, 2*BLOCK_SIZE, text, charset);


		// snake movement:
		if (moveTimer > SNAKE_SPEED)
		{
			WallCheck(snake); // check for collision with walls; adjust direction if necessary
			SnakeMove(snake); // move the snake (update its position parameters)
			SnakeDraw(screen, snake); // update snake's position on the screen
			moveTimer -= SNAKE_SPEED; // reset move timeout
		}

		// screen refresh:
		RefreshScreen(screen, screenTexture, renderer);

		// user input:
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				quit = 1;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					quit = 1;
					break;
				case SDLK_n:
					snake = SnakeInit(GREEN);
					SDL_FillRect(screen, NULL, BLACK);
					DrawFrame(screen, BLUE);
					MainLoop(snake, screen, charset, screenTexture, renderer);
					quit = 1;
					break;
				case SDLK_UP:
					snake->direction = UP;
					break;
				case SDLK_DOWN:
					snake->direction = DOWN;
					break;
				case SDLK_LEFT:
					snake->direction = LEFT;
					break;
				case SDLK_RIGHT:
					snake->direction = RIGHT;
					break;
				}
			}
			
		}
		frames++;
	}
}

int main(int argc, char** argv)
{
	int rc; // return code for window and renderer creation
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Surface* screenSurface, * charset;
	SDL_Texture* screenTexture;

	// ----------SDL initialization:----------

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("Error initializing SDL: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if (rc != 0) // window and renderer: returns 0 on success else non-zero
	{
		SDL_Quit();
		printf("Error creating window and renderer: %s\n", SDL_GetError());
		return 1;
	}
	SDL_SetWindowTitle(window, "The Snake Game");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

	screenSurface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
	screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	charset = SDL_LoadBMP("./cs8x8.bmp"); // 8x8 pixel font
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screenSurface);
		SDL_DestroyTexture(screenTexture);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	}
	SDL_SetColorKey(charset, 1, 0);

	SDL_ShowCursor(SDL_DISABLE);

	// ----------Game initialization:----------

	SDL_FillRect(screenSurface, NULL, BLACK);
	DrawFrame(screenSurface, BLUE);

	RefreshScreen(screenSurface, screenTexture, renderer);

	snake_t* snake = SnakeInit(GREEN);

	SnakeDraw(screenSurface, snake);
	RefreshScreen(screenSurface, screenTexture, renderer);

	// TODO: before-game status bar (press any key to start; input explanation)

	MainLoop(snake, screenSurface, charset, screenTexture, renderer);

	// ----------Game cleanup:----------

	SDL_DestroyWindow(window);
	window = NULL;
	screenSurface = NULL;
	SDL_Quit();

	return 0;
}