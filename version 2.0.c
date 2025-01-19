#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define MAX_SNAKE_SIZE 320
#define SCREEN_WIDTH	600
#define SCREEN_HEIGHT	600
#define SNAKE_INITIAL_SIZE 9
#define SNAKE_INITIAL_SPEED 0.7
#define POINT_PER_MOVE 1
#define POINTS_PER_BLUE 100
#define POINTS_PER_RED 300
#define SPEEDUP_INTERVAL 5					//number of seconds between every speedup
#define SPEEDUP_VALUE 0.05					//how faster snake will be [seconds]
#define MINIMUM_SPEED 0.05					//minimal snake speed [seconds]
#define DOT_PULSE_INTERVAL 0.4				//speed of dots graphic effect in seconds

//===============RED DOT CONSTANTS=================================================================================================
#define RED_DOT_DURATION 10					//how long red dot would be on screen
#define RED_DOT_MINIMAL_INTERVAL 10			//minimal time that needs to pass between apeareance of next red dot [seconds]
#define RED_DOT_CHANCE 5					//random number of seconds from 0 to this will be added to the interval
#define RED_DOT_SPEED_DEDUCTION 0.2			//snake will be slower of this ammount of [seconds] after eating red dot
#define RED_DOT_LENGHT_DEDUCTION 3			//how shorter snake will be after getting the red dot

//================DATA STRUCTURES====================================================
//===================================================================================
typedef struct {
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
}sdl;

typedef struct {
	int t1, t2, quit, rc;
	double delta, worldTime;
	double snakeWorldTime;											//counts time after last snake move
	double speedupWorldTime;										// =||= speedup
	double redDotWorldTime;											//counter for red dot time off and on screen
	int redDotCountdown;											//value of time to pass for red dot to appear
	double blueDotPulseTimer;
	int blueDotPulseMode;											// 1 - small dot 0 - big dot
	double snakeSpeed;												//do ustawiania prędkości węża we wspolpracy z snakeworldTime
	int tickPassed;
	char text[128];
}game;

typedef struct {
	SDL_Surface* snaker;
	SDL_Surface* lil_snake;
	SDL_Surface* left_head;
	SDL_Surface* right_head;
	SDL_Surface* up_head;
	SDL_Surface* down_head;
	SDL_Surface* tail;
	SDL_Surface* lil_tail;
	int x[MAX_SNAKE_SIZE];
	int y[MAX_SNAKE_SIZE];
	char dir;														//direction of the snake: w = up, a = left, s = down, d = right
	int crash;														// 1 if snake hot itself, 0 otherwise
	int points;
	double speed;													//how fast snake is moving
	int lenght;														//NUMBER OF PROCESSED SNAKE CELLS
	int lengheningReq;												//determines if lenghtening process occurs.
	int newCellX;
	int newCellY;
	char currentDirection;											//where snake is heading right now
}worm;

typedef struct {
	SDL_Surface* blue;
	SDL_Surface* lil_blue;
	int x;
	int y;
	int eaten;
}dot;

typedef struct {
	int czarny;
	int zielony;
	int czerwony;
	int niebieski;
}colors;

typedef struct {
	SDL_Surface* red;
	SDL_Surface* lil_red;
	int x;
	int y;
	int onScreen;
}bonus;

//==================FUNCTIONS========================================================
//===================================================================================
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset)
{
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};

// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color)
{
	for (int i = 0; i < l; i++)
	{
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor)
{
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void updateScreen(sdl* GAME)
{
	SDL_UpdateTexture(GAME->scrtex, NULL, GAME->screen->pixels, GAME->screen->pitch);
	//		SDL_RenderClear(renderer);
	SDL_RenderCopy(GAME->renderer, GAME->scrtex, NULL, NULL);
	SDL_RenderPresent(GAME->renderer);
}

//used for creating new blue dot
void checkIfSnakeEaten(dot* BLUE, worm* SNAKE)
{
	if ((SNAKE->x[0] == BLUE->x) && (SNAKE->y[0] == BLUE->y))				//snake head and blue dot has the same coordinates
	{
		BLUE->eaten = 1;
		SNAKE->points += POINTS_PER_BLUE;
	}
}

//used in checking if snake should lenghten
void checkBlueDot(dot* BLUE, worm* SNAKE)
{
	if ((SNAKE->x[0] == BLUE->x) && (SNAKE->y[0] == BLUE->y))
	{
		SNAKE->lengheningReq = 1;
	}
}

void drawSnake(worm* SNAKE, SDL_Surface* screen)							
{
	for (int i = 0; i < SNAKE->lenght; i++)
	{
		if (i == 0)										 //head of the snake
		{
			if (SNAKE->currentDirection == 'a')
			{
				DrawSurface(screen, SNAKE->left_head, SNAKE->x[i], SNAKE->y[i]);
			}
			else if (SNAKE->currentDirection == 's')
			{
				DrawSurface(screen, SNAKE->down_head, SNAKE->x[i], SNAKE->y[i]);
			}
			else if (SNAKE->currentDirection == 'd')
			{
				DrawSurface(screen, SNAKE->right_head, SNAKE->x[i], SNAKE->y[i]);
			}
			else if (SNAKE->currentDirection == 'w')
			{
				DrawSurface(screen, SNAKE->up_head, SNAKE->x[i], SNAKE->y[i]);
			}
		}
		else if (i == SNAKE->lenght - 1)				//tail of the snake
		{
			if (i % 2 == 1)
			{
				DrawSurface(screen, SNAKE->lil_tail, SNAKE->x[i], SNAKE->y[i]);
			}
			else
			{
				DrawSurface(screen, SNAKE->tail, SNAKE->x[i], SNAKE->y[i]);
			}
			
		}
		else if (i % 2 == 1)
		{
			DrawSurface(screen, SNAKE->lil_snake, SNAKE->x[i], SNAKE->y[i]);
		}
		else
		{
			DrawSurface(screen, SNAKE->snaker, SNAKE->x[i], SNAKE->y[i]);
		}
	}
}

void updateSnakeBody(worm* SNAKE)
{
	SNAKE->newCellX = SNAKE->x[SNAKE->lenght - 1];				//for snake lenghtening, saves previous position of last snake part
	SNAKE->newCellY = SNAKE->y[SNAKE->lenght - 1];				//for the new one
	for (int i = SNAKE->lenght - 1; i >= 1; i--)			//handling every snake "cell" without the head
	{
		SNAKE->x[i] = SNAKE->x[i - 1];						//every "cell" get one position further
		SNAKE->y[i] = SNAKE->y[i - 1];
	}
}

void moveSnake(worm* SNAKE, dot* BLUE)
{
	SNAKE->points++;

	updateSnakeBody(SNAKE); // update every "cell" of the snake without a head
	switch (SNAKE->dir)
	{
	case 'w':
		if (SNAKE->y[0] == 75)			
		{
			if (SNAKE->x[0] == SCREEN_WIDTH - 100) //move right or left if its not possible
			{
				SNAKE->x[0] -= 25;
				SNAKE->dir = 'a';
				SNAKE->currentDirection = 'a';
			}
			else
			{
				SNAKE->x[0] += 25;			
				SNAKE->dir = 'd';			
				SNAKE->currentDirection = 'd';
			}

		}
		else										//move normally if not at the border
		{
			SNAKE->y[0] -= 25;
			SNAKE->currentDirection = 'w';
			SNAKE->currentDirection = 'w';
		}

		break;

	case 'a':
		if (SNAKE->x[0] == 125)			
		{
			if (SNAKE->y[0] == 75)
			{
				SNAKE->y[0] += 25;			
				SNAKE->dir = 's';
				SNAKE->currentDirection = 's';
			}
			else
			{
				SNAKE->y[0] -= 25;			
				SNAKE->dir = 'w';
				SNAKE->currentDirection = 'w';
			}
		}
		else
		{
			SNAKE->x[0] -= 25;
			SNAKE->currentDirection = 'a';
		}
		break;

	case 's':
		if (SNAKE->y[0] == SCREEN_HEIGHT - 50)  
		{
			if (SNAKE->x[0] == 125)
			{
				SNAKE->x[0] += 25;
				SNAKE->dir = 'd';
				SNAKE->currentDirection = 'd';
			}
			else
			{
				SNAKE->x[0] -= 25;
				SNAKE->dir = 'a';
				SNAKE->currentDirection = 'a';
			}
		}
		else
		{
			SNAKE->y[0] += 25;
			SNAKE->currentDirection = 's';
		}
		break;

	case 'd':
		if (SNAKE->x[0] == SCREEN_WIDTH - 100)  
		{
			if (SNAKE->y[0] == SCREEN_HEIGHT - 50)
			{
				SNAKE->y[0] -= 25;
				SNAKE->dir = 'w';
				SNAKE->currentDirection = 'w';
			}
			else
			{
				SNAKE->y[0] += 25;
				SNAKE->dir = 's';
				SNAKE->currentDirection = 's';
			}
		}
		else
		{
			SNAKE->x[0] += 25;
			SNAKE->currentDirection = 'd';
		}
		break;
	}

	checkBlueDot(BLUE, SNAKE);							//check if snake head moved into a blue dot

	if (SNAKE->lengheningReq == 1)						// snake gets longer
	{
		SNAKE->x[SNAKE->lenght] = SNAKE->newCellX;
		SNAKE->y[SNAKE->lenght] = SNAKE->newCellY;
		SNAKE->lenght++;
		SNAKE->lengheningReq = 0;
	}
}

void initaializeSnake(worm* SNAKE)
{		
	SNAKE->snaker = SDL_LoadBMP("./snaker.bmp");		
	SNAKE->lil_snake = SDL_LoadBMP("./lil_snake.bmp");
	SNAKE->left_head = SDL_LoadBMP("./head_left.bmp");
	SNAKE->right_head = SDL_LoadBMP("./head_right.bmp");
	SNAKE->up_head = SDL_LoadBMP("./head_up.bmp");
	SNAKE->down_head = SDL_LoadBMP("./head_down.bmp");
	SNAKE->tail = SDL_LoadBMP("./tail.bmp");
	SNAKE->lil_tail = SDL_LoadBMP("./lil_tail.bmp");
	SNAKE->x[0] = 375;
	SNAKE->y[0] = 400;
	SNAKE->dir = 'd';
	SNAKE->points = 0;
	SNAKE->crash = 0;
	SNAKE->lenght = SNAKE_INITIAL_SIZE;
	SNAKE->currentDirection = 'd';
	int substitution = SNAKE->x[0];
	for (int i = 1; i < SNAKE->lenght; i++)
	{
		substitution -= 25;
		SNAKE->y[i] = SNAKE->y[0];
		SNAKE->x[i] = substitution;
	}
}

void initializeColors(colors* COLOR, SDL_Surface* screen)
{
	COLOR->czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	COLOR->zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	COLOR->czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	COLOR->niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
}

void initializeGame(sdl* GAME)
{
	GAME->screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	// loading cs8x8.bmp
	GAME->charset = SDL_LoadBMP("./cs8x8.bmp");
	if (GAME->charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(GAME->screen);
		SDL_DestroyTexture(GAME->scrtex);
		SDL_DestroyWindow(GAME->window);
		SDL_DestroyRenderer(GAME->renderer);
		SDL_Quit();
	};
	SDL_SetColorKey(GAME->charset, true, 0x000000);

	GAME->scrtex = SDL_CreateTexture(GAME->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
}

void initializeGameData(game* GAMEDATA, sdl* GAME)
{
	GAMEDATA->t1 = SDL_GetTicks();
	GAMEDATA->quit = 0;
	GAMEDATA->worldTime = 0;
	GAMEDATA->tickPassed = 0;
	GAMEDATA->snakeWorldTime = 0;
	GAMEDATA->speedupWorldTime = 0;
	GAMEDATA->snakeSpeed = SNAKE_INITIAL_SPEED;
	GAMEDATA->blueDotPulseTimer = 0;
	GAMEDATA->blueDotPulseMode = 0;

	GAMEDATA->rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&GAME->window, &GAME->renderer);
	if (GAMEDATA->rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
	};
}

void handleGameOver(worm* SNAKE, colors* COLOR, sdl* GAME, game* GAMEDATA, dot *BLUE, bonus *RED)
{
	char decision = 'x';
	// game over box
	DrawRectangle(GAME->screen, 50, 200, SCREEN_WIDTH - 100, 100, COLOR->czerwony, COLOR->niebieski);
	sprintf(GAMEDATA->text, "GAME OVER! YOU SCORED: %d POINTS!", SNAKE->points);
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 220, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "PRESS ESC TO QUIT, n FOR A NEW GAME");
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 260, GAMEDATA->text, GAME->charset);

	//update screen
	SDL_UpdateTexture(GAME->scrtex, NULL, GAME->screen->pixels, GAME->screen->pitch);
	//		SDL_RenderClear(renderer);
	SDL_RenderCopy(GAME->renderer, GAME->scrtex, NULL, NULL);
	SDL_RenderPresent(GAME->renderer);

	//stop game until player make a decision
	while (decision == 'x')
	{
		while (SDL_PollEvent(&GAME->event))
		{
			switch (GAME->event.type)
			{
			case SDL_KEYDOWN:														//button is pressed
				if (GAME->event.key.keysym.sym == SDLK_ESCAPE)
				{
					GAMEDATA->quit = 1;
					decision = 'q';													//ending a game stopping loop
					break;
				}
				else if (GAME->event.key.keysym.sym == SDLK_n)
				{
					initaializeSnake(SNAKE);
					decision = 'q';
					GAMEDATA->t1 = SDL_GetTicks();									//RESETING THE "CLOCK" AND THE TIMER
					GAMEDATA->worldTime = 0;										//reseting other game variables
					GAMEDATA->snakeSpeed = SNAKE_INITIAL_SPEED;
					BLUE->eaten = 1;
					RED->onScreen = 0;
					GAMEDATA->redDotWorldTime = 0;
					break;
				}
			case SDL_QUIT:
				GAMEDATA->quit = 1;
				decision = 'q';
				break;
			};
		};
	}
}

