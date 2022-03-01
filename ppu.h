#include <cstdint>
#include "log.h"
#ifndef PPU_H
#define PPU_H

struct PPURAM;
struct ObjectSprite
{
	uint8_t y_coord;
	uint8_t tile_num;	// index into sprite pattern table set in PPU control
	uint8_t attribute;	// bits 0,1: palette, 
						// bit 5: priority, bit 6: flip x, bit 7: flip y
	uint8_t x_coord;
};
struct VideoScreen;
struct PPU
{
	PPU(PPURAM& ppu_ram, VideoScreen& screen);
	
	enum State
	{
		// -1
		PRERENDER,
		// 0-239
		VISIBLE,
		// 240
		POSTRENDER,
		// 241-260
		VBLANK,
	}render_state;

	// $2000 control
	uint8_t nametable_addr;			// 0=$2000, 1=$2400, 2=$2800, 3=$2C00
	bool vram_increment;			// 0=add 1 (across), 1=add 32 (down)
	bool sprite_pattern_table;		// 0=$0000, 1=$1000 (ignored if tall_sprites)
	bool background_pattern_table;	// 0=$0000, 1=$1000
	bool tall_sprites;				// 0=8x8, 1=8x16
	bool generate_NMI;				// generate CPU NMI when entering v blank
	
	// $2001 mask
	bool greyscale;
	bool hide_edge_background;
	bool hide_edge_sprites;
	bool show_background;
	bool show_sprites;
	bool red_tint;
	bool green_tint;
	bool blue_tint;

	// $2002 status
	bool sprite0_hit;		// Set if sprite 0 in OAM's opaque pixel overlaps background pixel
	bool v_blank;			// cleared after reading and at 1st dot of pre-render, set after v blank

	// $2003
	uint8_t oam_addr;
	// $2004
	// OAM read, write access
	// $2005 scroll, 2x write
	// $2006 2x write
	// $2007
	// Data read/write

	 
	uint16_t data_address;
	bool first_write;
	uint8_t data_buffer;
	uint8_t fine_x_scroll;
	uint16_t temp_addr;
	// TEMP ADDRESS for scroll, 15 bits 
	// yyy NN YYYYY XXXXX
	// ||| || ||||| +++++ --coarse X scroll
	// ||| || +++++ --------coarse Y scroll
	// ||| ++--------------nametable select (bit 0 + 1 of $2000)
	// +++ ----------------- fine Y scroll

	PPURAM& ppubus;
	VideoScreen& screen;

	int scanline;	// -1-260 scanlines
	int cycle;		// 0-340 cycles per scanline

	bool send_nmi_output = false;


	// SHIFTERS, UPPER AND LOWER BITS
	uint16_t patterntable_bgrd[2];	// First 8 bits hi/lo current tile, upper bits loaded at every 8 cycle
	uint8_t palette_bgrd[2];		// palette index, fed input from palette latch when left shifted
	bool next_palette_latch[2];		// palette index loaded at every 8 cycle with patterntable

	uint8_t num_sprites_scanline;
	uint8_t scanline_sprite_indices[8];					// Every scanline, at most 8 sprites are choosen from OAM
	uint8_t scanline_sprite_pattern_shifters[8][2];		// Pattern tables are fetched
	uint8_t scanline_sprite_xpos[8];				
	bool scanline_sprite_palette_latch[8][2];			


	// 64 object sprites, or 256 Bytes
	ObjectSprite* OAM;

	// Data for sprites on scanline

	void clock();

	// Register read/write callbacks
	uint8_t RegisterRead(uint16_t addr);
	void RegisterWrite(uint16_t addr, uint8_t data);

};
#endif // !PPU_H