#include "cartridge.h"
#include <fstream>
#include "log.h"
#include "mapper0.h"
#include "mapper1.h"
#include "mapper4.h"

bool Cartridge::load_from_file(std::string file, PPURAM& ppu_ram)
{
	std::ifstream rom(file, std::ios::binary);
	LOG(Info) << "Loading rom file: " << file << std::endl;
	if (!rom) {
		LOG(Error) << "Couldn't load rom: " << file << std::endl;
		return false;
	}
	std::vector<uint8_t> header(0x10);
	rom.read((char*)header.data(), 0x10);
	std::string nes_name(&header[0], &header[4]);
	if (nes_name != "NES\x1A") {
		LOG(Error) << "Not an iNES file, missing constant  N E S 1A\n";
		return false;
	}
	
	prg_banks = header[4];
	PRG_ROM.resize(prg_banks * 0x4000);
	LOG(Info) << "PRG ROM size " << std::hex << +prg_banks << " x 16 KB\n";
	rom.read((char*)PRG_ROM.data(), prg_banks * 0x4000);
	
	chr_banks = header[5];
	CHR_ROM.resize(chr_banks * 0x2000);
	LOG(Info) << "CHR ROM size " << std::hex << +chr_banks << " x 8 KB\n";
	rom.read((char*)CHR_ROM.data(), chr_banks * 0x2000);
	if (chr_banks == 0) {
		LOG(Info) << "No CHR ROM, using CHR RAM" << std::endl;
	}

	mirroring = (header[6] & 0x1) ? VERTICAL : HORIZONTAL;
	LOG(Info) << "Mirroring: " << ((mirroring) ? "VERTICAL" : "HORIZONTAL" ) << std::endl;

	mapper_num = (((header[6] & 0xf0)>>4) | (header[7] & 0xf));
	LOG(Info) << "Mapper #: " << std::hex << +mapper_num << std::endl;

	switch (mapper_num)
	{
	case 0: mapper = new Mapper0(*this); break;
	case 1: mapper = new Mapper1(*this, ppu_ram); break;
	case 4: mapper = new Mapper4(*this, ppu_ram); break;
	default:
		LOG(Error) << "Mapper # is unsupported\n";
		return false;
		break;
	}

	return true;
}