void checkCollision(worm* SNAKE, colors* COLOR, sdl* GAME, game* GAMEDATA, dot *BLUE, bonus *RED)
{
	for (int i = 1; i < SNAKE->lenght; i++)
	{
		if ((SNAKE->x[i] == SNAKE->x[0]) && (SNAKE->y[i] == SNAKE->y[0]))
		{
			SNAKE->crash = 1;
		}
	}

	if (SNAKE->crash == 1)
	{
		handleGameOver(SNAKE, COLOR, GAME, GAMEDATA, BLUE, RED);
	}
}

void handleSnake(worm* SNAKE, colors* COLOR, sdl* GAME, game* GAMEDATA, dot* BLUE, bonus* RED)
{
	if (GAMEDATA->tickPassed == 1)						//amount of time required for snake to move passed
	{
		moveSnake(SNAKE, BLUE);					
		GAMEDATA->tickPassed = 0;
	}
	drawSnake(SNAKE, GAME->screen);
	checkCollision(SNAKE, COLOR, GAME, GAMEDATA, BLUE, RED);
}

void writeInfoScreen(sdl* GAME, colors* COLOR, game* GAMEDATA, worm* SNAKE)
{
	DrawRectangle(GAME->screen, 4, 8, SCREEN_WIDTH - 8, 36, COLOR->czerwony, COLOR->niebieski);
	sprintf(GAMEDATA->text, "Time elapsed: %.1lf s, POINTS: %d", GAMEDATA->worldTime, SNAKE->points);
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 14, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "Esc - exit, n - new game");
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 30, GAMEDATA->text, GAME->charset);
}

