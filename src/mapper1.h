#include "mapper.h"
#include "cartridge.h"
#include "log.h"

// Shift register defaults to having bit 1 set to determine when its full
constexpr uint8_t shift_register_reset = 0b10000;
// MMC1
struct PPURAM;
class Mapper1 : public Mapper
{
public:
	Mapper1(Cartridge& cart, PPURAM& ppu_ram) : Mapper(cart), ppu_ram(ppu_ram)
	{
		shift_register = shift_register_reset;
		prg_bank = 0;
		prg_32kb_mode = 0;
		prg_fixed_8000 = 0;
		chr_bank0 = chr_bank1 = 0;
		chr_8kb_mode = 0;
		control_reg = 0;
		uses_chr_ram = false;

		if (cart.chr_banks == 0) {
			CHR_RAM.resize(0x2000);
			uses_chr_ram = true;
		}
		cart.extended_ram.resize(0x2000);

	}
	

	uint8_t read_chr(uint16_t addr) override
	{
		if (uses_chr_ram) {
			return CHR_RAM[addr];
		}
		
		if (chr_8kb_mode) {
			return cart.CHR_ROM.at((chr_bank0 >> 1) * 0x2000 + addr);
		}
		else {
			if (addr < 0x1000) {
				return cart.CHR_ROM.at(chr_bank0 * 0x1000 + addr);
			}
			else {
				return cart.CHR_ROM.at(chr_bank1 * 0x1000 + (addr &0xfff));
			}
		}
	}
	void write_chr(uint16_t addr, uint8_t val) override
	{
		if (uses_chr_ram) {
			CHR_RAM[addr] = val;
		}
		else {
			LOG(Error) << "WRITE ATTEMPT ON CHR ROM @ " << std::hex << +addr << std::endl;
		}
	}
	uint8_t read_prg(uint16_t addr) override
	{
		if (prg_32kb_mode) {
			return cart.PRG_ROM.at((prg_bank >> 1) * 0x8000 + (addr - 0x8000));
		}
		else {
			if (prg_fixed_8000) {
				if (addr < 0xC000) {
					// first bank
					return cart.PRG_ROM.at(addr & 0x3fff);
				}
				else {
					return cart.PRG_ROM.at((prg_bank * 0x4000) + (addr & 0x3fff));
				}
			}
			else {
				if (addr < 0xC000) {
					return cart.PRG_ROM.at((prg_bank * 0x4000) + (addr & 0x3fff));
				}
				else {
					// last bank
					return cart.PRG_ROM.at(((cart.prg_banks-1) * 0x4000) + (addr & 0x3fff));
				}
			}
		}
	}
	void write_prg(uint16_t addr, uint8_t val) override
	{
		// if bit 7 is set, 
		if (val & 0x80) {
			shift_register = shift_register_reset;
			// Write to control with bits 2 and 3 set, setting PRG ROM to 
			write_control(control_reg | 0xc);
		}
		else {
			bool register_full = (shift_register & 1);	// The 1 bit in the reset has now been shifted out, 
			shift_register >>= 1;
			shift_register |= (val & 1) << 4;
			if (register_full && addr >= 0x8000) {
				if (addr <= 0x9FFF)
					write_control(shift_register);
				else if (addr <= 0xBFFF)	// CHR bank 0, 
					chr_bank0 = shift_register;
				else if (addr <= 0xDFFF)	// CHR bank 1, ignored in 8kb chr mode
					chr_bank1 = shift_register;
				else if (addr <= 0xFFFF) {	// PRG bank
					prg_bank = shift_register & 0xf;	// MMC1B uses last bit, MMC1A uses bit 3 to bypass fixed bank logic, NOT IMPLEMENTED! 
				}
				shift_register = shift_register_reset;
			}
		}
	}
protected:
	void write_control(uint8_t val);
// Internal registers
	uint8_t prg_bank;
	uint8_t chr_bank0;
	uint8_t chr_bank1;

	uint8_t control_reg;

	bool chr_8kb_mode;
	bool prg_32kb_mode;
	bool prg_fixed_8000;	// 0= fixed last bank @ $C000, 1= fixed bank @ $8000, unused if 32kb mode

	uint8_t shift_register;

	PPURAM& ppu_ram;		// Reference to update nametable mirroring

	bool uses_chr_ram;
	std::vector<uint8_t> CHR_RAM;

};