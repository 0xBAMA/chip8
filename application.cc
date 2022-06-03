#include "application.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "time.h"

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

    // for seeding the rng, only used by Cxkk
    srand(time(nullptr));

    // initializing addressable arrays
    for (int i = 0; i < vregister_size; i++)
        m_vregisters[i] = 0;

    for (int i = 0; i < stack_size; i++)
        m_address_stack[i] = 0;

    for (int row = 0; row < buffer_width; row++)
        for (int col = 0; col < buffer_length; col++)
            m_frame_buffer[row][col] = 0;

    for (int i = 0; i < ram_size; i++)
        m_ram[i] = 0;

    // load font set into memory
    std::memcpy(m_ram, m_font, 80);
    m_index_register = 0;
    m_program_counter = pc_start;
    m_stack_pointer = 0;
    m_delay_timer = 0;
    m_sound_timer = 0;
    m_ticks = 0;
    m_file_length = 0;
}

application::~application() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void application::loadRom(const std::string filename) {
    std::ifstream file(filename, std::fstream::binary);
    if (!file.is_open()) {
        file.seekg(0, file.end);
        m_file_length = file.tellg();
        file.seekg(0, file.beg);

        if (m_file_length > (0xFFF - 0x200))
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
    for (int i = 0; i < buffer_width; i++)
        for (int j = 0; j < buffer_length; j++)
            m_frame_buffer[i][j] = 0;
}

bool application::update() {
	static int i = 10;
	SDL_Delay( 100 );
	std::cout << i-- << std::endl;
	return !( i == 0 );
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
        m_vregisters[(instruction & 0x0F00) >> 8] = (rand() % 256) & (instruction & 0x00FF);
        break;
    case 0xD: // DRaW Vx, Vy, nibble 
        // not implemented
        break;
    case 0xE: // instrs Ex9E and ExA1 here
        // not implemented
        break;
    case 0xF:
        switch (instruction & 0x00FF) {
        case 0x07: // LoaD Vx, DT
            m_vregisters[(instruction & 0x0F00) >> 8] = m_delay_timer;
            break;
        case 0x0A: // LoaD Vx, K
            // implement keyInput
            m_vregisters[(instruction & 0x0F00) >> 8] = keyInput();
            break;
        case 0x15: // LoaD DT, Vx
            m_delay_timer = m_vregisters[(instruction & 0x0F00) >> 8];
            break;
        case 0x18: // LoaD ST, Vx
            m_sound_timer = m_vregisters[(instruction & 0x0F00) >> 8];
            break;
        case 0x1E: // ADD I, Vx
            m_vregisters[0xF] = (m_index_register + m_vregisters[(instruction & 0x0F00) >> 8] & 0xFFFF0000) != 0;
            m_index_register = m_index_register + m_vregisters[(instruction & 0x0F00) >> 8] & 0x0000FFFF;
            break;
        case 0x29: // LoaD F, Vx
            m_index_register = m_vregisters[(instruction & 0x0F00) >> 8] * 5;
            break;
        case 0x33: // LoaD B, Vx
            uint8_t vx = m_vregisters[(instruction & 0x0F00) >> 8];
            m_ram[m_index_register] = (vx / 100) % 10;
            m_ram[m_index_register + 1] = (vx / 10) % 10;
            m_ram[m_index_register + 2] = vx % 10;
            break;
        case 0x55: // LoaD [I], Vx
            for (int i = 0; i < ((instruction & 0x0F00) >> 8); i++)
                m_ram[m_index_register + i] = m_vregisters[i];
            m_index_register += ((instruction & 0x0F00) >> 8) + 1;
            break;
        case 0x65: // LoaD Vx, [I]
            for (int i = 0; i < ((instruction & 0x0F00) >> 8); i++)
                m_vregisters[i] = m_ram[m_index_register + i];
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
