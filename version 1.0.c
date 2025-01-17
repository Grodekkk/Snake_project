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

#define SCREEN_WIDTH	600
#define SCREEN_HEIGHT	600
#define SNAKE_SIZE 9
#define SNAKE_INITIAL_SPEED 1.5
#define POINT_PER_MOVE 1
#define POINTS_PER_BLUE 100
#define SPEEDUP_INTERVAL 10					//number of seconds between every speedup
#define SPEEDUP_VALUE 0.1						//how faster snake will be [seconds]
#define MINIMUM_SPEED 0.25					//minimal snake speed [seconds]

//================DATA STRUCTURES====================================================
//===================================================================================
typedef struct {													//1
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* eti;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
}sdl;

typedef struct {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed;
	double snakeWorldTime;											//mierzy czas od ostatniego ruchu węża
	double speedupWorldTime;
	double snakeSpeed;												//do ustawiania prędkości węża we wspolpracy z snakeworldTime
	int tickPassed;
	char text[128];
}game;

typedef struct {
	SDL_Surface* snaker;
	int x[SNAKE_SIZE];
	int y[SNAKE_SIZE];
	char dir;				//direction of the snake: w = up, a = left, s = down, d = right
	int crash;				// 1 if snake hot itself, 0 otherwise
	int points;
	double speed;
}worm;

