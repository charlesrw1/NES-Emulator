#include "ppu.h"
#include "ppuram.h"
#include "video_screen.h"
#include "mapper.h"
#include "cartridge.h"

#include <cassert>

const sf::Uint32 colors[] = {
			0x666666ff, 0x002a88ff, 0x1412a7ff, 0x3b00a4ff, 0x5c007eff, 0x6e0040ff, 0x6c0600ff, 0x561d00ff,
			0x333500ff, 0x0b4800ff, 0x005200ff, 0x004f08ff, 0x00404dff, 0x000000ff, 0x000000ff, 0x000000ff,
			0xadadadff, 0x155fd9ff, 0x4240ffff, 0x7527feff, 0xa01accff, 0xb71e7bff, 0xb53120ff, 0x994e00ff,
			0x6b6d00ff, 0x388700ff, 0x0c9300ff, 0x008f32ff, 0x007c8dff, 0x000000ff, 0x000000ff, 0x000000ff,
			0xfffeffff, 0x64b0ffff, 0x9290ffff, 0xc676ffff, 0xf36affff, 0xfe6eccff, 0xfe8170ff, 0xea9e22ff,
			0xbcbe00ff, 0x88d800ff, 0x5ce430ff, 0x45e082ff, 0x48cddeff, 0x4f4f4fff, 0x000000ff, 0x000000ff,
			0xfffeffff, 0xc0dfffff, 0xd3d2ffff, 0xe8c8ffff, 0xfbc2ffff, 0xfec4eaff, 0xfeccc5ff, 0xf7d8a5ff,
			0xe4e594ff, 0xcfef96ff, 0xbdf4abff, 0xb3f3ccff, 0xb5ebf2ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff,
};

