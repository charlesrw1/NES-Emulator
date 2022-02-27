#include "ppuram.h"

#include "cartridge.h"
#include "mapper.h"

PPURAM::PPURAM(Cartridge& cart) : cart(cart)
{
	nametable_ram = new uint8_t[0x0800];
	palette_ram = new uint8_t[0x20];
	update_mirroring();
}
PPURAM::~PPURAM() {
	delete[] nametable_ram;
	delete[] palette_ram;
}
uint8_t PPURAM::read_byte(uint16_t addr)
{
	if (addr < 0x2000) {
		return cart.mapper->read_chr(addr);
	}
	else if (addr < 0x3EFF) {
		uint16_t index = addr & 0x3FF;
		if (addr < 0x2400) {
			return nametable_ram[nametable_1 + index];
		}
		else if (addr < 0x2800) {
			return nametable_ram[nametable_2 + index];
		}
		else if (addr < 0x2C00) {
			return nametable_ram[nametable_3 + index];
		}
		else {
			return nametable_ram[nametable_4 + index];
		}
	}
	else {
		return palette_ram[addr & 0x1F];
	}
}
void PPURAM::write_byte(uint16_t addr, uint8_t val) 
{
	if (addr < 0x2000) {
		cart.mapper->write_chr(addr, val);
	}
	else if (addr < 0x3F00) {
		uint16_t index = addr & 0x3FF;
		if (addr < 0x2400) {
			nametable_ram[nametable_1 + index] = val;
		}
		else if (addr < 0x2800) {
			nametable_ram[nametable_2 + index] = val;
		}
		else if (addr < 0x2C00) {
			nametable_ram[nametable_3 + index] = val;
		}
		else {
			nametable_ram[nametable_4 + index] = val;
		}
	}
	else {
		palette_ram[addr & 0x1F] = val;
	}
}
void PPURAM::update_mirroring()
{
	switch (cart.mirroring)
	{
	case Cartridge::VERTICAL:
		nametable_1 = 0x0000;
		nametable_2 = 0x0400;
		nametable_3 = 0x0000;
		nametable_4 = 0x0400;
		break;
	case Cartridge::HORIZONTAL:
		nametable_1 = 0x0000;
		nametable_2 = 0x0000;
		nametable_3 = 0x0400;
		nametable_4 = 0x0400;
		break;
	}
}