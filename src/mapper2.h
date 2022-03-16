#include "mapper.h"
#include "cartridge.h"
#include <vector>

// UxROM
class Mapper2 : public Mapper
{
public:
	Mapper2(Cartridge& cart) : Mapper(cart) 
	{
		if (!cart.chr_banks) {
			chr_ram.resize(0x2000);
		}
	}
	uint8_t read_prg(uint16_t addr) {
		if (addr <= 0xBFFF) {
			return cart.PRG_ROM.at(prg_16kb_bank + (addr & 0x3FFF));
		}
		return cart.PRG_ROM.at((cart.prg_banks - 1) * 0x4000 + (addr & 0x3FFF));
	}
	void write_prg(uint16_t addr, uint8_t val) {
		prg_16kb_bank = (val & 0x7) * 0x4000;
	}
	uint8_t read_chr(uint16_t addr) {
		if (cart.chr_banks) {
			return cart.CHR_ROM[addr];
		}
		else {
			return chr_ram[addr];
		}
	}
	void write_chr(uint16_t addr, uint8_t val) {
		if (!cart.chr_banks) {
			chr_ram[addr] = val;
		}
		else {
			LOG(Error) << "Write attempt on CHR ROM @ " << +addr << std::endl;
		}
	}
	int prg_16kb_bank = 0;
	std::vector<uint8_t> chr_ram;
};