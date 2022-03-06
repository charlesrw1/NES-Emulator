#include "mapper1.h"
#include "ppuram.h"

void Mapper1::write_control(uint8_t val)
{
	control_reg = val;

	// UPDATE MIRRORING FOR LOWER 2 BITS
	switch (val & 0x3) {
	case 0:
		ppu_ram.cart.mirroring = Cartridge::ONESCREENLOW;
		break;
	case 1:
		ppu_ram.cart.mirroring = Cartridge::ONESCREENHIGH;
		break;
	case 2:
		ppu_ram.cart.mirroring = Cartridge::VERTICAL;
		break;
	case 3:
		ppu_ram.cart.mirroring = Cartridge::HORIZONTAL;
		break;
	}
	ppu_ram.update_mirroring();

	prg_32kb_mode = false;
	switch ((val & 0xc) >> 2)
	{
	case 0:
	case 1:
		// switch 32KB @ $8000
		prg_32kb_mode = true;
		break;
	case 2:
		// fixed first bank @ $8000, switch 16KB bank @ $C000
		prg_fixed_8000 = true;
		break;
	case 3:
		// fixed last bank @ $C000, switch 16KB bank @ $8000
		prg_fixed_8000 = false;
		break;
	}

	chr_8kb_mode = !(val & 0x10);	// 0= 8kb bank, 1= 2, 4 kb banks
}