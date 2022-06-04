#include "application.h"

uint8_t application::rng(){ // query for a random value 0-255
	std::uniform_int_distribution< uint8_t > dist( 0, 255 );
	return dist( *gen );
}

application::application() {
	// create the rng object
	std::random_device rd;
	std::seed_seq s{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
	gen = std::make_shared< std::mt19937 >( s );

	// initializing addressable arrays
	for ( int i = 0; i < vregister_size; i++ ) {
		// m_vregisters[ i ] = 0;
		m_vregisters[ i ] = rng(); // some bullshit value to visualize
	}

	for ( int i = 0; i < stack_size; i++ ) {
		// m_address_stack[ i ] = 0;
		m_address_stack[ i ] = i; // some nonzero init ...
	}

	for ( int y = 0; y < bufferHeight; y++ ) {
		for ( int x = 0; x < bufferWidth; x++ ) {
			// a uint8_t per pixel wastes 7 bits but we're not hurting for memory so it's probably not important for this implementation
			m_frame_buffer[ x + y * bufferWidth ] = 0;
		}
	}

	for ( int i = 0; i < ram_size; i++ ) {
		m_ram[ i ] = 0;
	}

	// load font set into memory
	std::memcpy( m_ram, m_font, 80 );
	m_index_register = 0;
	m_program_counter = pc_start;
	m_stack_pointer = 16;
	m_delay_timer = 0;
	m_sound_timer = 0;
	m_ticks = 0;
	m_file_length = 0;


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

void application::loadRom(const std::string filename) {
    std::ifstream file(filename, std::fstream::binary);
    if (!file.is_open()) {
        file.seekg(0, file.end);
        m_file_length = file.tellg();
        file.seekg(0, file.beg);

        if (m_file_length > (0xF000 - 0x200))
            throw std::runtime_error("Could not load file, file is too large.");
        else if (m_file_length == 0)
            throw std::runtime_error("Empty file.");

        file.read((char*)m_ram + pc_start, m_file_length);
        file.close();
    }
    else {
        throw std::runtime_error("Could not open file.");
    }
}

void application::clearRom() {
    for (int i = pc_start; i < pc_start + m_file_length; i++)
        m_ram[i] = 0;
}

void application::frameClear() {
	for ( int x = 0; x < bufferWidth; x++ ) {
		for ( int y = 0; y < bufferHeight; y++ ) {
			m_frame_buffer[ x + y * bufferWidth ] = 0;
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

/*
nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
n or nibble - A 4-bit value, the lowest 4 bits of the instruction
x - A 4-bit value, the lower 4 bits of the high byte of the instruction
y - A 4-bit value, the upper 4 bits of the low byte of the instruction
kk or byte - An 8-bit value, the lowest 8 bits of the instruction
*/
void application::tick() {
    // shift the first 8bit val into the higher values of instruction, OR the second 8bit val to build the instruction
    uint16_t instruction = m_ram[m_program_counter] << 8 | m_ram[m_program_counter + 1];
    m_program_counter += 2;

    /* TODO
    better errors
    implement drawing and keyInput functions
    rewrite instruction documentation
    */

    // op code names are written out with the capital letters being those of the respective abbreviated name i.e. JP == JumP
    switch (instruction & 0xF000 >> 12) {
    case 0x0:
        if (instruction == 0x00E0) //CLearScreen
            frameClear();
        else // RETurn
            m_program_counter = m_address_stack[--m_stack_pointer];
        break;
    case 0x1: // JumP addr
        m_program_counter = instruction & 0x0FFF;
        break;
    case 0x2: // CALL addr
        m_address_stack[m_stack_pointer++] = m_program_counter;
        m_program_counter = instruction & 0x0FFF;
        break;
    case 0x3: // SkipEqual Vx, byte
        if (m_vregisters[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF))
            m_program_counter += 2;
        break;
    case 0x4: // SkipNotEqual Vx, byte
        if (m_vregisters[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF))
            m_program_counter += 2;
        break;
    case 0x5: // SkipEqual Vx, Vy
        if (m_vregisters[(instruction & 0x0F00) >> 8] == m_vregisters[(instruction & 0x00F0) >> 4])
            m_program_counter += 2;
        break;
    case 0x6: // LoaD Vx, byte
        m_vregisters[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;
        break;
    case 0x7: // ADD Vx, byte
        m_vregisters[(instruction & 0x0F00) >> 8] += instruction & 0x00FF;
        break;
    case 0x8:
        switch (instruction & 0x000F) {
        case 0x0: // LoaD Vx, Vy
            m_vregisters[(instruction & 0x0F00) >> 8] = m_vregisters[(instruction & 0x00F0) >> 4];
            break;
        case 0x1: // OR Vx, Vy
            m_vregisters[(instruction & 0x0F00) >> 8] |= m_vregisters[(instruction & 0x00F0) >> 4];
            break;
        case 0x2: // AND Vx, Vy
            m_vregisters[(instruction & 0x0F00) >> 8] &= m_vregisters[(instruction & 0x00F0) >> 4];
            break;
        case 0x3: // XOR Vx, Vy
            m_vregisters[(instruction & 0x0F00) >> 8] ^= m_vregisters[(instruction & 0x00F0) >> 4];
            break;
        case 0x4: // ADD Vx, Vy
            m_vregisters[0xF] = ((int)m_vregisters[(instruction & 0x0F00) >> 8] + (int)m_vregisters[(instruction & 0x00F0) >> 4]) > 255;
            m_vregisters[(instruction & 0x0F00) >> 8] += m_vregisters[(instruction & 0x00F0) >> 4];
            break;
        case 0x5: // SUBtract Vx, Vy
            m_vregisters[0xF] = m_vregisters[(instruction & 0x0F00) >> 8] > m_vregisters[(instruction & 0x00F0) >> 4];
            m_vregisters[(instruction & 0x0F00) >> 8] -= m_vregisters[(instruction & 0x00F0) >> 4];
            break;
        case 0x6: // SHiftRight Vx {, Vy}
            m_vregisters[0xF] = (m_vregisters[(instruction & 0x0F00) >> 8] & 0b00000001) == 1;
            m_vregisters[(instruction & 0x0F00) >> 8] >>= 1;
            break;
        case 0x7: // SUBtractN Vx, Vy
            m_vregisters[0xF] = m_vregisters[(instruction & 0x00F0) >> 4] > m_vregisters[(instruction & 0x0F00) >> 8];
            m_vregisters[(instruction & 0x0F00) >> 8] = m_vregisters[(instruction & 0x00F0) >> 4] - m_vregisters[(instruction & 0x0F00) >> 8];
            break;
        case 0xE: // SHiftLeft Vx {, Vy}
            m_vregisters[0xF] = (m_vregisters[(instruction & 0x0F00) >> 8] & 0b10000000) == 1;
            m_vregisters[(instruction & 0x0F00) >> 8] <<= 1;
            break;
        default:
            std::cout << "err: instruction not found for 0x8 op code." << "\n";
            break;
        };
				break;
    case 0x9: // SkipNotEqual Vx, Vy
        if (m_vregisters[(instruction & 0x0F00) >> 8] != m_vregisters[(instruction & 0x00F0) >> 4])
            m_program_counter += 2;
        break;
    case 0xA: // LoaD I, addr
        m_index_register = instruction & 0x0FFF;
        break;
    case 0xB: // JumP V0, addr
        m_program_counter = (instruction & 0x0FFF) + m_vregisters[0];
        break;
    case 0xC: //RaNDom Vx,
        m_vregisters[(instruction & 0x0F00) >> 8] = rng() & (instruction & 0x00FF);
        break;
    case 0xD: // DRaW Vx, Vy, nibble
        // not implemented
					// add touched pixels to drawlist
        break;
    case 0xE: // instrs Ex9E and ExA1 here
        // not implemented
        break;
    case 0xF:
        switch (instruction & 0x00FF) {
        case 0x07: // LoaD Vx, DT
            m_vregisters[(instruction & 0x0F00) >> 8] = m_delay_timer;
            break;
        case 0x0A: // LoaD Vx, K - wait for key event to occur
            m_vregisters[(instruction & 0x0F00) >> 8] = keyInput();
            break;
        case 0x15: // LoaD DT, Vx
            m_delay_timer = m_vregisters[(instruction & 0x0F00) >> 8];
            break;
        case 0x18: // LoaD ST, Vx
            m_sound_timer = m_vregisters[(instruction & 0x0F00) >> 8];
            break;
        case 0x1E: // ADD I, Vx
            // m_vregisters[0xF] = (m_index_register + m_vregisters[(instruction & 0x0F00) >> 8] & 0xFFFF0000) != 0; // add parenthesis
            // m_index_register = m_index_register + m_vregisters[(instruction & 0x0F00) >> 8] & 0x0000FFFF; // add parenthesis
            break;
        case 0x29: // LoaD F, Vx
            m_index_register = m_vregisters[(instruction & 0x0F00) >> 8] * 5;
            break;
        case 0x33: // LoaD B, Vx
						{ // scoped because of variable declaration
							uint8_t vx = m_vregisters[(instruction & 0x0F00) >> 8];
							m_ram[m_index_register] = (vx / 100) % 10;
							m_ram[m_index_register + 1] = (vx / 10) % 10;
							m_ram[m_index_register + 2] = vx % 10;
						}
            break;
        case 0x55: // LoaD [I], Vx
            for ( int i = 0; i < ( ( instruction & 0x0F00 ) >> 8 ); i++ ) {
							m_ram[ m_index_register + i ] = m_vregisters[ i ];
						}
            m_index_register += ((instruction & 0x0F00) >> 8) + 1;
            break;
        case 0x65: // LoaD Vx, [I]
            for (int i = 0; i < ((instruction & 0x0F00) >> 8); i++) {
							m_vregisters[i] = m_ram[m_index_register + i];
						}
            m_index_register += ((instruction & 0x0F00) >> 8) + 1;
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
