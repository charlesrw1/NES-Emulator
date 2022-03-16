#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <cstdint>
#include <vector>
#include <string>
#include "log.h"
struct Mapper;
struct PPURAM;
struct Cartridge
{
	// Some mappers need access to ppu to change nametable mirroring
	bool load_from_file(std::string file, PPURAM& ppu_ram);
	
	uint8_t read_extended_ram(uint16_t addr) {
		if (extended_ram.size() == 0) {
			LOG(Debug) << "Read attempt @ save RAM, no save RAM exists" << std::endl;
			return 0;
		}
		return extended_ram.at(addr - 0x6000);
	}
	void write_extended_ram(uint16_t addr, uint8_t val) {
		if (extended_ram.size() == 0) {
			LOG(Warning) << "Write attempt @ save RAM, no save RAM exists" << std::endl;
			return;
		}
		extended_ram.at(addr - 0x6000) = val;
	}

	enum NametableMirroring {
		HORIZONTAL,
		VERTICAL,
		ONESCREENLOW,
		ONESCREENHIGH
	}mirroring;
	uint8_t mapper_num;
	
	Mapper* mapper;

	std::vector<uint8_t> PRG_ROM;
	std::vector<uint8_t> CHR_ROM;

	bool battery_ram = false;
	// $6000-$7FFF, optional
	std::vector<uint8_t> extended_ram;

	std::string rom_name;

	uint8_t prg_banks;	// 16KB 
	uint8_t chr_banks;	// 8KB
};
#endif // !CARTRIDGE_H