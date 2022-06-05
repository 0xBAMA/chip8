#include "application.h"

uint8_t application::rng() { // query for a random value 0-255
	std::uniform_int_distribution< uint8_t > dist( 0, 255 );
	return dist( *gen );
}

application::application() {
	// create the rng object
	std::random_device rd;
	std::seed_seq s{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
	gen = std::make_shared< std::mt19937 >( s );

	// initializing addressable arrays
	for ( int i = 0; i < vregisterSize; i++ ) {
		// vregisters[ i ] = 0;
		vregisters[ i ] = rng(); // some bullshit value to visualize
	}

	for ( int i = 0; i < stackSize; i++ ) {
		// addressStack[ i ] = 0;
		addressStack[ i ] = i; // some nonzero init ...
	}

	for ( int y = 0; y < bufferHeight; y++ ) {
		for ( int x = 0; x < bufferWidth; x++ ) {
			// a uint8_t per pixel wastes 7 bits but we're not hurting for memory so it's probably not important for this implementation
			frameBuffer[ x + y * bufferWidth ] = 0;
		}
	}

	for ( int i = 0; i < ramSize; i++ ) {
		ram[ i ] = 0;
	}

	// load font set into memory
	std::memcpy( ram, font, 80 );
	indexRegister = 0;
	programCounter = pcStart;
	stackPointer = 16;
	delayTimer = 0;
	soundTimer = 0;
	ticks = 0;
	fileLength = 0;


	// SDL Initialization
	SDL_Init( SDL_INIT_EVERYTHING );
	window = SDL_CreateWindow( windowTitle, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN );
	renderer = SDL_CreateRenderer( window, -1, 0 );
	running = true;

	// copy the mockup to the renderer, as a background image ( titles, labels, etc )
	SDL_Surface* image = IMG_Load( "memoryDisplay.png" );
	texture = SDL_CreateTextureFromSurface( renderer, image );

	// put initial state of the virtual machine in the display
	updateGraphicsFull();
}

application::~application() {
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();
}

void application::loadRom( const std::string filename ) {
	std::ifstream file( filename, std::fstream::binary );
	if (!file.is_open()) {
		file.seekg( 0, file.end );
		fileLength = file.tellg();
		file.seekg( 0, file.beg );

		if ( fileLength > 0xF000 - 0x200 )
			throw std::runtime_error( "Could not load file, file is too large." );
		else if ( fileLength == 0 )
			throw std::runtime_error( "Empty file." );

		file.read( ( char* )ram + pcStart, fileLength );
		file.close();
	}
	else {
		throw std::runtime_error( "Could not open file." );
	}
}

void application::clearRom() {
	for ( int i = pcStart; i < pcStart + fileLength; i++ )
		ram[ i ] = 0;
}

void application::frameClear() {
	for ( int x = 0; x < bufferWidth; x++ ) {
		for ( int y = 0; y < bufferHeight; y++ ) {
			frameBuffer[ x + y * bufferWidth ] = 0;
		}
	}
}

uint8_t application::keyInput() {
	bool gotOne = false;
	/*
	// input values         // keyboard
	+---+---+---+---+       +---+---+---+---+
	| 1 | 2 | 3 | C |       | 1 | 2 | 3 | 4 |
	+---+---+---+---+       +---+---+---+---+
	| 4 | 5 | 6 | D |       | q | w | e | r |
	+---+---+---+---+  <--  +---+---+---+---+
	| 7 | 8 | 9 | E |       | a | s | d | f |
	+---+---+---+---+       +---+---+---+---+
	| A | 0 | B | F |       | z | x | c | v |
	+---+---+---+---+       +---+---+---+---+
	*/
	while ( !gotOne ) {
		SDL_Event event;
		while ( SDL_PollEvent( &event ) ) {
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_1 )
				return 0x1;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_2 )
				return 0x2;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_3 )
				return 0x3;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_4 )
				return 0xC;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_q )
				return 0x4;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_w )
				return 0x5;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_e )
				return 0x6;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_r )
				return 0xD;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_a )
				return 0x7;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_s )
				return 0x8;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_d )
				return 0x9;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_f )
				return 0xE;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_z )
				return 0xA;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_x )
				return 0x0;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_c )
				return 0xB;
			if ( event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_v )
				return 0xF;
		}
	}
	// maybe add a check for escape here, so that you can still abort while this loop waits
	return -1; // because this will be a blocking wait, and nothing can happen on the main thread
	// while it is waiting
}