void drawReqScreen(sdl* GAME, colors* COLOR, game* GAMEDATA)
{
	DrawRectangle(GAME->screen, 4, 60, 100, 90, COLOR->czerwony, COLOR->niebieski);
	sprintf(GAMEDATA->text, "Met req:");
	DrawString(GAME->screen, 14, 70, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "1, 2, 3, 4");
	DrawString(GAME->screen, 14, 90, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "A, B, C, D");
	DrawString(GAME->screen, 14, 110, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "G");
	DrawString(GAME->screen, 14, 130, GAMEDATA->text, GAME->charset);
}

void updateGameData(game* GAMEDATA)
{
	GAMEDATA->t2 = SDL_GetTicks();
	// here t2-t1 is the time in milliseconds since
	// the last screen was drawn
	// delta is the same time in seconds
	GAMEDATA->delta = (GAMEDATA->t2 - GAMEDATA->t1) * 0.001;
	GAMEDATA->t1 = GAMEDATA->t2;
	GAMEDATA->worldTime += GAMEDATA->delta;									//worldTime is time from beggining of a new game in seconds

	//UPDATING GAME TIMERS, "world Time" variables count seconds from last specified event
	GAMEDATA->snakeWorldTime += GAMEDATA->delta;							//snakeWorldTime is time from last move of snake in seconds
	GAMEDATA->speedupWorldTime += GAMEDATA->delta;							//last speedup
	GAMEDATA->redDotWorldTime += GAMEDATA->delta;							//last change of state visible/not visible
	GAMEDATA->blueDotPulseTimer += GAMEDATA->delta;							//last pulse of blue dot
	if (GAMEDATA->snakeWorldTime >= GAMEDATA->snakeSpeed)					//snakeSpeed is number of seconds between every snake move
	{
		GAMEDATA->tickPassed = 1;											//tickPassed allows snake to move
		GAMEDATA->snakeWorldTime = 0;
	}
	if (GAMEDATA->speedupWorldTime >= SPEEDUP_INTERVAL)
	{

		if (GAMEDATA->snakeSpeed - SPEEDUP_VALUE >= MINIMUM_SPEED)			//this ensures snake speed wont drop bellow minimum
		{
			GAMEDATA->snakeSpeed -= SPEEDUP_VALUE;
		}
		GAMEDATA->speedupWorldTime = 0;
	}

	if (GAMEDATA->blueDotPulseTimer >= DOT_PULSE_INTERVAL)
	{
		if (GAMEDATA->blueDotPulseMode == 0)
		{
			GAMEDATA->blueDotPulseMode = 1;
		}
		else
		{
			GAMEDATA->blueDotPulseMode = 0;
		}
		GAMEDATA->blueDotPulseTimer = 0;
	}
}

