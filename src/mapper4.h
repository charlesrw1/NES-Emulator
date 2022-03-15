#include "mapper.h"
#include "cartridge.h"
#include <cassert>
struct PPURAM;
class Mapper4 : public Mapper
{
public:
	Mapper4(Cartridge& cart, PPURAM& ppu_ram) 
	: Mapper(cart), ppu_ram(ppu_ram)
	{
		memset(prg_ptrs, 0, sizeof(prg_ptrs));
		memset(chr_ptrs, 0, sizeof(chr_ptrs));
		memset(chr_2kb, 0, sizeof(chr_2kb));

		memset(prg_8kb, 0, sizeof(prg_8kb));
		memset(chr_1kb, 0, sizeof(chr_1kb));


		irq_counter = 0;
		irq_latch = 0;
		prg_ptrs[0] = 0;
		prg_ptrs[1] = 0x2000;
		prg_ptrs[2] = (cart.prg_banks * 2 - 2) * 0x2000;
		prg_ptrs[3] = (cart.prg_banks * 2 - 1) * 0x2000;

		//cart.extended_ram.resize(0x2000);
	}

	uint8_t read_prg(uint16_t addr) override
	{
		uint32_t index = prg_ptrs[(addr >> 13) & 0x3];
		assert(index < cart.prg_banks * 0x4000);
		return cart.PRG_ROM.at(index + (addr & 0x1FFF));
	}
	uint32_t get_prg_rom_address(uint16_t prg_addr) override
	{
		return prg_ptrs[(prg_addr >> 13) & 0x3] + (prg_addr & 0x1FFF);
	}
	void write_prg(uint16_t addr, uint8_t val) override;
	uint8_t read_chr(uint16_t addr) override
	{
		return cart.CHR_ROM.at(chr_ptrs[(addr >> 10) & 0x7] + (addr & 0x03FF));
	}
	void write_chr(uint16_t addr, uint8_t val) override
	{
		LOG(Error) << "WRITE ATTEMPT @ CHR ROM: " << +addr << '\n';
	}
	void scanline() override
	{
		if (irq_counter == 0) {
			irq_counter = irq_latch;
		}
		else {
			--irq_counter;
		}
		if (irq_counter == 0 && irq_enable) {
			generate_IRQ = true;	// The actual IRQ will be handeled higher up
		}
	}

private:
	void update_bank_vals(uint8_t val);
	void update_pointers();

	bool irq_enable = false;
	bool irq_reload = false;
	uint8_t irq_latch;		// Counter gets set to this if reload is true or counter reaches 0
	uint8_t irq_counter;	// Decrements every scanline, when 0, an IRQ is called if enabled and counter reloaded


	uint8_t bank_to_update;	// Taken from first 3 bits on first write to Bank select, Bank data will use this to set one below

	bool chr_2kb_bank_back = false; // 0=$0000-$0FFF, 2x2KB banksl; $1000-$01FFF, 4x1KB bank; (1 is opposite)
	uint8_t chr_2kb[2];	// R0,R1
	uint8_t chr_1kb[4];	// R2-R5

	bool prg_swappable_back = false; // ($C000-$DFFF = back), 0= front is swap, back is fixed to 2nd to last bank; 1= opposite
	uint8_t prg_8kb[2];				// prg_1= $8000-$9FFF or $C000-$DFFF swappable
									// prg_2= $A000-$BFFF swappable
									// R6,R7
	
	uint32_t prg_ptrs[4];	// 8-9,A-B,C-D,E-F
	uint32_t chr_ptrs[8];	// 0-3,4-7,8-B,C-F, 10-13,...

	PPURAM& ppu_ram;
};