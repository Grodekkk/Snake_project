#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	600
#define SCREEN_HEIGHT	600
#define SNAKE_SIZE 9
#define SNAKE_SPEED 0.5

//================DATA STRUCTURES====================================================
typedef struct {
	SDL_Surface *snaker;
	int x[SNAKE_SIZE];
	int y[SNAKE_SIZE];
	char dir;				//direction of the snake: w = up, a = left, s = down, d = right
	int crash;				// 1 if snake hot itself, 0 otherwise
}worm;


// narysowanie napisu txt na powierzchni screen, zaczynając od punktu (x, y)
// charset to bitmapa 128x128 zawierająca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images


void DrawString(SDL_Surface *screen, int x, int y, const char *text, SDL_Surface *charset) 
{
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
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


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt środka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
	};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};


// rysowanie linii o długości l w pionie (gdy dx = 0, dy = 1) 
// bądź poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) 
{
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
};


// rysowanie prostokąta o długości boków l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) 
{
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

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
	updateSnakeBody(SNAKE); // update every "cell" of the snake without a head
	switch(SNAKE->dir)
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
	int substitution = SNAKE->x[0];
	for (int i = 1; i < SNAKE_SIZE; i++)
	{
		substitution -= 25;
		SNAKE->y[i] = SNAKE->y[0];
		SNAKE->x[i] = substitution;
	}
}


// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed;

	//game speed variables, working with delta variable provided with the template
	double snakeWorldTime;											//mierzy czas od ostatniego ruchu węża
	double snakeSpeed;												//do ustawiania prędkości węża we wspolpracy z snakeworldTime
	int tickPassed;													//1
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Surface *eti;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;

	
	worm SNAKE;

	initaializeSnake(&SNAKE);

	// okno konsoli nie jest widoczne, jeżeli chcemy zobaczyć
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmienić na "Console"
	// console window is not visible, to see the printf output
	// the option:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// must be changed to "Console"
	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
		}

	// tryb pełnoekranowy / fullscreen mode
//	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
//	                                 &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
	                                 &window, &renderer);
	if(rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
		};
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Szablon do zdania drugiego 2017");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
	                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
	                           SDL_TEXTUREACCESS_STREAMING,
	                           SCREEN_WIDTH, SCREEN_HEIGHT);


	// wyłączenie widoczności kursora myszy
	//SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};
	SDL_SetColorKey(charset, true, 0x000000);

	eti = SDL_LoadBMP("./eti.bmp");
	//snaker = SDL_LoadBMP("./snaker.bmp");  //calyprotokol co jak sie nie wczyta
	if(eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};

	//definetly can be moved into a structure and a separate function
	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	t1 = SDL_GetTicks();

	//this can be moved into a function, maybe into a structure
	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	distance = 0;
	etiSpeed = 1;

	tickPassed = 0;
	snakeWorldTime = 0;
	snakeSpeed = SNAKE_SPEED;

	while(!quit) {
		t2 = SDL_GetTicks();

		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplynał od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach
		// here t2-t1 is the time in milliseconds since
		// the last screen was drawn
		// delta is the same time in seconds
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;


		//nowe, dodane przeze mnie
		snakeWorldTime += delta;
		if (snakeWorldTime >= snakeSpeed)
		{
			tickPassed = 1;
			snakeWorldTime = 0;
		}

		distance += etiSpeed * delta;

		SDL_FillRect(screen, NULL, czarny);
		//drawing game area right now it will be fixed, we will make it interchangeable later
		DrawRectangle(screen, 110, 60, SCREEN_WIDTH - 195, SCREEN_HEIGHT - 95, zielony, niebieski);

		//DrawSurface(screen, eti,
		 //           SCREEN_WIDTH / 2 + sin(distance) * SCREEN_HEIGHT / 3,
		//	    SCREEN_HEIGHT / 2 + cos(distance) * SCREEN_HEIGHT / 3);
		if (tickPassed == 1)
		{
			moveSnake(&SNAKE);					//how to handle game speed?    ONE FUNCTION HANDLESNAKE?
			
			tickPassed = 0;
		}
		drawSnake(&SNAKE, screen); // to musi byc poza petla aby nie bylo czyszczone przez czarny prostokat
		

		fpsTimer += delta;
		if(fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
			};

		// tekst informacyjny / info text
		DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
		//            "template for the second project, elapsed time = %.1lf s  %.0lf frames / s"
		sprintf(text, "Debbuging info: snakeWorldTime = %.1lf s, snake[0]  %.0d ", snakeWorldTime, SNAKE.x[0]);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
		//	      "Esc - exit, \030 - faster, \031 - slower"
		sprintf(text, "Esc - wyjscie, n - new game");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);


		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
//		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// obsługa zdarzeń (o ile jakieś zaszły) / handling of events (if there were any)
		while(SDL_PollEvent(&event)) 
		{
			switch(event.type) 
			{
				case SDL_KEYDOWN:														//button is pressed
					if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					else if (event.key.keysym.sym == SDLK_n)
					{
						initaializeSnake(&SNAKE);															//there is a bug that if you press quickly d then s you can do 180
					}
					else if (event.key.keysym.sym == SDLK_UP && SNAKE.dir != 's')
					{
						SNAKE.dir = 'w';
						break;															//there is a bug that if you press quickly d then s you can do 180
					}																	//this will can be fixed with something like gameTick, but for events
																						//also 'n' and 'esc' must bypass this requirement
					else if (event.key.keysym.sym == SDLK_DOWN && SNAKE.dir != 'w')
					{
						SNAKE.dir = 's';
						break;
					}
						
					else if (event.key.keysym.sym == SDLK_LEFT && SNAKE.dir != 'd')
					{
						SNAKE.dir = 'a';
						break;
					}
						
					else if (event.key.keysym.sym == SDLK_RIGHT && SNAKE.dir != 'a')
					{
						SNAKE.dir = 'd';
						break;
					}
						
					break;
				case SDL_KEYUP:															//button is released
					//snakeSpeed = SNAKE_SPEED;
					break;
				case SDL_QUIT:
					quit = 1;
					break;
			};
		};

		frames++;
		};

	// zwolnienie powierzchni / freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
	};