void fillScreen(sdl* GAME, colors* COLOR)
{
	//FILLING WHOLE WINDOW WITH BLACK RECTANGLE -> SERVES AS A REFRESH
	SDL_FillRect(GAME->screen, NULL, COLOR->czarny);
	DrawRectangle(GAME->screen, 110, 60, SCREEN_WIDTH - 195, SCREEN_HEIGHT - 95, COLOR->zielony, COLOR->niebieski);
}

void initializeBlueDot(dot* BLUE)
{
	srand(time(NULL));										
	BLUE->blue = SDL_LoadBMP("./blue.bmp");
	BLUE->lil_blue = SDL_LoadBMP("./lil_blue.bmp");
	BLUE->eaten = 1;										//checks if snake "eaten" the dot, 1 to generate new dot at the beggining
}

void placeNewBlueDot(dot* BLUE, worm* SNAKE, sdl* GAME)
{
	int x, y;
	int placeFound = 0;
	int onSnake = 0;
	while (placeFound == 0)
	{
		onSnake = 0;
		x = ((rand() % 15) + 5) * 25;							//random place in bounds of the game
		y = ((rand() % 20) + 3) * 25;
		for (int i = 0; i < SNAKE->lenght; i++)
		{
			if ((SNAKE->x[i] == x) && (SNAKE->y[i] == y))		//checking if area is not ocupied by the snake
			{
				onSnake = 1;
			}
		}

		if (onSnake == 0)
		{
			BLUE->x = x;
			BLUE->y = y;
			placeFound = 1;
		}
	}
	BLUE->eaten = 0;											//new blue dot is not yet eaten
}

void drawBlueDot(dot* BLUE, sdl* GAME, game *GAMEDATA)
{
	if (GAMEDATA->blueDotPulseMode == 0)
	{
		DrawSurface(GAME->screen, BLUE->lil_blue, BLUE->x, BLUE->y);
	}
	else
	{
		DrawSurface(GAME->screen, BLUE->blue, BLUE->x, BLUE->y);
	}
	
}

void handleBlueDot(dot* BLUE, worm* SNAKE, sdl* GAME, game *GAMEDATA)
{
	checkIfSnakeEaten(BLUE, SNAKE);

	if (BLUE->eaten == 1)
	{
		placeNewBlueDot(BLUE, SNAKE, GAME);
	}

	drawBlueDot(BLUE, GAME, GAMEDATA);
}

void initializeRedDot(bonus* RED, game* GAMEDATA)		//PLACE THIS IN INIT EVERYTHING, 
{
	RED->red = SDL_LoadBMP("./red.bmp");
	RED->lil_red = SDL_LoadBMP("./lil_red.bmp");
	GAMEDATA->redDotWorldTime = 0;
	GAMEDATA->redDotCountdown = RED_DOT_MINIMAL_INTERVAL + rand() % RED_DOT_CHANCE;
	RED->onScreen = 0;
}

