#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"

#define BLOCK_SIZE 8 // 8x8 pixel block - default block for characters and snake segments
#define SCREEN_WIDTH 640 // [px], 80 blocks wide - when changed value should be divisible by BLOCK_SIZE
#define SCREEN_HEIGHT 480 // [px], 60 blocks high - when changed value should be divisible by BLOCK_SIZE
#define STATUSBAR_HEIGHT 40 // [px], 5 blocks high - status bar at the top of the screen
#define GAME_GRID_WIDTH ( ((SCREEN_WIDTH - 2 * BLOCK_SIZE) / BLOCK_SIZE) ) // number of blocks in the game grid width (excluding borders)
#define GAME_GRID_HEIGHT ( ((SCREEN_HEIGHT - (BLOCK_SIZE + STATUSBAR_HEIGHT)) / BLOCK_SIZE) ) // number of blocks in the game grid height (excluding borders)

#define BLUE 0x0000FF
#define GREEN 0x00FF00
#define RED 0xFF0000
#define BLACK 0x000000

#define INITIAL_SNAKE_LENGTH 5
#define SNAKE_SPEED 0.5 // [s] per move

#define RA(min, max) ( (min) + rand() % ((max) - (min) + 1) ) // random number between min and max (inc) - macro from 1st project demo game

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

typedef struct {
	int x, y;
} point_t;

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

// check if the point is colliding with the snake - return 1 if collision detected, 0 otherwise
int PointCollisionCheck(snake_t* snake, point_t* point)
{
	if (snake->headX == point->x && snake->headY == point->y)
	{
		return 1;
	}
	for (int i = 0; i < snake->length; i++)
	{
		if (snake->tailX[i] == point->x && snake->tailY[i] == point->y)
		{
			return 1;
		}
	}
	return 0;
}

// initialize a point with a random position on the screen, avoiding collision with the snake; for first point initialization, passed_value should be 0
point_t* PointInit(snake_t* snake)
{
	point_t* point;
	point = (point_t*)malloc(sizeof(point_t));
	point->x = BLOCK_SIZE + RA(0, GAME_GRID_WIDTH) * BLOCK_SIZE;
	point->y = STATUSBAR_HEIGHT + RA(0, GAME_GRID_HEIGHT) * BLOCK_SIZE;
	while (PointCollisionCheck(snake, point)) // avoid spawning the point on the snake
	{
		point->x = BLOCK_SIZE + RA(1, GAME_GRID_WIDTH) * BLOCK_SIZE;
		point->y = STATUSBAR_HEIGHT + RA(1, GAME_GRID_HEIGHT) * BLOCK_SIZE;
	}
	return point;
}

