#include "mapper4.h"
#include "ppuram.h"
#include "mainram.h"
#include "cartridge.h"
#include "log.h"

void Mapper4::write_prg(uint16_t addr, uint8_t val)
{
	//LOG(Info) << "Write program\n";
	bool even = ~addr & 1;
	if (addr <= 0x9FFF) {
		if (even) {		// Bank Select
			chr_2kb_bank_back = val & 0x80;
			prg_swappable_back = val & 0x40;
			bank_to_update = val & 0x7;
		}
		else {			// Bank Data
			update_bank_vals(val);
			update_pointers();
		}
	}
	else if (addr <= 0xBFFF) {
		if (even) {		// Mirroring
			cart.mirroring = (val & 1) ? Cartridge::HORIZONTAL : Cartridge::VERTICAL;
			ppu_ram.update_mirroring();
		}
		else {			// PRG Ram protect (nothing)
		}
	}
	else if (addr <= 0xDFFF) {
		if (even) {		// IRQ latch
			irq_latch = val;
		}
		else {			// IRQ reload
			irq_counter = 0;	// Reloaded at next scanline
		}
	}
	else if (addr <= 0xFFFF) {
		if (even) {		// IRQ disable
			irq_enable = false;
		}
		else {			// IRQ enable
			irq_enable = true;
		}
	}
}
void Mapper4::update_bank_vals(uint8_t val)
{
	switch (bank_to_update)
	{
	case 0:
	case 1:
		chr_2kb[bank_to_update] = (val & 0xFE); // Ignore bottom bit
		break;
	case 2:
	case 3:
	case 4:
	case 5:
		chr_1kb[bank_to_update - 2] = val;
		break;
	case 6:
	case 7:
		prg_8kb[bank_to_update - 6] = val & 0x3F;	// Ignore top 2 bits
		break;
	default:
		LOG(Error) << "Unknown Bank Update: " << bank_to_update << std::endl;
	}
}
void Mapper4::update_pointers()
{
	//LOG(Info) << "Pointers updates\n";
	if (chr_2kb_bank_back) {
		chr_ptrs[0] = chr_1kb[0] * 0x0400;
		chr_ptrs[1] = chr_1kb[1] * 0x0400;
		chr_ptrs[2] = chr_1kb[2] * 0x0400;
		chr_ptrs[3] = chr_1kb[3] * 0x0400;

		chr_ptrs[4] = chr_2kb[0] * 0x0400;
		chr_ptrs[5] = chr_2kb[0] * 0x0400 + 0x0400;
		chr_ptrs[6] = chr_2kb[1] * 0x0400;
		chr_ptrs[7] = chr_2kb[1] * 0x0400 + 0x0400;
	}
	else {
		chr_ptrs[0] = chr_2kb[0] * 0x0400;
		chr_ptrs[1] = chr_2kb[0] * 0x0400 + 0x0400;
		chr_ptrs[2] = chr_2kb[1] * 0x0400;
		chr_ptrs[3] = chr_2kb[1] * 0x0400 + 0x0400;

		chr_ptrs[4] = chr_1kb[0] * 0x0400;
		chr_ptrs[5] = chr_1kb[1] * 0x0400;
		chr_ptrs[6] = chr_1kb[2] * 0x0400;
		chr_ptrs[7] = chr_1kb[3] * 0x0400;
	}
	if (prg_swappable_back) {
		prg_ptrs[0] = ((cart.prg_banks * 2) - 2) * 0x2000;	// PRG cart banks are stored as 16kb
		prg_ptrs[2] = prg_8kb[0] * 0x2000;
	}
	else {
		prg_ptrs[2] = ((cart.prg_banks * 2) - 2) * 0x2000;
		prg_ptrs[0] = prg_8kb[0] * 0x2000;
	}
	prg_ptrs[1] = prg_8kb[1] * 0x2000;
	prg_ptrs[3] = ((cart.prg_banks * 2) - 1) * 0x2000;	// Fixed to last bank
}