void positionRedDot(worm* SNAKE, bonus* RED)
{
	int x, y;
	int placeFound = 0;
	int onSnake = 0;
	while (placeFound == 0)
	{
		onSnake = 0;
		x = ((rand() % 15) + 5) * 25;							//random place within bounds of the game
		y = ((rand() % 20) + 3) * 25;
		for (int i = 0; i < SNAKE->lenght; i++)
		{
			if ((SNAKE->x[i] == x) && (SNAKE->y[i] == y))		//checking if area is not ocupied by the snake
			{
				onSnake = 1;
			}
		}

		if (onSnake == 0)
		{
			RED->x = x;
			RED->y = y;
			placeFound = 1;
		}
	}
}

void checkRedDotTimer(bonus* RED, game* GAMEDATA, worm* SNAKE)
{
	if (RED->onScreen == 0)
	{
		if (GAMEDATA->redDotWorldTime >= GAMEDATA->redDotCountdown)
		{
			GAMEDATA->redDotWorldTime = 0;							//red dot appears on screen, gets new x,y, and new wait time to reappear
			RED->onScreen = 1;
			positionRedDot(SNAKE, RED);
			GAMEDATA->redDotCountdown = RED_DOT_MINIMAL_INTERVAL + rand() % RED_DOT_CHANCE;
		}
	}
	else if (RED->onScreen == 1)
	{
		if (GAMEDATA->redDotWorldTime >= RED_DOT_DURATION)
			{
				GAMEDATA->redDotWorldTime = 0;
				RED->onScreen = 0;
			}
	}
}

void drawRedDot(bonus* RED, sdl* GAME, game *GAMEDATA)
{
	if (RED->onScreen == 0)
	{
		return;						//do not draw red dot when it is not active
	}

	if (GAMEDATA->blueDotPulseMode == 0)
	{
		DrawSurface(GAME->screen, RED->red, RED->x, RED->y);
	}
	else
	{
		DrawSurface(GAME->screen, RED->lil_red, RED->x, RED->y);
	}
}

void checkIfRedDotEaten(bonus* RED, worm* SNAKE, game* GAMEDATA)
{
	if (RED->onScreen == 0)
	{
		return;
	}

	if ((SNAKE->x[0] == RED->x) && (SNAKE->y[0] == RED->y))
	{
		if (rand() % 2 == 0)
		{
			GAMEDATA->snakeSpeed += RED_DOT_SPEED_DEDUCTION;				//making wait time for speedup longer -> slowing the snake
		}
		else
		{
			if (SNAKE->lenght - RED_DOT_LENGHT_DEDUCTION >= 1)				//checking if snake will not become too short
			{
				SNAKE->lenght -= RED_DOT_LENGHT_DEDUCTION;
			}
		}

		GAMEDATA->redDotWorldTime = 0;
		RED->onScreen = 0;
		SNAKE->points += POINTS_PER_RED;
	}
}

void handleProgressBar(bonus* RED, game* GAMEDATA, sdl* GAME, colors *COLOR)
{
	if (RED->onScreen == 0)
	{
		return;
	}
	double progress = RED_DOT_DURATION / 5;
	DrawRectangle(GAME->screen, 540, 80, 22, 102, COLOR->zielony, COLOR->czarny);
	sprintf(GAMEDATA->text, "RED DOT!");
	DrawString(GAME->screen, 525, 70, GAMEDATA->text, GAME->charset);

	//rectangle will be filled with red squares depending of how many of duration time has passed
	if (GAMEDATA->redDotWorldTime >= progress)
	{
		DrawRectangle(GAME->screen, 541, 81, 20, 20, COLOR->czerwony, COLOR->czerwony);
	}
	if (GAMEDATA->redDotWorldTime >= progress * 2)
	{
		DrawRectangle(GAME->screen, 541, 101, 20, 20, COLOR->czerwony, COLOR->czerwony);
	}
	if (GAMEDATA->redDotWorldTime >= progress * 3)
	{
		DrawRectangle(GAME->screen, 541, 121, 20, 20, COLOR->czerwony, COLOR->czerwony);
	}
	if (GAMEDATA->redDotWorldTime >= progress * 4)
	{
		DrawRectangle(GAME->screen, 541, 141, 20, 20, COLOR->czerwony, COLOR->czerwony);
	}
	if (GAMEDATA->redDotWorldTime >= progress * 4.5)
	{
		DrawRectangle(GAME->screen, 541, 161, 20, 10, COLOR->czerwony, COLOR->czerwony);
	}
	if (GAMEDATA->redDotWorldTime >= RED_DOT_DURATION - 0.2)
	{
		DrawRectangle(GAME->screen, 541, 171, 20, 8, COLOR->czerwony, COLOR->czerwony);
	}
}

