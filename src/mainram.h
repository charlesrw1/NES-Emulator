#include <cstdint>
#ifndef MAINRAM_H
#define MAINRAM_H

struct PPU;
struct Cartridge;
class MainRAM
{
public:
	MainRAM(PPU& ppu, Cartridge& cart);
	~MainRAM();
	uint8_t read_byte(uint16_t addr);
	void write_byte(uint16_t addr, uint8_t val);

	PPU& ppu;
	Cartridge& cart;

	uint8_t cached_controller_port1;
	uint8_t controller_port_1;


	uint8_t* memory;

	bool DMA_request;
	uint8_t DMA_page;
};
#endif // !MAINRAM_H