PPU::PPU(PPURAM& ppu_ram, VideoScreen& screen) : ppubus(ppu_ram), screen(screen)
{
	nametable_addr = 0;
	vram_increment = sprite_pattern_table = background_pattern_table = tall_sprites = generate_NMI = 0;
	greyscale = show_background = show_sprites = red_tint = green_tint = blue_tint = 0;
	show_edge_background = show_edge_sprites = 1;
	sprite0_hit = v_blank = 0;

	v_blank = 0;
	send_nmi_output = 0;

	oam_addr = 0;
	OAM = new ObjectSprite[64];
	first_write = 0;
	data_buffer = 0;
	fine_x_scroll = 0;
	data_address = 0;
	temp_addr = 0;

	memset(OAM, 0xFF, sizeof(ObjectSprite) * 64);
	memset(scanline_sprite_indices, 0, sizeof(scanline_sprite_indices));
	num_sprites_scanline = 0;

	cycle = 0;
	scanline = 0;
	render_state = VISIBLE;
}
uint8_t PPU::RegisterRead(uint16_t addr)
{
	//LOG(PPU_Info) << "PPU Register Read: " << std::hex << +addr << std::dec << std::endl;
	uint8_t res = 0;
	switch (addr)
	{
	case 0x2002:
		res |= (v_blank << 7);
		res |= (sprite0_hit << 6);
		v_blank = false;
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
		//res = data_buffer;
		//data_buffer = ppubus.read_byte(data_address);

		res = ppubus.read_byte(data_address);
		data_address += (vram_increment) ? 32 : 1;

		// Palette ram only takes one cycle, other must read twice from OAMDATA
		if (data_address < 0x3F00) {
			uint8_t temp = res;
			res = data_buffer;
			data_buffer = temp;
		}
		break;
	default:
		LOG(Error) << "Read attempt @ write only register: " << std::hex << +addr << std::endl;
		break;
	}
	return res;
}
void PPU::RegisterWrite(uint16_t addr, uint8_t val)
{
	//LOG(PPU_Info) << "PPU Register Write: " << std::hex << +addr << ", " <<  std::dec << std::endl;
	switch (addr)
	{
	case 0x2000:
		temp_addr &= 0xF3FF;
		temp_addr |= (val & 0x3) << 10; // first 2 bits

		vram_increment = val & (1 << 2);
		sprite_pattern_table = val & (1 << 3);
		background_pattern_table = val & (1 << 4);
		tall_sprites = val & (1 << 5);

		generate_NMI = val & (1 << 7);

		break;
	case 0x2001:
		greyscale = val & (1 << 0);
		show_edge_background = (val & (1 << 1));
		show_edge_sprites = (val & (1 << 2));
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
			temp_addr &= 0xFFE0;
			temp_addr |= (val >> 3) & 0x1f;
		}
		else {
			first_write = false;
			// ABCDEFGH -> FGH..AB CDE.....
			temp_addr &= 0x8C1F;
			temp_addr |= (val & 0xf8) << 2;
			temp_addr |= (val & 0x7) << 12;
		}
		break;
	case 0x2006:
		if (!first_write) {
			first_write = true;
			// ..CDEFGH -> 00CDEFGH ........
			temp_addr &= 0x00FF;
			temp_addr = (val & 0x3f) << 8;
		}
		else {
			first_write = false;
			// ABCDEFGH ->  ....... ABCDEFGH
			temp_addr &= 0xFF00;
			temp_addr |= val;
			data_address = temp_addr;
		}

		break;
	case 0x2007:
	{
	ppubus.write_byte(data_address, val);
	data_address += (vram_increment) ? 32 : 1;
	break;
	}
	default:
		LOG(Error) << "Write attempt @ read only register: " << std::hex << +addr << std::endl;
	}

}
// General Guide to PPU Rendering, not cycle accurate
// 
// 1 scanline = 340 cycles always
// 
// Scanline -1, pre-render
//		Cycle 1, clear sprite0 and vblank flags
//		Cycle 257, move Temp ptr coarse x into data ptr (reset data ptr to the left side of screen) 
//		Cycle 280-304, move Temp ptr [coarse y]+ [nametable y] into data ptr (temp ptr stores the scrolling offsets of the top left)
//		Cycle 340, load first 2 tiles into shifters + increment horizontal twice
//		
//
// Scanline 0-239, visible (AKA y coord)
//		Cycles 1-256, generate pixel (AKA x coord)
//			Move shifters
//			Every 8 tiles: v= data ptr
//				Fetch tile,  tile address      = 0x2000 | (v & 0x0FFF)
//					-> get hi/lo order bytes from pattern table (4 bits per pixel)
//					-> lo = ((tile * 16) + fine y) | (background_table << 12)
//					-> hi = lo + 8 bytes ( or 1 << 4)
//				
//				Fetch attribute, attribute address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)
//					-> Move palette index into latches
//				Increment horizontal data ptr
// 
//			Pixel = cycles-1, scanline, get_palette((hishifter<<1 | loshifter)[BOTH PATTERN AND PALETTE INDEX SHIFTERS]);
//			
//		Cycle 256, increment vertical Y data ptr
//		Cycle 257, move Temp ptr coarse x into data ptr (reset data ptr to the left side of screen) 
//		Cycle 340 (last cycle), fill the 8 sprite buffer for the next scanline, load first 2 tiles into shifters + increment horizontal twice
//
// Scanline 240, post-render
//		(Do nothing)
//
//  Scanline 241-260, vblank
//		Cycle 1, set vblank flag, generate NMI interrupt if register flag is set (called by Emulator class)
//		


inline void update_shifters(PPU& p)
{
	p.patterntable_bgrd[0] <<= 1;
	p.patterntable_bgrd[1] <<= 1;
	p.palette_bgrd[0] <<= 1;
	p.palette_bgrd[1] <<= 1;
	p.palette_bgrd[0] |= (uint8_t)p.next_palette_latch[0];
	p.palette_bgrd[1] |= (uint8_t)p.next_palette_latch[1];
	for (int i = 0; i < p.num_sprites_scanline; i++) {
		if (p.scanline_sprite_xpos[i] == 0) {
			p.scanline_sprite_pattern_shifters[i][0] <<= 1;
			p.scanline_sprite_pattern_shifters[i][1] <<= 1;
		}
		if (p.scanline_sprite_xpos[i] > 0) {
			--p.scanline_sprite_xpos[i];
		}
		// sprites are considered active if xpos is 0
	}
}


