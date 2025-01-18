// to run the program, substitute the main.cpp file in the helper program project folder with this one
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"

#define BLOCK_SIZE 8 // 8x8 pixel block - default block for characters and snake segments
#define SCREEN_WIDTH 640 // [px], 80 blocks wide - when changed value should be divisible by BLOCK_SIZE
#define SCREEN_HEIGHT 480 // [px], 60 blocks high - when changed value should be divisible by BLOCK_SIZE
#define STATUSBAR_HEIGHT 56 // [px], 7 blocks high - status bar at the top of the screen
#define GAME_GRID_WIDTH ( ((SCREEN_WIDTH - 2 * BLOCK_SIZE) / BLOCK_SIZE) ) // number of blocks in the game grid width (excluding borders)
#define GAME_GRID_HEIGHT ( ((SCREEN_HEIGHT - (BLOCK_SIZE + STATUSBAR_HEIGHT)) / BLOCK_SIZE) ) // number of blocks in the game grid height (excluding borders)

#define BLUE 0x0000FF
#define GREEN 0x00FF00
#define RED 0xFF0000
#define BLACK 0x000000

#define INITIAL_SNAKE_LENGTH 5 // initial snake body length (head excluded)
#define BASE_SNAKE_SPEED 0.3 // [s] per move
#define MINIMAL_SNAKE_SPEED 0.04 // [s] per move

#define BLUE_POINT 1 // points for blue point
#define RED_POINT 10 // points for red point

#define SPEED_INCREASE 0.01 // [s] - not used in this version
#define SPEED_CHANGE_INTERVAL 10 // [s] how often the snake speed increases
#define SPEED_INCREASE_FACTOR 0.1 // by how much of the previous value the snake speed increases

#define MIN_BONUS_APPEAR_INTERVAL 10 // [s] minimum time between bonus point appearances
#define MAX_BONUS_APPEAR_INTERVAL 30 // [s] maximum time between bonus point appearances
#define BONUS_DURATION 10 // [s] for how long the bonus point appears on the screen
#define BONUS_SLOWDOWN_FACTOR 0.2 // by how much of the previous value the snake speed decreases when the bonus point is collected
#define BONUS_SHORTEN 2 // by how many segments the snake is shortened when the bonus point is collected
#define BONUS_PROGRESSBAR_LENGTH 20 // [blocks] length of the bonus point progress bar

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
	double speed;
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

// initialize a point with a random position on the screen, avoiding collision with the snake
point_t* PointInit(snake_t* snake)
{
	point_t* point;
	point = (point_t*)malloc(sizeof(point_t));
	point->x = BLOCK_SIZE + RA(0, GAME_GRID_WIDTH - 1) * BLOCK_SIZE;
	point->y = STATUSBAR_HEIGHT + RA(0, GAME_GRID_HEIGHT - 1) * BLOCK_SIZE;
	if (PointCollisionCheck(snake, point))
	{
		point = PointInit(snake); // if the point collides with the snake, try again
	}
	return point;
}

void PointDraw(SDL_Surface* screen, point_t* point, int colour)
{
	DrawRect(screen, point->x, point->y, BLOCK_SIZE, BLOCK_SIZE, colour);
}

void BonusAction(snake_t* snake)
{
	int bonusAction = RA(0, 1); // random bonus action out of the two possible
	switch (bonusAction)
	{
	case 0:
		// shorten the snake
		if (snake->length > INITIAL_SNAKE_LENGTH + BONUS_SHORTEN) // prevent the snake from shortening below initial (minimal) length
		{
			snake->length -= BONUS_SHORTEN;
			snake->tailX = (int*)realloc(snake->tailX, snake->length * sizeof(int)); // snake shortening
			snake->tailY = (int*)realloc(snake->tailY, snake->length * sizeof(int));
			break;
		}
		else bonusAction = 1; // if the snake is too short, perform the other bonus action
	case 1:
		// speed up the snake
		snake->speed += snake->speed * BONUS_SLOWDOWN_FACTOR;
		break;
	}
}

