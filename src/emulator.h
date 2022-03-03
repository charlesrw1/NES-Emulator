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
#include "controller.h"

// Class structure:
// 
// APU -----+
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
		ppu_ram(cart), ppu(ppu_ram, screen), screen(window), window(window)  {}
	MainRAM main_ram;
	CPU cpu;
	PPU ppu;
	PPURAM ppu_ram;
	Cartridge cart;
	VideoScreen screen;
	sf::RenderWindow& window;

	void load_cartridge(std::string file);

	void step()
	{
		static int cycles_since_nmi = 0;
		static bool in_nmi = false;
		uint64_t total_cycles = 0;
		// About 1 frame
		//main_ram.cont1.update();

		uint8_t input = 0;
		for (int i = 0; i < 8; i++) {
			if (sf::Keyboard::isKeyPressed(input_keys[i])) {
				input |= (1 << i);
			}
		}
		main_ram.cached_controller_port1 = input;

		while (total_cycles < 29780) {


			for (int i = 0; i < cpu.cycles * 3; i++) {
				ppu.clock();
			}
			cpu.cycles = 0;

			if (main_ram.DMA_request) {
				uint8_t* oam_ptr = reinterpret_cast<uint8_t*>(ppu.OAM);
				for (int i = 0; i < 256; i++) {
					// NESDEV wiki isn't clear, I assume the writes wrap around if oam_addr is anything greater than 0
					uint8_t data = main_ram.read_byte((main_ram.DMA_page << 8) | uint8_t(i));
					oam_ptr[uint8_t(i + ppu.oam_addr)] = data;
				}
				main_ram.DMA_request = false;
				// Actually this could be either 513/514, maybe a problem
				ppu.cycle += 513;
			}

			if (ppu.generate_NMI && ppu.send_nmi_output) {
				ppu.send_nmi_output = false;
				in_nmi = true;
				cpu.nmi();
			}

			cpu.step();
			
			
			total_cycles += cpu.cycles;
			if (in_nmi) {
				cycles_since_nmi += cpu.cycles;
			}
		}
	}
};
#endif // !EMULATOR_H