// Data and temp address, again for reference
// yyy NN YYYYY XXXXX
// ||| || ||||| +++++ --coarse X scroll
// ||| || +++++ --------coarse Y scroll
// ||| ++--------------nametable select
// +++ ---------------- - fine Y scroll

inline void copy_horizontal_bits(PPU& p)
{
	if (!p.show_background && !p.show_sprites) return;
	
	// v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF
	p.data_address &= ~(0x41F);
	p.data_address |= p.temp_addr & (0x41F);
}
inline void copy_vertical_bits(PPU& p)
{
	if (!p.show_background && !p.show_sprites) return;
	
	// v: GHIA.BC DEF..... <- t: GHIA.BC DEF.....
	p.data_address &= ~(0x7BE0);
	p.data_address |= p.temp_addr & 0x7BE0;
}
// Thanks NESDEV wiki
inline void increment_horizontal(PPU& p)
{
	if (!p.show_background && !p.show_sprites) return;

	if ((p.data_address & 0x001F) == 31) {	// coarse X == 31
		p.data_address &= ~0x001F;			// coarse X = 0
		p.data_address ^= 0x0400;			// toggle nametable x
	}
	else {
		p.data_address += 1;				// increment coarse X
	}
}
inline void increment_vertical(PPU& p)
{
	if (!p.show_background && !p.show_sprites) return;


	if ((p.data_address & 0x7000) != 0x7000) {	// fine y != 7
		p.data_address += 0x1000;				// increment fine y
	}
	else {
		p.data_address &= ~(0x7000);			// fine y = 0

 		int y = (p.data_address & 0x03E0) >> 5;	// y = coarse y
		if (y == 29) {
			y = 0;
			p.data_address ^= 0x0800;			// toggle nametable y
		}
		else if (y == 31) {
			y = 0;
		}
		else {
			y += 1;
		}
		p.data_address = (p.data_address & ~(0x03E0)) | (y << 5);
	}
}
inline uint8_t read_nametable_tile(PPU& p)
{
	int coarse_x = p.data_address & 0x001F;
	int coarse_y = (p.data_address & 0x03E0) >> 5;
	int nametable = (p.data_address & 0xC00) >> 10;

	return p.ppubus.read_byte(0x2000 | (p.data_address & 0x0FFF));
}
// Attribute address:
// NN 1111 YYY XXX
// || |||| ||| +++ --high 3 bits of coarse X(x / 4)
// || |||| +++ ------high 3 bits of coarse Y(y / 4)
// || ++++----------attribute offset(960 bytes)
// ++--------------- nametable select
inline uint8_t read_attribute_byte(PPU& p)
{
	return p.ppubus.read_byte(0x23C0 | (p.data_address & 0x0C00)
		| ((p.data_address >> 4) & 0x38) | ((p.data_address >> 2) & 0x07));
}
// Pattern table address:
// DCBA98 76543210
// ---------------
// 0HRRRR CCCCPTTT
// |||||| |||||+++-T : Fine Y offset, the row number within a tile
// |||||| ||||+----P : Bit plane(0: "lower"; 1: "upper")
// |||||| ++++-----C : Tile column
// ||++++----------R : Tile row
// |+--------------H : Half of sprite table(0: "left"; 1: "right")
// +---------------0 : Pattern table is at $0000 - $1FFF

