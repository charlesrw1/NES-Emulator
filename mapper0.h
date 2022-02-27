#include "mapper.h"
#include "cartridge.h"
#include "log.h"
struct Mapper0 : public Mapper
{
	Mapper0(Cartridge& cart) : Mapper(cart)
	{
		if (!cart.chr_banks) {
			chr_ram.resize(0x2000);
		}
	}
	uint8_t read_prg(uint16_t addr) override {
		// If cartridge has 2 banks, use full address space, else, mirror top half downwards
		addr = addr - 0x8000;
		if (cart.prg_banks == 1) {
			addr &= 0x3FFF;
		}
		return cart.PRG_ROM[addr];
	}
	void write_prg(uint16_t addr, uint8_t val) override {
		// Mapper 0 doesn't support writing to PRG ROM
		LOG(Error) << "Write attempt on PRG ROM @ " << +addr << std::endl;
	}
	uint8_t read_chr(uint16_t addr) override {
		if (cart.chr_banks) {
			return cart.CHR_ROM[addr];
		}
		else {
			return chr_ram[addr];
		}
	}
	void write_chr(uint16_t addr, uint8_t val) override {
		if (!cart.chr_banks) {
			chr_ram[addr] = val;
		}
		else {
			LOG(Error) << "Write attempt on CHR ROM @ " << +addr << std::endl;
		}
	}

	// only used if program doesn't have any chr banks
	std::vector<uint8_t> chr_ram;
};