void PointDraw(SDL_Surface* screen, point_t* point)
{
	DrawRect(screen, point->x, point->y, BLOCK_SIZE, BLOCK_SIZE, RED);
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

// helper function for drawing the snake on the screen in a given colour
void SnakeDraw(SDL_Surface* screen, snake_t* snake, int colour)
{
	DrawRect(screen, snake->headX, snake->headY, BLOCK_SIZE, BLOCK_SIZE, colour);
	for (int i = 0; i < snake->length; i++)
	{
		DrawRect(screen, snake->tailX[i], snake->tailY[i], BLOCK_SIZE, BLOCK_SIZE, colour);
	}
}

// draw the snake on the screen
void SnakeAppear(SDL_Surface* screen, snake_t* snake)
{
	SnakeDraw(screen, snake, snake->colour);
}

// erase the snake from the screen
void SnakeDisappear(SDL_Surface* screen, snake_t* snake)
{
	SnakeDraw(screen, snake, BLACK);
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
		if (snake->headX == BLOCK_SIZE)
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

// check whether the snake has collided with itself - return 1 if collision detected, 0 otherwise
int SnakeCollisionCheck(snake_t* snake)
{
	for (int i = 0; i < snake->length; i++)
	{
		if (snake->headX == snake->tailX[i] && snake->headY == snake->tailY[i])
		{
			return 1;
		}
	}
	return 0;
}

void SnakeMove(snake_t* snake)
{
	for (int i = snake->length; i > 0; i--)
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
	char text[128];
	int gameover = 0; // variable to trigger game over mode (no snake movement, only n and Esc inputs accepted)

	// time management setup:
	int t1, t2, frames;
	double delta, worldTime, fpsTimer, fps, moveTimer;
	t1 = SDL_GetTicks();
	frames = 0; fpsTimer = 0; fps = 0; worldTime = 0, moveTimer = 0;

	// point system:
	int points = 0;
	point_t* point = PointInit(snake);
	
	
	// game loop:
	while (!quit)
	{
		if (!gameover) // status bar is not displayed and time doesn't run in game over mode
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
			DrawFrame(screen, BLUE); // clear status bar
			sprintf(text, "elapsed time = %.1lf s", worldTime);
			DrawString(screen, BLOCK_SIZE, BLOCK_SIZE, text, charset);
			sprintf(text, "%.0lf frames / s", fps);
			DrawString(screen, SCREEN_WIDTH / 2 - (strlen(text) * BLOCK_SIZE / 2), BLOCK_SIZE, text, charset);
			sprintf(text, "implemented requirements: 1234A");
			DrawString(screen, BLOCK_SIZE, 2*BLOCK_SIZE, text, charset);
			sprintf(text, "points: %d", points);
			DrawString(screen, SCREEN_WIDTH - (strlen(text) * BLOCK_SIZE + BLOCK_SIZE), BLOCK_SIZE, text, charset);
			
			PointDraw(screen, point); // draw the point on the screen
		}

		// snake movement:
		if (moveTimer > SNAKE_SPEED)
		{
			SnakeDisappear(screen, snake); // erase the snake at its previous position from the screen
			WallCheck(snake); // check for collision with walls; adjust direction if necessary
			SnakeMove(snake); // move the snake (update its position parameters)
			if (PointCollisionCheck(snake, point)) // check for collision with the point
			{
				points++;
				snake->length++;
				snake->tailX = (int*)realloc(snake->tailX, snake->length * sizeof(int)); // snake lengthening
				snake->tailY = (int*)realloc(snake->tailY, snake->length * sizeof(int));
				snake->tailX[snake->length - 1] = snake->tailX[snake->length - 2]; // add a new segment to the tail
				snake->tailY[snake->length - 1] = snake->tailY[snake->length - 2];
				point = PointInit(snake); // spawn a new point
			}
			if (SnakeCollisionCheck(snake)) // check for collision with itself
			{
				DrawFrame(screen, BLUE);
				sprintf(text, "GAME OVER! Press 'n' to start a new game, press 'Esc' to exit.");
				DrawString(screen, BLOCK_SIZE, BLOCK_SIZE, text, charset);
				sprintf(text, "game time: %.1lf s", worldTime);
				DrawString(screen, BLOCK_SIZE, 2*BLOCK_SIZE, text, charset);
				sprintf(text, "points: %d", points);
				DrawString(screen, SCREEN_WIDTH - (strlen(text) * BLOCK_SIZE + BLOCK_SIZE), 2 * BLOCK_SIZE, text, charset);
				gameover = 1;
			}
			else SnakeAppear(screen, snake); // update snake's position on the screen
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
					SnakeAppear(screen, snake);
					MainLoop(snake, screen, charset, screenTexture, renderer);
					quit = 1;
					break;
				case SDLK_UP:
					if (snake->direction != DOWN) snake->direction = UP; // prevent the snake from turning back on itself
					break;
				case SDLK_DOWN:
					if (snake->direction != UP) snake->direction = DOWN;
					break;
				case SDLK_LEFT:
					if (snake->direction != RIGHT) snake->direction = LEFT;
					break;
				case SDLK_RIGHT:
					if (snake->direction != LEFT) snake->direction = RIGHT;
					break;
				}
			}
		}
		frames++;
	}
}

int main(int argc, char** argv)
{
	srand(time(NULL)); // seed for random number generation

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

	SnakeAppear(screenSurface, snake);
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