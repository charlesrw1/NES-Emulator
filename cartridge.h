#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <cstdint>
#include <vector>
#include <string>
struct Mapper;
struct Cartridge
{
	bool load_from_file(std::string file);

	enum NametableMirroring {
		HORIZONTAL,
		VERTICAL
	}mirroring;
	uint8_t mapper_num;
	
	Mapper* mapper;

	std::vector<uint8_t> PRG_ROM;
	std::vector<uint8_t> CHR_ROM;

	uint8_t prg_banks;	// 16KB 
	uint8_t chr_banks;	// 8KB
};
#endif // !CARTRIDGE_H