typedef struct {
	SDL_Surface* blue;
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

void drawSnake(worm* SNAKE, SDL_Surface* screen)  // SEPARATE MOVING AND SEPARATE DRAWING, DIRECTIONS, LOGIC
{
	for (int i = 0; i < SNAKE_SIZE; i++)
	{
		DrawSurface(screen, SNAKE->snaker, SNAKE->x[i], SNAKE->y[i]);
	}
}

void updateSnakeBody(worm* SNAKE)
{
	for (int i = SNAKE_SIZE - 1; i >= 1; i--)	//handling every snake "cell" without the head
	{
		SNAKE->x[i] = SNAKE->x[i - 1];			//every "cell" get one position further
		SNAKE->y[i] = SNAKE->y[i - 1];
	}
}

void moveSnake(worm* SNAKE)
{
	SNAKE->points++;
	updateSnakeBody(SNAKE); // update every "cell" of the snake without a head
	switch (SNAKE->dir)
	{
	case 'w':
		if (SNAKE->y[0] == 75)			//100, 125 CHANGE INTO BORDER CONSTS
		{
			if (SNAKE->x[0] == SCREEN_WIDTH - 100)
			{
				SNAKE->x[0] -= 25;
				SNAKE->dir = 'a';
			}
			else
			{
				SNAKE->x[0] += 25;			//by snake or by edge
				SNAKE->dir = 'd';			//need to implement left move if its not possible
			}
		}
		else
		{
			SNAKE->y[0] -= 25;
		}

		break;

	case 'a':
		if (SNAKE->x[0] == 125)			//100, 125 CHANGE INTO BORDER CONSTS
		{
			if (SNAKE->y[0] == 75)
			{
				SNAKE->y[0] += 25;			//error checking????????????	
				SNAKE->dir = 's';
			}
			else
			{
				SNAKE->y[0] -= 25;			//error checking????????????	
				SNAKE->dir = 'w';
			}
		}
		else
		{
			SNAKE->x[0] -= 25;
		}
		break;

	case 's':
		if (SNAKE->y[0] == SCREEN_HEIGHT - 50)  //100, 125 CHANGE INTO BORDER CONSTS
		{
			if (SNAKE->x[0] == 125)
			{
				SNAKE->x[0] += 25;
				SNAKE->dir = 'd';
			}
			else
			{
				SNAKE->x[0] -= 25;
				SNAKE->dir = 'a';
			}
		}
		else
		{
			SNAKE->y[0] += 25;
		}
		break;

	case 'd':
		if (SNAKE->x[0] == SCREEN_WIDTH - 100)  //100, 125 CHANGE INTO BORDER CONSTS
		{
			if (SNAKE->y[0] == SCREEN_HEIGHT - 50)
			{
				SNAKE->y[0] -= 25;
				SNAKE->dir = 'w';
			}
			else
			{
				SNAKE->y[0] += 25;
				SNAKE->dir = 's';
			}
		}
		else
		{
			SNAKE->x[0] += 25;
		}
		break;
	}

}

void initaializeSnake(worm* SNAKE)
{
	int width, height, startX, startY;					//zmienne pomocnicze do obliczenia położenia startowego węża		
	SNAKE->snaker = SDL_LoadBMP("./snaker.bmp");		//obsluga bledow
	//width = SCREEN_WIDTH / 25;
	//startX = (width / 2) * 25;  // ustawianie węża na środku ekranu
	//height = SCREEN_HEIGHT / 25;
	//startY = (height / 2) * 25;
	SNAKE->x[0] = 375;
	SNAKE->y[0] = 400;
	SNAKE->dir = 'd';
	SNAKE->points = 0;
	SNAKE->crash = 0;
	int substitution = SNAKE->x[0];
	for (int i = 1; i < SNAKE_SIZE; i++)
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

	// wczytanie obrazka cs8x8.bmp
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

	GAME->eti = SDL_LoadBMP("./eti.bmp");
	//snaker = SDL_LoadBMP("./snaker.bmp");  //calyprotokol co jak sie nie wczyta
	if (GAME->eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(GAME->charset);
		SDL_FreeSurface(GAME->screen);
		SDL_DestroyTexture(GAME->scrtex);
		SDL_DestroyWindow(GAME->window);
		SDL_DestroyRenderer(GAME->renderer);
		SDL_Quit();
	};

	GAME->scrtex = SDL_CreateTexture(GAME->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
}

void initializeGameData(game* GAMEDATA, sdl* GAME)
{
	GAMEDATA->t1 = SDL_GetTicks();
	GAMEDATA->quit = 0;
	GAMEDATA->frames = 0;
	GAMEDATA->fpsTimer = 0;
	GAMEDATA->fps = 0;
	GAMEDATA->worldTime = 0;
	GAMEDATA->distance = 0;
	GAMEDATA->etiSpeed = 1;
	GAMEDATA->tickPassed = 0;
	GAMEDATA->snakeWorldTime = 0;
	GAMEDATA->speedupWorldTime = 0;
	GAMEDATA->snakeSpeed = SNAKE_INITIAL_SPEED;

	GAMEDATA->rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&GAME->window, &GAME->renderer);
	if (GAMEDATA->rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
	};
}

void handleGameOver(worm* SNAKE, colors* COLOR, sdl* GAME, game* GAMEDATA)
{
	char decision = 'x';
	//DrawRectangle(GAME->screen, 110, 60, SCREEN_WIDTH - 195, SCREEN_HEIGHT - 95, COLOR->zielony, COLOR->niebieski);
	DrawRectangle(GAME->screen, 50, 200, SCREEN_WIDTH - 100, 100, COLOR->czerwony, COLOR->niebieski);
	sprintf(GAMEDATA->text, "GAME OVER! YOU SCORED: %d POINTS!", SNAKE->points);
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 220, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "PRESS ESC TO QUIT, n FOR A NEW GAME");
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 260, GAMEDATA->text, GAME->charset);

	SDL_UpdateTexture(GAME->scrtex, NULL, GAME->screen->pixels, GAME->screen->pitch);
	//		SDL_RenderClear(renderer);
	SDL_RenderCopy(GAME->renderer, GAME->scrtex, NULL, NULL);
	SDL_RenderPresent(GAME->renderer);

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
					decision = 'q';
					break;
				}
				else if (GAME->event.key.keysym.sym == SDLK_n)
				{
					initaializeSnake(SNAKE);
					decision = 'q';
					GAMEDATA->t1 = SDL_GetTicks();					//RESETING THE "CLOCK" AND THE TIMER
					GAMEDATA->worldTime = 0;
					GAMEDATA->snakeSpeed = SNAKE_INITIAL_SPEED;
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

void checkCollision(worm* SNAKE, colors* COLOR, sdl* GAME, game* GAMEDATA)
{
	for (int i = 1; i < SNAKE_SIZE; i++)
	{
		if ((SNAKE->x[i] == SNAKE->x[0]) && (SNAKE->y[i] == SNAKE->y[0]))
		{
			SNAKE->crash = 1;
		}
	}

	if (SNAKE->crash == 1)
	{
		handleGameOver(SNAKE, COLOR, GAME, GAMEDATA);
		//initaializeSnake(SNAKE);
		//SNAKE->crash = 0;
	}
}

void handleSnake(worm* SNAKE, colors* COLOR, sdl* GAME, game* GAMEDATA)
{
	if (GAMEDATA->tickPassed == 1)			//in void updateGamedata
	{
		moveSnake(SNAKE);					//how to handle game speed?    ONE FUNCTION HANDLESNAKE?
		GAMEDATA->tickPassed = 0;
	}

	drawSnake(SNAKE, GAME->screen);

	checkCollision(SNAKE, COLOR, GAME, GAMEDATA);
}

void writeInfoScreen(sdl* GAME, colors* COLOR, game* GAMEDATA, worm* SNAKE)
{
	DrawRectangle(GAME->screen, 4, 8, SCREEN_WIDTH - 8, 36, COLOR->czerwony, COLOR->niebieski);
	sprintf(GAMEDATA->text, "Time elapsed: %.1lf s, POINTS: %d", GAMEDATA->worldTime, SNAKE->points);
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 14, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "Esc - wyjscie, n - new game");
	DrawString(GAME->screen, GAME->screen->w / 2 - strlen(GAMEDATA->text) * 8 / 2, 30, GAMEDATA->text, GAME->charset);
}

