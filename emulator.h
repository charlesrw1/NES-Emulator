#ifndef EMULATOR_H
#define EMULATOR_H

#include "cpu.h"
#include "ppu.h"
#include "mainram.h"
#include "ppuram.h"
#include "mapper.h"
#include "cartridge.h"
#include "video_screen.h"
#include "SFML/Graphics.hpp"

// Class structure:
// 
// APU -----+
//			V
// CPU -> MAINBUS --------------+
//			V					V
//		   PPU -> PPUBUS --> MAPPER --> CARTRIDGE
//			V
//		  SCREEN

class Emulator
{
public:
	Emulator(sf::RenderWindow& window) : cpu(main_ram), main_ram(ppu, cart),
		ppu_ram(cart), ppu(ppu_ram, screen), screen(window), window(window)  {}
	MainRAM main_ram;
	CPU cpu;
	PPU ppu;
	PPURAM ppu_ram;
	Cartridge cart;
	VideoScreen screen;
	sf::RenderWindow& window;

	void load_cartridge(std::string file);
	const sf::Keyboard::Key input_keys[8] = { sf::Keyboard::Z,sf::Keyboard::X,sf::Keyboard::C,sf::Keyboard::V,
		sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Left, sf::Keyboard::Right};
	void poll_input()
	{
		uint8_t input = 0;
		for (int i = 0; i < 8; i++) {
			if (sf::Keyboard::isKeyPressed(input_keys[i])) {
				input |= (1 << i);
			}
		}
		main_ram.cached_controller_port1 = input;
	}
	void step()
	{
		uint64_t total_cycles = 0;
		// About 1 frame
		while (total_cycles < 29780) {

			for (int i = 0; i < cpu.cycles * 3; i++) {
				ppu.clock();
			}
			cpu.cycles = 0;

			if (ppu.generate_NMI && ppu.send_nmi_output) {
				ppu.send_nmi_output = false;
				cpu.nmi();
			}

			cpu.step();
			total_cycles += cpu.cycles;
		}
	}
};
#endif // !EMULATOR_H