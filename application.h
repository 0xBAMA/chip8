#ifndef APPLICATION
#define APPLICATION

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

class application {
public:
	application();
	~application();

	// main loop
	bool update();

	// pick events off the queue
	void handleEvents();

	// handle particular events
	void handleEvent( uint8_t eventCode );

	// SDL Resource Handles
	SDL_Window* window;
	SDL_Renderer* renderer;

	// program state
	bool running;
};

#endif