void drawReqScreen(sdl* GAME, colors* COLOR, game* GAMEDATA)
{
	DrawRectangle(GAME->screen, 4, 60, 100, 70, COLOR->czerwony, COLOR->niebieski);
	sprintf(GAMEDATA->text, "Met req:");
	DrawString(GAME->screen, 14, 70, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "1, 2, 3, 4");
	DrawString(GAME->screen, 14, 90, GAMEDATA->text, GAME->charset);
	sprintf(GAMEDATA->text, "B, WIP: C");
	DrawString(GAME->screen, 14, 110, GAMEDATA->text, GAME->charset);
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

	//CHECKING FOR TIME INTERVAL OF SNAKE MOVEMENT
	GAMEDATA->snakeWorldTime += GAMEDATA->delta;							//snakeWorldTime is time from last move of snake in seconds
	GAMEDATA->speedupWorldTime += GAMEDATA->delta;
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
	GAMEDATA->distance += GAMEDATA->etiSpeed * GAMEDATA->delta;

	GAMEDATA->fpsTimer += GAMEDATA->delta;
	if (GAMEDATA->fpsTimer > 0.5) {
		GAMEDATA->fps = GAMEDATA->frames * 2;
		GAMEDATA->frames = 0;
		GAMEDATA->fpsTimer -= 0.5;
	};
}

void fillScreen(sdl* GAME, colors* COLOR)
{
	//FILLING WHOLE WINDOW WITH BLACK RECTANGLE -> SERVES AS A REFRESH
	SDL_FillRect(GAME->screen, NULL, COLOR->czarny);
	//drawing game area right now it will be fixed, we will make it interchangeable later
	DrawRectangle(GAME->screen, 110, 60, SCREEN_WIDTH - 195, SCREEN_HEIGHT - 95, COLOR->zielony, COLOR->niebieski);

}

void initializeBlueDot(dot* BLUE)
{
	srand(time(NULL));										//for future apple randomness
	BLUE->blue = SDL_LoadBMP("./apple.bmp");
	BLUE->eaten = 1;										//checks if snake "eaten" the dot, 1 to generate new dot at the beggining
}

