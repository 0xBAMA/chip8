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

// #include <cstdint>
#include <cstring>
#include <fstream>
#include <random>
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

constexpr int vregister_size = 16;
constexpr int stack_size = 16;
constexpr int bufferHeight = 32;
constexpr int bufferWidth = 64;
constexpr int ram_size = 0xF000;
constexpr int pc_start = 0x200;

// Window defines
constexpr int windowWidth = 1280;
constexpr int windowHeight = 800;

struct graphicalOffsets_t {
	// Program Counter
	const int PCBaseX = 174;
	const int PCBaseY = 488;
	const int PCBoxDim = 16;
	const int PCOffset = 2;

	// Registers
	const int RBaseX = 177;
	const int RBaseY = 524;
	const int RBoxDim = 8;
	const int ROffset = 2;

	// Timers
	const int TBaseX = 177;
	const int TBaseY = 700;
	const int TBoxDim = 8;
	const int TOffset = 2;

	// Stack
	const int SBaseX = 300;
	const int SBaseY = 524;
	const int SBoxDim = 8;
	const int SOffset = 2;

	// Stack Pointer
	const int SPBaseX = 300;
	const int SPBaseY = 699;
	const int SPBoxDim = 8;
	const int SPOffset = 2;

	// Input State
	const int IBaseX = 485;
	const int IBaseY = 497;
	const int IBoxDim = 36;
	const int IOffsetX = 11;
	const int IOffsetY = 17;

	// Screen
	const int SCRBaseX = 20;
	const int SCRBaseY = 19;
	const int SCRBoxDim = 11;
	const int SCROffset = 2;

	// Memory
	const int MBaseX = 869;
	const int MBaseY = 19;
	const int MBoxDim = 2;
	const int MOffset = 1;
	const int MColOffset = 25;
};

class application {
private:
	// registers
	uint8_t m_vregisters[ vregister_size ];
	uint16_t m_index_register;
	uint16_t m_program_counter;
	uint8_t m_stack_pointer;
	uint8_t m_delay_timer;
	uint8_t m_sound_timer;

	// addressable arrays
	uint16_t m_address_stack[ stack_size ];
	uint8_t m_frame_buffer[ bufferWidth * bufferHeight ];
	uint8_t m_ram[ ram_size ];

	int m_ticks;
	int m_file_length;

	uint8_t m_font[ 80 ] = {
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

	std::shared_ptr< std::mt19937 > gen;
	uint8_t rng(); // get a sample of the distribution

public:
	application();
	~application();

	void loadRom(const std::string filename);
	void clearRom();

	void frameClear();

	void tick();
	uint8_t keyInput(); // stalls until a key is pressed
	uint16_t inputStateSinceLastFrame = 0;

	// main loop for display
	bool update();
	void updateRegisters();
	void updateScreen();
	void updateMemory();
	void updateGraphicsPartial();
	void updateGraphicsFull();
	void clearDrawLists();
	void drawBackground();
	void addRectangle( bool bitState, SDL_Rect footprint );
	graphicalOffsets_t go;

	std::vector< SDL_Rect > drawListTrue;
	std::vector< SDL_Rect > drawListFalse;

	// SDL Resource Handles
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	const char* windowTitle = "CHIP-8 Interpreter";

	// program state
	bool running;
};

#endif
