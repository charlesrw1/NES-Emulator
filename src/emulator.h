#ifndef EMULATOR_H
#define EMULATOR_H
#include <iosfwd>

#include "cpu.h"
#include "ppu.h"
#include "mainram.h"
#include "ppuram.h"
#include "mapper.h"
#include "cartridge.h"
#include "video_screen.h"
#include "SFML/Graphics.hpp"
#include <vector>

class Emulator;
struct EmulatorStatus
{
	uint8_t ar, xr, yr;
	uint16_t pc;
	uint8_t sp;
	bool nf : 1, vf : 1, df : 1, inf : 1, zf : 1, cf : 1;
	uint64_t total_cycles;

	uint32_t rom_addr;
	uint16_t scanline;
	uint16_t ppu_cycle;

	uint16_t ppu_addr;
	uint16_t ppu_temp_addr;

	uint8_t opcode;

	void print(std::ostream& stream);
	void save_state(Emulator& emu);
};
// Class structure:
// 
// APU(TBD)-+
//			V
// CPU -> MAINBUS --------------+
//			V					V
//		   PPU -> PPUBUS --> MAPPER --> CARTRIDGE
//			V
//		  SCREEN
const sf::Keyboard::Key input_keys[8] = { sf::Keyboard::A,sf::Keyboard::S,sf::Keyboard::Q,sf::Keyboard::W,
		sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Left, sf::Keyboard::Right };
class Emulator
{
public:
	Emulator(sf::RenderWindow& window) : cpu(main_ram), main_ram(ppu, cart),
		ppu_ram(cart), ppu(ppu_ram, screen), screen(window), window(window), ring_buffer(20)  {}
	MainRAM main_ram;
	CPU cpu;
	PPU ppu;
	PPURAM ppu_ram;
	Cartridge cart;
	VideoScreen screen;
	sf::RenderWindow& window;

	bool load_cartridge(std::string file);

	void step();

	int ring_buffer_index = 0;
	int ring_buffer_elements = 0;
	std::vector<EmulatorStatus> ring_buffer;
};

#endif // !EMULATOR_H