void BonusProgressBar(SDL_Surface* screen, SDL_Surface* charset, double bonusTimer)
{
	char text[128];
	sprintf(text, "BONUS:");
	int length = BONUS_PROGRESSBAR_LENGTH * BLOCK_SIZE + strlen(text) * BLOCK_SIZE;
	DrawString(screen, SCREEN_WIDTH / 2 - length / 2, 5 * BLOCK_SIZE, text, charset);
	DrawRect(screen, SCREEN_WIDTH / 2 - length / 2 + strlen(text) * BLOCK_SIZE, 5 * BLOCK_SIZE, BONUS_PROGRESSBAR_LENGTH * BLOCK_SIZE, BLOCK_SIZE, BLACK); // progress bar background
	DrawRect(screen, SCREEN_WIDTH / 2 - length / 2 + strlen(text) * BLOCK_SIZE, 5 * BLOCK_SIZE, (int)(BONUS_PROGRESSBAR_LENGTH * BLOCK_SIZE * (bonusTimer / BONUS_DURATION)), BLOCK_SIZE, RED); // progress bar
}

snake_t* SnakeInit(int colour)
{
	snake_t* snake;
	snake = (snake_t*)malloc(sizeof(snake_t));
	snake->colour = colour;
	snake->direction = UP;
	snake->speed = BASE_SNAKE_SPEED;
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
	int inputProcessed = 1; // avoid multiple movement inputs in one frame
	char text[128];
	int gameover = 0; // variable to trigger game over mode (no snake movement, only n and Esc inputs accepted)

	// time management setup:
	int t1, t2, frames;
	double delta, worldTime, fpsTimer, fps, moveTimer, speedupTimer;
	t1 = SDL_GetTicks();
	frames = 0; fpsTimer = 0; fps = 0; worldTime = 0; moveTimer = 0; speedupTimer = 0;

	// point system:
	int points = 0;
	point_t* point = PointInit(snake);
	PointDraw(screen, point, BLUE);

	// bonus point system:
	double bonusTimer = 0;
	double bonusAppearTime = RA(MIN_BONUS_APPEAR_INTERVAL, MAX_BONUS_APPEAR_INTERVAL);
	point_t* bonusPoint = NULL;
	
	// game loop:
	while (!quit)
	{
		if (!gameover) // status bar is not displayed and time doesn't run in game over mode
		{
			// time management:
			t2 = SDL_GetTicks();
			delta = (t2 - t1) * 0.001; // delta time in seconds
			t1 = t2;
			worldTime += delta; fpsTimer += delta; moveTimer += delta; speedupTimer += delta; bonusTimer += delta; // update timing variables
			
			// fps calculations:
			if (fpsTimer > 0.5) {
				fps = frames * 2;
				frames = 0;
				fpsTimer -= 0.5;
			}

			// snake speed increase:
			if (speedupTimer > SPEED_CHANGE_INTERVAL && snake->speed > MINIMAL_SNAKE_SPEED)
			{
				// speed increase by a fixed amount (unused):
				// snake->speed -= SPEED_INCREASE;
				// speed increase by a factor of the previous value:
				snake->speed -= SPEED_INCREASE_FACTOR * snake->speed;
				speedupTimer -= SPEED_CHANGE_INTERVAL;
			}
			
			// status bar:
			DrawFrame(screen, BLUE); // clear status bar
			sprintf(text, "elapsed time = %.1lf s", worldTime);
			DrawString(screen, BLOCK_SIZE, BLOCK_SIZE, text, charset);
			sprintf(text, "%.0lf frames / s", fps);
			DrawString(screen, SCREEN_WIDTH / 2 - (strlen(text) * BLOCK_SIZE / 2), BLOCK_SIZE, text, charset);
			// display current snake speed (for debugging):
			// sprintf(text, "snake moves 8 blocks per %.2lf s", snake->speed);
			// DrawString(screen, SCREEN_WIDTH - (strlen(text) * BLOCK_SIZE), 3 * BLOCK_SIZE, text, charset);
			sprintf(text, "implemented requirements: 1234ABCD");
			DrawString(screen, BLOCK_SIZE, 3*BLOCK_SIZE, text, charset);
			sprintf(text, "points: %d", points);
			DrawString(screen, SCREEN_WIDTH - (strlen(text) * BLOCK_SIZE + BLOCK_SIZE), BLOCK_SIZE, text, charset);

			// bonus point system:
			if (bonusPoint == NULL && bonusTimer > bonusAppearTime)
			{
				bonusPoint = PointInit(snake);
				PointDraw(screen, bonusPoint, RED);
				bonusTimer = 0;
			}
			else if (bonusPoint != NULL && bonusTimer > BONUS_DURATION)
			{
				// delete the bonus point:
				PointDraw(screen, bonusPoint, BLACK);
				free(bonusPoint);
				bonusPoint = NULL;
				bonusTimer = 0;
			}
			else if (bonusPoint != NULL)
			{
				// progress bar for the bonus point duration:
				BonusProgressBar(screen, charset, bonusTimer);
			}
		}

		// snake movement:
		if (moveTimer > snake->speed)
		{
			SnakeDisappear(screen, snake); // erase the snake at its previous position from the screen
			WallCheck(snake); // check for collision with walls; adjust direction if necessary
			SnakeMove(snake); // move the snake (update its position parameters)
			inputProcessed = 1; // reset input processing variable
			if (PointCollisionCheck(snake, point)) // check for collision with the point
			{
				points += BLUE_POINT; // add points for collecting the point
				snake->length++;
				snake->tailX = (int*)realloc(snake->tailX, snake->length * sizeof(int)); // snake lengthening
				snake->tailY = (int*)realloc(snake->tailY, snake->length * sizeof(int));
				snake->tailX[snake->length - 1] = snake->tailX[snake->length - 2]; // add a new segment to the tail
				snake->tailY[snake->length - 1] = snake->tailY[snake->length - 2];
				point = PointInit(snake); // spawn a new point
				PointDraw(screen, point, BLUE); // draw the point on the screen
			}
			if (bonusPoint != NULL && PointCollisionCheck(snake, bonusPoint)) // check for collision with the bonus point
			{
				// bonus point action:
				BonusAction(snake);
				points += RED_POINT; // add points for collecting the bonus point

				// generate new bonus point appear time:
				bonusTimer = 0;
				bonusAppearTime = RA(MIN_BONUS_APPEAR_INTERVAL, MAX_BONUS_APPEAR_INTERVAL);

				// delete the bonus point:
				free(bonusPoint);
				bonusPoint = NULL;
			}
			if (SnakeCollisionCheck(snake)) // check for collision with itself
			{
				DrawFrame(screen, BLUE);
				sprintf(text, "GAME OVER! Press 'n' to start a new game, press 'Esc' to exit.");
				DrawString(screen, BLOCK_SIZE, BLOCK_SIZE, text, charset);
				sprintf(text, "game time: %.1lf s", worldTime);
				DrawString(screen, BLOCK_SIZE, 3*BLOCK_SIZE, text, charset);
				sprintf(text, "points: %d", points);
				DrawString(screen, SCREEN_WIDTH - (strlen(text) * BLOCK_SIZE + BLOCK_SIZE), 3 * BLOCK_SIZE, text, charset);
				gameover = 1;
			}
			else SnakeAppear(screen, snake); // update snake's position on the screen
			moveTimer -= snake->speed; // reset move timeout
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
					if (snake->direction != DOWN && inputProcessed)
					{
						snake->direction = UP; // prevent the snake from turning back on itself
						inputProcessed = 0;
					}
					break;
				case SDLK_DOWN:
					if (snake->direction != UP && inputProcessed)
					{
						snake->direction = DOWN;
						inputProcessed = 0;
					}
					break;
				case SDLK_LEFT:
					if (snake->direction != RIGHT && inputProcessed)
					{
						snake->direction = LEFT;
						inputProcessed = 0;
					}
					break;
				case SDLK_RIGHT:
					if (snake->direction != LEFT && inputProcessed)
					{
						snake->direction = RIGHT;
						inputProcessed = 0;
					}
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