#include <cstdint>
struct CPU
{
	// accumulator, x register, y register
	uint8_t ar, xr, yr;

	// program counter
	uint16_t pc;
	// stack pointer
	uint8_t sp;

	// Status register
	// negative, overflow, decimal (not used), interupt disable, zero, carry
	bool nf:1, vf:1, df:1, inf:1, zf:1, cf:1;

	uint8_t* memory;

	void step();
	void process_interrupts();

	uint8_t read_byte(uint16_t addr) {

		return memory[addr];
	}
	void write_byte(uint16_t addr, uint8_t val) {
		
		if (addr == 199) {
			printf("");
		}
		memory[addr] = val;
	}
	uint16_t read_word(uint16_t addr) {
		return (read_byte(addr + 1) << 8) | read_byte(addr);
	}
	void write_word(uint16_t addr, uint16_t val) {
		write_byte(addr, val & 0xFF);
		write_byte(addr + 1, val >> 8);
	}
};