void handleRedDot(bonus* RED, sdl* GAME, game* GAMEDATA, worm* SNAKE, colors *COLOR)
{
	checkIfRedDotEaten(RED, SNAKE, GAMEDATA);
	checkRedDotTimer(RED, GAMEDATA, SNAKE);
	drawRedDot(RED, GAME, GAMEDATA);
	handleProgressBar(RED, GAMEDATA, GAME, COLOR);
}

void initializeEverything(sdl* GAME, game* GAMEDATA, colors* COLOR, worm* SNAKE, dot* BLUE, bonus* RED)
{
	initializeGameData(GAMEDATA, GAME);
	initializeGame(GAME);
	initaializeSnake(SNAKE);
	initializeColors(COLOR, GAME->screen);
	initializeBlueDot(BLUE);
	initializeRedDot(RED, GAMEDATA);
	SDL_SetWindowTitle(GAME->window, "SNAKE GAME");
}

void handleNewGame(worm *SNAKE, game *GAMEDATA, dot *BLUE, bonus *RED)
{
	initaializeSnake(SNAKE);
	GAMEDATA->worldTime = 0;
	GAMEDATA->snakeSpeed = SNAKE_INITIAL_SPEED;
	BLUE->eaten = 1;
	RED->onScreen = 0;
	GAMEDATA->redDotWorldTime = 0;
}

void freeSurfaces(sdl* GAME)
{
	SDL_FreeSurface(GAME->charset);
	SDL_FreeSurface(GAME->screen);
	SDL_DestroyTexture(GAME->scrtex);
	SDL_DestroyRenderer(GAME->renderer);
	SDL_DestroyWindow(GAME->window);
}
// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv)
{
	worm SNAKE;
	colors COLOR;
	sdl GAME;
	game GAMEDATA;
	dot BLUE;
	bonus RED;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
	}

	initializeEverything(&GAME, &GAMEDATA, &COLOR, &SNAKE, &BLUE, &RED);
	
	while (!GAMEDATA.quit)
	{
		fillScreen(&GAME, &COLOR);
		updateGameData(&GAMEDATA);
		writeInfoScreen(&GAME, &COLOR, &GAMEDATA, &SNAKE);
		drawReqScreen(&GAME, &COLOR, &GAMEDATA);
		handleBlueDot(&BLUE, &SNAKE, &GAME, &GAMEDATA);
		handleRedDot(&RED, &GAME, &GAMEDATA, &SNAKE, &COLOR);
		handleSnake(&SNAKE, &COLOR, &GAME, &GAMEDATA, &BLUE, &RED);
		updateScreen(&GAME);

		//handling of events (if there were any)
		while (SDL_PollEvent(&GAME.event))
		{
			switch (GAME.event.type)
			{
			case SDL_KEYDOWN:														//button is pressed
				if (GAME.event.key.keysym.sym == SDLK_ESCAPE) GAMEDATA.quit = 1;
				else if (GAME.event.key.keysym.sym == SDLK_n)
				{
					handleNewGame(&SNAKE, &GAMEDATA, &BLUE, &RED);
				}
				else if (GAME.event.key.keysym.sym == SDLK_UP && SNAKE.dir != 's' && SNAKE.currentDirection != 's')
				{
					SNAKE.dir = 'w';
					break;															
				}																																		
				else if (GAME.event.key.keysym.sym == SDLK_DOWN && SNAKE.dir != 'w' && SNAKE.currentDirection != 'w')
				{
					SNAKE.dir = 's';
					break;
				}
				else if (GAME.event.key.keysym.sym == SDLK_LEFT && SNAKE.dir != 'd' && SNAKE.currentDirection != 'd')
				{
					SNAKE.dir = 'a';
					break;
				}
				else if (GAME.event.key.keysym.sym == SDLK_RIGHT && SNAKE.dir != 'a' && SNAKE.currentDirection != 'a')
				{
					SNAKE.dir = 'd';
					break;
				}
				break;
			case SDL_QUIT:
				GAMEDATA.quit = 1;
				break;
			};
		};
	};

	// zwolnienie powierzchni / freeing all surfaces
	freeSurfaces(&GAME);
	SDL_Quit();
	return 0;
};