void application::play() {

}

/*
nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
n or nibble - A 4-bit value, the lowest 4 bits of the instruction
x - A 4-bit value, the lower 4 bits of the high byte of the instruction
y - A 4-bit value, the upper 4 bits of the low byte of the instruction
kk or byte - An 8-bit value, the lowest 8 bits of the instruction
*/
void application::tick() {
	// shift the first 8bit val into the higher values of instruction, OR the second 8bit val to build the instruction
	uint16_t instruction = ram[ programCounter ] << 8 | ram[ programCounter + 1 ];
	programCounter += 2;

	if (soundTimer > 0) {
		play();
		soundTimer--;
	}
	/* TODO
	better errors
	implement drawing
	rewrite instruction documentation
	*/

	// op code names are written out with the capital letters being those of the respective abbreviated name i.e. JP == JumP
	switch ( instruction & 0xF000 >> 12 ) {
	case 0x0:
		if ( instruction == 0x00E0 ) //CLearScreen
			frameClear();
		else // RETurn
			programCounter = addressStack[ --stackPointer ];
		break;
	case 0x1: // JumP addr
		programCounter = instruction & 0x0FFF;
		break;
	case 0x2: // CALL addr
		addressStack[ stackPointer++ ] = programCounter;
		programCounter = instruction & 0x0FFF;
		break;
	case 0x3: // SkipEqual Vx, byte
		if ( vregisters[ ( instruction & 0x0F00 ) >> 8 ] == ( instruction & 0x00FF ) )
			programCounter += 2;
		break;
	case 0x4: // SkipNotEqual Vx, byte
		if ( vregisters[ ( instruction & 0x0F00 ) >> 8 ] != ( instruction & 0x00FF ) )
			programCounter += 2;
		break;
	case 0x5: // SkipEqual Vx, Vy
		if (vregisters[ ( instruction & 0x0F00 ) >> 8 ] == vregisters[ ( instruction & 0x00F0 ) >> 4 ] )
			programCounter += 2;
		break;
	case 0x6: // LoaD Vx, byte
		vregisters[ ( instruction & 0x0F00 ) >> 8 ] = instruction & 0x00FF;
		break;
	case 0x7: // ADD Vx, byte
		vregisters[ ( instruction & 0x0F00 ) >> 8 ] += instruction & 0x00FF;
		break;
	case 0x8:
		switch ( instruction & 0x000F ) {
		case 0x0: // LoaD Vx, Vy
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] = vregisters[ ( instruction & 0x00F0 ) >> 4 ];
			break;
		case 0x1: // OR Vx, Vy
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] |= vregisters[ ( instruction & 0x00F0 ) >> 4 ];
			break;
		case 0x2: // AND Vx, Vy
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] &= vregisters[ ( instruction & 0x00F0 ) >> 4 ];
			break;
		case 0x3: // XOR Vx, Vy
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] ^= vregisters[ ( instruction & 0x00F0 ) >> 4 ];
			break;
		case 0x4: // ADD Vx, Vy
			vregisters[ 0xF ] = ( ( int )vregisters[ ( instruction & 0x0F00 ) >> 8 ] + ( int )vregisters[ ( instruction & 0x00F0 ) >> 4 ] ) > 255;
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] += vregisters[(instruction & 0x00F0) >> 4];
			break;
		case 0x5: // SUBtract Vx, Vy
			vregisters[ 0xF ] = vregisters[ ( instruction & 0x0F00 ) >> 8 ] > vregisters[ ( instruction & 0x00F0 ) >> 4 ];
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] -= vregisters[ ( instruction & 0x00F0 ) >> 4 ];
			break;
		case 0x6: // SHiftRight Vx {, Vy}
			vregisters[ 0xF ] = ( vregisters[ ( instruction & 0x0F00 ) >> 8 ] & 0b00000001 ) == 1;
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] >>= 1;
			break;
		case 0x7: // SUBtractN Vx, Vy
			vregisters[ 0xF ] = vregisters[ ( instruction & 0x00F0 ) >> 4 ] > vregisters[ ( instruction & 0x0F00 ) >> 8 ];
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] = vregisters[ ( instruction & 0x00F0 ) >> 4 ] - vregisters[ ( instruction & 0x0F00 ) >> 8 ];
			break;
		case 0xE: // SHiftLeft Vx {, Vy}
			vregisters[ 0xF ] = ( vregisters[ ( instruction & 0x0F00 ) >> 8 ] & 0b10000000 ) == 1;
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] <<= 1;
			break;
		default:
			std::cout << "err: instruction not found for 0x8 op code." << "\n";
			break;
		};
		break;
	case 0x9: // SkipNotEqual Vx, Vy
		if ( vregisters[ ( instruction & 0x0F00 ) >> 8 ] != vregisters[ ( instruction & 0x00F0 ) >> 4 ] )
			programCounter += 2;
		break;
	case 0xA: // LoaD I, addr
		indexRegister = instruction & 0x0FFF;
		break;
	case 0xB: // JumP V0, addr
		programCounter = ( instruction & 0x0FFF ) + vregisters[ 0 ];
		break;
	case 0xC: //RaNDom Vx,
		vregisters[ ( instruction & 0x0F00 ) >> 8 ] = rng() & ( instruction & 0x00FF );
		break;
	case 0xD: // DRaW Vx, Vy, nibble
		// not implemented
					// add touched pixels to drawlist
		break;
	case 0xE:
		switch ( instruction & 0x00FF ) {
		case 0x9E: // Skip if keymap[Vx] is down
			{
				const Uint8* state = SDL_GetKeyboardState(NULL);
				
				if ( state[ vregisters[ ( instruction & 0x0F00 ) >> 8 ] ] )
					programCounter += 2;
			}
			break;
		case 0xA1: // Skip if keymap[Vx] is up
			{
			const Uint8* state = SDL_GetKeyboardState(NULL);
				
				if ( !state[ vregisters[ ( instruction & 0x0F00 ) >> 8 ] ] )
					programCounter += 2;
			}
			break;
		default:
			std::cout << "could not find op code for 0xE instruction" << "\n";
			break;
		}
		break;
	case 0xF:
		switch ( instruction & 0x00FF ) {
		case 0x07: // LoaD Vx, DT
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] = delayTimer;
			break;
		case 0x0A: // LoaD Vx, K - wait for key event to occur
			vregisters[ ( instruction & 0x0F00 ) >> 8 ] = keyInput();
			break;
		case 0x15: // LoaD DT, Vx
			delayTimer = vregisters[ ( instruction & 0x0F00 ) >> 8 ];
			break;
		case 0x18: // LoaD ST, Vx
			soundTimer = vregisters[ ( instruction & 0x0F00 ) >> 8 ];
			break;
		case 0x1E: // ADD I, Vx
			 vregisters[ 0xF ] = ( indexRegister + vregisters[ ( instruction & 0x0F00 ) >> 8 ] & 0xFFFF0000 ) != 0;
			 indexRegister = indexRegister + vregisters[ ( instruction & 0x0F00 ) >> 8 ] & 0x0000FFFF;
			break;
		case 0x29: // LoaD F, Vx
			indexRegister = vregisters[ ( instruction & 0x0F00 ) >> 8 ] * 5;
			break;
		case 0x33: // LoaD B, Vx
			{ // scoped because of variable declaration
				uint8_t vx = vregisters[ ( instruction & 0x0F00 ) >> 8 ];
				ram[ indexRegister ] = ( vx / 100 ) % 10;
				ram[ indexRegister + 1 ] = ( vx / 10 ) % 10;
				ram[ indexRegister + 2 ] = vx % 10;
			}
			break;
		case 0x55: // LoaD [I], Vx
			for ( int i = 0; i < ( ( instruction & 0x0F00 ) >> 8 ); i++ ) {
				ram[ indexRegister + i ] = vregisters[ i ];
			}
			indexRegister += ( ( instruction & 0x0F00 ) >> 8 ) + 1;
			break;
		case 0x65: // LoaD Vx, [I]
			for ( int i = 0; i < ((instruction & 0x0F00) >> 8); i++ ) {
				vregisters[ i ] = ram[indexRegister + i];
			}
			indexRegister += ( ( instruction & 0x0F00 ) >> 8 ) + 1;
			break;
		default:
			std::cout << "error: instruction not found for 0xF op code." << '\n';
			break;
		};
	default:
		std::cout << "error: op code not found." << '\n';
		break;
	};
}
