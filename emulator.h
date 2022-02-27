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
	Emulator() : cpu(main_ram), main_ram(ppu, cart), 
		ppu_ram(cart), ppu(ppu_ram), screen(window)  {}
	MainRAM main_ram;
	CPU cpu;
	PPU ppu;
	PPURAM ppu_ram;
	Cartridge cart;
	VideoScreen screen;
	sf::RenderWindow window;

	void load_cartridge(std::string file);
};
#endif // !EMULATOR_H