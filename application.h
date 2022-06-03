#define unixcheck (!defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))))
#if unixcheck
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_image.h>
#else
	#define SDL_MAIN_HANDLED
	#include <SDL.h>
	#include <SDL_image.h>
#endif
#undef unixcheck

#include <iostream>

// Window defines
const int windowWidth = 1000;
const int windowHeight = 500;
const char* windowTitle = "CHIP-8 Interpreter";

class application {
public:
	application() {
		// SDL Initialization
		SDL_Init( SDL_INIT_EVERYTHING );
		window = SDL_CreateWindow( windowTitle, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN );
		renderer = SDL_CreateRenderer( window, -1, 0 );
		running = true;
	}

	~application() {
		SDL_DestroyRenderer( renderer );
		SDL_DestroyWindow( window );
		SDL_Quit();
	}

	bool update() {
		static int i = 100;
		SDL_Delay( 100 );
		std::cout << i-- << std::endl;
		return !( i == 0 );
	}

	// SDL Resource Handles
	SDL_Window* window;
	SDL_Renderer* renderer;

	// program state
	bool running;
};
