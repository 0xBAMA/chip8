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

#include <cstdint>
#include <iostream>

constexpr int vregister_size = 16;
constexpr int stack_size = 16;
constexpr int buffer_width = 32;
constexpr int buffer_length = 64;
constexpr int ram_size = 0xFFF;
constexpr int pc_start = 0x200;

class application {
private:
	// registers
	uint8_t m_vregisters[vregister_size];
	uint16_t m_index_register;
	uint16_t m_program_counter;
	uint8_t m_stack_pointer;
	uint8_t m_delay_timer;
	uint8_t m_sound_timer;

	// addressable arrays
	uint16_t m_address_stack[stack_size];
	uint8_t m_frame_buffer[buffer_width][buffer_length];
	uint8_t m_ram[ram_size];

	int m_ticks;
	int m_file_length;

	uint8_t m_font[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F 
	};

public:
	application();
	~application();

	void loadRom(const std::string filename);
	void clearRom();

	void frameClear();

	void tick();
	uint8_t keyInput();

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
