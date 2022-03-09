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
		ppu_ram(cart), ppu(ppu_ram, screen), screen(window), window(window)  {}
	MainRAM main_ram;
	CPU cpu;
	PPU ppu;
	PPURAM ppu_ram;
	Cartridge cart;
	VideoScreen screen;
	sf::RenderWindow& window;

	bool load_cartridge(std::string file);

	void step();
};
#endif // !EMULATOR_H