void placeNewBlueDot(dot *BLUE, worm *SNAKE, sdl *GAME)
{
	int x, y;
	int placeFound = 0;
	int onSnake = 0;
	while (placeFound == 0)
	{
		onSnake = 0;
		x = ((rand() % 15) + 5) * 25;
		y = ((rand() % 20) + 3) * 25;
		for (int i = 0; i < SNAKE_SIZE; i++)
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

void checkIfSnakeEaten(dot* BLUE, worm* SNAKE)
{
	if ((SNAKE->x[0] == BLUE->x) && (SNAKE->y[0] == BLUE->y))
	{
		BLUE->eaten = 1;
		SNAKE->points += POINTS_PER_BLUE;
	}
}

void drawBlueDot(dot* BLUE, sdl* GAME)
{
	DrawSurface(GAME->screen, BLUE->blue, BLUE->x, BLUE->y);
}

void handleBlueDot(dot* BLUE, worm *SNAKE, sdl* GAME)
{
	checkIfSnakeEaten(BLUE, SNAKE);

	if (BLUE->eaten == 1)
	{
		placeNewBlueDot(BLUE, SNAKE, GAME);
	}

	drawBlueDot(BLUE, GAME);
}

void initializeEverything(sdl* GAME, game* GAMEDATA, colors* COLOR, worm* SNAKE, dot *BLUE)
{
	//INITIALIZATION OF DATA STRUCTURES, SCREEN SNAKE ETC
	initializeGameData(GAMEDATA, GAME);
	initializeGame(GAME);
	initaializeSnake(SNAKE);
	initializeColors(COLOR, GAME->screen);
	initializeBlueDot(BLUE);
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

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
	}
	
	initializeEverything(&GAME, &GAMEDATA, &COLOR, &SNAKE, &BLUE);

	while (!GAMEDATA.quit)
	{
		fillScreen(&GAME, &COLOR);
		updateGameData(&GAMEDATA);
		writeInfoScreen(&GAME, &COLOR, &GAMEDATA, &SNAKE);
		drawReqScreen(&GAME, &COLOR, &GAMEDATA);
		handleSnake(&SNAKE, &COLOR, &GAME, &GAMEDATA);
		handleBlueDot(&BLUE, &SNAKE, &GAME);
		updateScreen(&GAME);

		// obsługa zdarzeń (o ile jakieś zaszły) / handling of events (if there were any)
		while (SDL_PollEvent(&GAME.event))
		{
			switch (GAME.event.type)
			{
			case SDL_KEYDOWN:														//button is pressed
				if (GAME.event.key.keysym.sym == SDLK_ESCAPE) GAMEDATA.quit = 1;
				else if (GAME.event.key.keysym.sym == SDLK_n)
				{
					initaializeSnake(&SNAKE);	
					GAMEDATA.worldTime = 0;
					GAMEDATA.snakeSpeed = SNAKE_INITIAL_SPEED;
				}
				else if (GAME.event.key.keysym.sym == SDLK_UP && SNAKE.dir != 's')
				{
					SNAKE.dir = 'w';
					break;															//there is a bug that if you press quickly d then s you can do 180
				}																	//this will can be fixed with something like gameTick, but for events
																					//also 'n' and 'esc' must bypass this requirement
				else if (GAME.event.key.keysym.sym == SDLK_DOWN && SNAKE.dir != 'w')
				{
					SNAKE.dir = 's';
					break;
				}

				else if (GAME.event.key.keysym.sym == SDLK_LEFT && SNAKE.dir != 'd')
				{
					SNAKE.dir = 'a';
					break;
				}

				else if (GAME.event.key.keysym.sym == SDLK_RIGHT && SNAKE.dir != 'a')
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

		GAMEDATA.frames++;
	};

	// zwolnienie powierzchni / freeing all surfaces
	freeSurfaces(&GAME);

	SDL_Quit();
	return 0;
};


//code fragments

//spinning eti wheel
//DrawSurface(screen, eti,
	 //           SCREEN_WIDTH / 2 + sin(distance) * SCREEN_HEIGHT / 3,
	//	    SCREEN_HEIGHT / 2 + cos(distance) * SCREEN_HEIGHT / 3);
// tryb pełnoekranowy / fullscreen mode
//	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
//	                                 &window, &renderer);
// wyłączenie widoczności kursora myszy
	//SDL_ShowCursor(SDL_DISABLE);
//sprintf(GAMEDATA->text, "Debbug info: snakeWorldTime = %.1lf s, snake[0]  %.0d POINTS: %d", GAMEDATA->snakeWorldTime, SNAKE->x[0], SNAKE->points);

// console window is not visible, to see the printf output
	// the option:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// must be changed to "Console"
	//printf("printf output goes here\n");


