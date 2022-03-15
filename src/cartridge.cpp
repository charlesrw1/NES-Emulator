#include "cartridge.h"
#include <fstream>
#include "log.h"
#include "mapper0.h"
#include "mapper1.h"
#include "mapper4.h"

bool Cartridge::load_from_file(std::string file, PPURAM& ppu_ram)
{
	std::ifstream rom("../../../roms/" + file, std::ios::binary);
	LOG(Info) << "Loading rom file: " << file << std::endl;
	if (!rom) {
		LOG(Error) << "Couldn't load rom: " << file << std::endl;
		return false;
	}
	int pos = file.rfind('.');
	rom_name = file.substr(0, pos);

	std::vector<uint8_t> header(0x10);
	rom.read((char*)header.data(), 0x10);
	std::string nes_name(&header[0], &header[4]);
	if (nes_name != "NES\x1A") {
		LOG(Error) << "Not an iNES file, missing constant  N E S 1A\n";
		return false;
	}
	bool ines2 = false;
	// INES2 Header check
	if ((header[7] & 0x0C) == 0x08) {
		LOG(Info) << "Header is INES 2.0\n";
		ines2 = true;
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

	battery_ram = (header[6] & 0x2);
	if (battery_ram) {
		LOG(Info) << "Using battery-backed PRG RAM" << std::endl;
		extended_ram.resize(0x2000);
		std::ifstream save_file("../../../saves/"+rom_name + ".sav");
		if (!save_file) {
			LOG(Info) << ".sav file not found, creating new save file" << std::endl;
		}
		else {
			LOG(Info) << ".sav file found" << std::endl;
			save_file.read((char*)extended_ram.data(), 0x2000);
		}
	}

	if (ines2) {
		uint8_t prg_ram = header[10] & 0xF;
		if (prg_ram > 0) {
			LOG(Info) << "Using PRG-RAM (volatile)\n";
			extended_ram.resize(0x2000);
		}
	}


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