inline uint8_t fetch_pattern_table(PPU& p, uint8_t location, bool upper_bit_plane, bool right_table)
{
	uint16_t address = 0;
	address |= upper_bit_plane << 3;
	address |= location << 4;
	address |= right_table << 12;
	address |= (p.data_address >> 12) & 0x7;	// add fine y offset
	return p.ppubus.read_byte(address);
}
// OAM Sprite Attribute: 
// 76543210
// ||||||||
// ||||||++-Palette(4 to 7) of sprite
// |||+++---Unimplemented
// ||+------Priority(0: in front of background; 1: behind background)
// |+-------Flip sprite horizontally
// +--------Flip sprite vertically

inline uint8_t fetch_sprite_pattern_table(const PPU& p, const ObjectSprite& sprite, bool upper_bit_plane)
{
	uint16_t address = 0;
	uint8_t y_offset = p.scanline - sprite.y_coord;
	if (!p.tall_sprites) {
		address |= upper_bit_plane << 3;
		address |= sprite.tile_num << 4;
		address |= p.sprite_pattern_table << 12;

		if (sprite.attribute & (1 << 7)) {	// Flip vertically
			y_offset = 7 - y_offset;
		}

		address |= (y_offset);
	}
	else {
		bool flip_y = sprite.attribute & (1 << 7);
		address |= upper_bit_plane << 3;
		address |= (sprite.tile_num & 1) << 12;		// Tall sprites select table with low bit
		address |= ((sprite.tile_num & 0xFE) + ((y_offset > 7 && !flip_y)||(y_offset <= 7 && flip_y))) << 4;

		if (flip_y) {	// Flip vertically
			y_offset = 15 - y_offset;
		}
		address |= (y_offset%8);
	}

	uint8_t pattern = p.ppubus.read_byte(address);
	if (sprite.attribute & (1 << 6)) {	// Flip horizontally
		uint8_t b = pattern;
		b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
		b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
		b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
		pattern = b;
	}

	return pattern;
}
// Attribute data:
// 7654 3210
// |||| ||++-Color bits 3-2 for top left quadrant of this byte
// |||| ++---Color bits 3-2 for top right quadrant of this byte
// ||++------Color bits 3-2 for bottom left quadrant of this byte
// ++--------Color bits 3-2 for bottom right quadrant of this byte
inline uint8_t get_tile_palette_index(PPU& p, uint8_t attribute) 
{
	uint8_t coarse_x = p.data_address & 0x001F;
	uint8_t coarse_y = (p.data_address & 0x03E0) >> 4;
	if (coarse_y & 4) {
		attribute >>= 4;
	}
	if (coarse_x & 2) {
		attribute >>= 2;
	}
	return attribute;
}
inline void fetch_and_append_tile_shifters(PPU& p)
{
	uint8_t nametable_tile = read_nametable_tile(p);
	if (nametable_tile != '$' && nametable_tile != 0xcd) {
		printf("");
	}
	uint8_t tile_attribute = read_attribute_byte(p);
	uint8_t pattern_lower = fetch_pattern_table(p, nametable_tile, 0, p.background_pattern_table);
	uint8_t pattern_upper = fetch_pattern_table(p, nametable_tile, 1, p.background_pattern_table);
	uint8_t palette = get_tile_palette_index(p, tile_attribute);

	// these should have been left shifted 8 times already so oring is fine
	p.patterntable_bgrd[0] |= pattern_upper;
	p.patterntable_bgrd[1] |= pattern_lower;

	p.next_palette_latch[0] = 0;
	p.next_palette_latch[1] = 0;

	p.next_palette_latch[0] |= palette & 2;
	p.next_palette_latch[1] |= palette & 1;

	increment_horizontal(p);
}
void PPU::clock()
{

	switch (render_state)
	{
	case PRERENDER:
		if (cycle == 1) {
			sprite0_hit = 0;
			v_blank = 0;
		}
		if (cycle == 257) {
			copy_horizontal_bits(*this);
		}
		if (cycle >= 280 && cycle <= 304 && show_background) {
			copy_vertical_bits(*this);
		}
		// load shifters for next scnaline
		if (cycle >= 340) {
			patterntable_bgrd[0] = 0;
			patterntable_bgrd[1] = 0;
			next_palette_latch[0] = 0;
			next_palette_latch[1] = 0;
			palette_bgrd[0] = 0;
			palette_bgrd[1] = 0;

			uint8_t nametable_tile = read_nametable_tile(*this);
			uint8_t	tile_attribute = read_attribute_byte(*this);
			uint8_t pattern_lower = fetch_pattern_table(*this, nametable_tile, 0, background_pattern_table);
			uint8_t pattern_upper = fetch_pattern_table(*this, nametable_tile, 1, background_pattern_table);
			uint8_t palette = get_tile_palette_index(*this, tile_attribute);

			patterntable_bgrd[0] |= pattern_upper << 8;
			patterntable_bgrd[1] |= pattern_lower << 8;
			// Fill entire row with 1 (0xFF) if bit is set
			if (palette & 2) {
				palette_bgrd[0] = 0xFF;
			}
			if (palette & 1) {
				palette_bgrd[1] = 0xFF;
			}
			increment_horizontal(*this);

			fetch_and_append_tile_shifters(*this);

			num_sprites_scanline = 0;	// Fix for annoying sprite wraparounds from scanline 239 onto scanline 0

			cycle = -1;
			++scanline;
			render_state = VISIBLE;
		}
		break;
	case VISIBLE:
		if ((show_background || show_sprites) && cycle >= 1 && cycle <= 256) {
			// Every 8 cycles, fetch new tile
			if ((cycle - 1) % 8 == 0 && cycle > 1) {
				fetch_and_append_tile_shifters(*this);
			}
			uint8_t bgrd_color = 0;
			if (show_background && (show_edge_background || cycle >= 8)) {
				// Select bit from shifter, and shift over hi bit 1
				bgrd_color |= ((patterntable_bgrd[0] >> (15 - fine_x_scroll)) & 1) << 1;
				bgrd_color |= ((patterntable_bgrd[1] >> (15 - fine_x_scroll)) & 1);

				bgrd_color |= ((palette_bgrd[0] >> (7 - fine_x_scroll)) & 1) << 3;
				bgrd_color |= ((palette_bgrd[1] >> (7 - fine_x_scroll)) & 1) << 2;
			}

			uint8_t sprite_color = 0;
			bool is_sprite_0 = false;
			bool sprite_behind_bgrd = false;
			if (show_sprites && (show_edge_sprites || cycle >= 8)) {
				for (int i = 0; i < num_sprites_scanline; i++) {
					if (scanline_sprite_xpos[i] == 0) {
						sprite_color |= ((scanline_sprite_pattern_shifters[i][0] >> 7) & 1) << 1;
						sprite_color |= (scanline_sprite_pattern_shifters[i][1] >> 7) & 1;

						// if its transparent, keep looking for color
						if (sprite_color == 0) continue;

						sprite_color |= (scanline_sprite_palette_latch[i][0]) << 3;
						sprite_color |= (scanline_sprite_palette_latch[i][1]) << 2;

						sprite_color |= (1 << 4);	// use sprite palette

						// if the sprite is sprite zero ...
						if (scanline_sprite_indices[i] == 0) {
							is_sprite_0 = true;
						}
						sprite_behind_bgrd = OAM[scanline_sprite_indices[i]].attribute & (1 << 5);

						// break on first color found
						break;
					}
				}
			}
			uint8_t color_index;
			if (((bgrd_color&0x3)==0) && ((sprite_color&0x3)==0)) {
				color_index = ppubus.read_byte(0x3F00);
			}
			else if ((bgrd_color & 0x3) && ((sprite_color & 0x3) == 0)) {
				color_index = ppubus.read_byte(0x3F00 + bgrd_color);
			}
			else if (((bgrd_color & 0x3) == 0) && (sprite_color & 0x3)) {
				color_index = ppubus.read_byte(0x3F00 + sprite_color);
			}
			// Both hit
			else {
				if (sprite_behind_bgrd) {
					color_index = ppubus.read_byte(0x3F00 + bgrd_color);
				}
				else {
					color_index = ppubus.read_byte(0x3F00 + sprite_color);
				}
				if (is_sprite_0) {
					sprite0_hit = true;
				}
			}

			sf::Uint32 pixel_color = colors[color_index];
			screen.set_pixel(cycle - 1, scanline, sf::Color(pixel_color));


			update_shifters(*this);
			if (cycle == 256) {
				increment_vertical(*this);
			}

		}
		if (cycle == 257) {
			copy_horizontal_bits(*this);

		}
		// load sprites for next scanline, real hardware does this during regular rendering
		// simpler to just do it at end of scanline
		if (cycle >= 340) {			
			uint8_t height = (tall_sprites) ? 16 : 8;
			num_sprites_scanline = 0;
			for (int i = 0; i < 64 && num_sprites_scanline < 8; i++) {
				if (OAM[i].y_coord > scanline) continue;
				if (scanline - OAM[i].y_coord >= height) continue;
				scanline_sprite_indices[num_sprites_scanline] = i;

				uint8_t pattern_hi = fetch_sprite_pattern_table(*this, OAM[i], 1);
				uint8_t pattern_lo = fetch_sprite_pattern_table(*this, OAM[i], 0);

				scanline_sprite_pattern_shifters[num_sprites_scanline][0] = pattern_hi;
				scanline_sprite_pattern_shifters[num_sprites_scanline][1] = pattern_lo;
				scanline_sprite_palette_latch[num_sprites_scanline][0] = OAM[i].attribute & 0x2;
				scanline_sprite_palette_latch[num_sprites_scanline][1] = OAM[i].attribute & 0x1;
				scanline_sprite_xpos[num_sprites_scanline] = OAM[i].x_coord;

				num_sprites_scanline++;
			}
			ppubus.cart.mapper->scanline(); // So MMC3 mapper (4), can decrement/call IRQ


		// Load next 2 tiles into shifters
			patterntable_bgrd[0] = 0;
			patterntable_bgrd[1] = 0;
			next_palette_latch[0] = 0;
			next_palette_latch[1] = 0;
			palette_bgrd[0] = 0;
			palette_bgrd[1] = 0;

			uint8_t nametable_tile = read_nametable_tile(*this);
			uint8_t	tile_attribute = read_attribute_byte(*this);
			uint8_t pattern_lower = fetch_pattern_table(*this, nametable_tile, 0, background_pattern_table);
			uint8_t pattern_upper = fetch_pattern_table(*this, nametable_tile, 1, background_pattern_table);
			uint8_t palette = get_tile_palette_index(*this, tile_attribute);
			patterntable_bgrd[0] |= pattern_upper << 8;
			patterntable_bgrd[1] |= pattern_lower << 8;
			// Fill entire row with 1 (0xFF) if bit is set
			if (palette & 2) {
				palette_bgrd[0] = 0xFF;
			}
			if (palette & 1) {
				palette_bgrd[1] = 0xFF;
			}
			increment_horizontal(*this);

			fetch_and_append_tile_shifters(*this);


			++scanline;
			cycle = -1;
		}

		if (scanline > 239) {
			render_state = POSTRENDER;
			screen.render();
		}
		break;
	case POSTRENDER:
		// scanline does nothing here
		if (cycle >= 340) {
			++scanline;
			cycle = -1;
			render_state = VBLANK;
		}
		break;
	case VBLANK:
		// YOU FORGOT THE SCANLINE AHHHHHHHHHHHHHHHHHHHHHHHHHHHH
		// REEEEEEEEEEEEEEEEEEEEEE
		if (cycle == 1 && scanline == 241) {
			send_nmi_output = true;
			v_blank = true;
		}
		if (cycle >= 340) {
			cycle = -1;
			++scanline;
		}
		if (scanline > 261) {
			scanline = -1;
			render_state = PRERENDER;
		}
		break;
	}
	++cycle;
}