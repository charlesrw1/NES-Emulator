#include "ppu.h"
#include "ppuram.h"

PPU::PPU(PPURAM& ppu_ram) : ppubus(ppu_ram) 
{
	nametable_addr = 0;
	vram_increment = sprite_pattern_table = background_pattern_table = tall_sprites = generate_NMI = 0;
	greyscale = show_background = show_sprites = red_tint = green_tint = blue_tint = 0;
	hide_edge_background = hide_edge_sprites = 1;
	sprite0_hit = v_blank = 0;

	v_blank = 1;

	oam_addr = 0;
	OAM = new ObjectSprite[64];
	first_write = 0;
	data_buffer = 0;
	fine_x_scroll = 0;
	data_address = 0;
	temp_addr = 0;
}
uint8_t PPU::RegisterRead(uint16_t addr)
{
	LOG(PPU_Info) << "PPU Register Read: " << std::hex << +addr << std::endl;
	uint8_t res = 0;
	switch (addr)
	{
	case 0x2002:
		res |= (v_blank << 7);
		res |= (sprite0_hit << 6);
		first_write = false;
		break;
	case 0x2004:
	{
		uint8_t* oam_uint = reinterpret_cast<uint8_t*>(OAM);
		res = oam_uint[oam_addr];
		break;
	}
	case 0x2007:
		// TEMPORARY, add in PPURAM read functionality,
		// $2007 reads from bus @ data_address, written in $2006
		// 
		res = data_buffer;
		data_buffer = ppubus.read_byte(data_address);

		// Palette ram only takes one cycle, other must read twice from OAMDATA
		if (data_address >= 0x3F00) {
			res = data_buffer;
		}
		data_address += (vram_increment) ? 32 : 1;
		break;
	default:
		LOG(Error) << "Read attempt @ write only register: " << std::hex << +addr << std::endl;
		break;
	}
	return res;
}
void PPU::RegisterWrite(uint16_t addr, uint8_t val)
{
	LOG(PPU_Info) << "PPU Register Write: " << std::hex << +addr << std::endl;
	switch (addr)
	{
	case 0x2000:
		nametable_addr = val & 0x3; // first 2 bits
		vram_increment = val & (1 << 2);
		sprite_pattern_table = val & (1 << 3);
		background_pattern_table = val & (1 << 4);
		tall_sprites = val & (1 << 5);

		generate_NMI = val & (1 << 7);
		break;
	case 0x2001:
		greyscale = val & (1 << 0);
		hide_edge_background = !(val & (1 << 1));
		hide_edge_sprites = !(val & (1 << 2));
		show_background = val & (1 << 3);
		show_sprites = val & (1 << 4);
		red_tint = val & (1 << 5);
		green_tint = val & (1 << 6);
		blue_tint = val & (1 << 7);
		break;
	case 0x2003:
		oam_addr = val;
		break;
	case 0x2004:
	{
		uint8_t* oam_uint = reinterpret_cast<uint8_t*>(OAM);
		oam_uint[oam_addr] = val;
		oam_addr++;
		break;
	}
	case 0x2005:
		if (!first_write) {
			first_write = true;
			// .....FGH -> FGH
			fine_x_scroll = val & 0x7;
			//  ABCDE... -> ....... ...ABCDE
			temp_addr |= (val & 0xF8) >> 3;
		}
		else {
			first_write = false;
			// ABCDEFGH -> FGH..AB CDE.....
			temp_addr |= ((val & 0x7) << 12);
			temp_addr |= ((val & 0xf8) << 2);
		}
		break;
	case 0x2006:
		if (!first_write) {
			first_write = true;
			// ..CDEFGH -> 00CDEFGH ........
			temp_addr = 0;
			temp_addr = (val & 0x3f) << 8;
		}
		else {
			first_write = false;
			// ABCDEFGH ->  ....... ABCDEFGH
			temp_addr |= val;
			data_address = temp_addr;
		}
		break;
	case 0x2007:
		ppubus.write_byte(data_address, val);
		data_address += (vram_increment) ? 32 : 1;
		break;

	default:
		LOG(Error) << "Write attempt @ read only register: " << std::hex << +addr << std::endl;
	}
}