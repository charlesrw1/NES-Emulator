#include <cstdint>
#ifndef PPURAM_H
#define PPURAM_H
struct Cartridge;
struct PPURAM
{
	PPURAM(Cartridge& cart);
	~PPURAM();
	// 2KB nametable ram, mirroring from $2000 to $3EFFF
	uint8_t* nametable_ram;
	// Indexs into nametable ram with correct mirroring 
	uint16_t nametable_1, nametable_2, nametable_3, nametable_4;
	// 32 bytes palette ram
	uint8_t* palette_ram;

	uint8_t read_byte(uint16_t addr);
	void write_byte(uint16_t addr, uint8_t val);
	void update_mirroring();

	Cartridge& cart;
};
#endif // !PPURAM_H