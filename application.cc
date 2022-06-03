#include "application.h"

// Window defines
const int windowWidth = 1000;
const int windowHeight = 500;
const char* windowTitle = "CHIP-8 Interpreter";

application::application() {
	// SDL Initialization
	SDL_Init( SDL_INIT_EVERYTHING );
	window = SDL_CreateWindow( windowTitle, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN );
	renderer = SDL_CreateRenderer( window, -1, 0 );
	running = true;
}

application::~application() {
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();
}

bool application::update() {
	static int i = 10;
	SDL_Delay( 100 );
	std::cout << i-- << std::endl;
	return !( i == 0 );
}
