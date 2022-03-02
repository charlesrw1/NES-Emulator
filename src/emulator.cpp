#include "emulator.h"
void Emulator::load_cartridge(std::string file)
{
	cart.load_from_file(file);
	ppu_ram.update_mirroring();
}