#include <cstdint>
#ifndef CPU_H
#define CPU_H
#include "mainram.h"

struct CPU
{
	CPU(MainRAM& mem) : mem(mem)
	{
		cycles = 7;
		ar = xr = yr = 0;
		nf = vf = df = zf = cf = 0;
		inf = 1;
		pc = 0;
		sp = 0xFD;
	}
	
	uint64_t cycles;
	uint64_t total_cycles=0;

	
	// accumulator, x register, y register
	uint8_t ar, xr, yr;

	// program counter
	uint16_t pc;
	// stack pointer
	uint8_t sp;

	// Status register
	// negative, overflow, decimal (not used), interupt disable, zero, carry
	bool nf:1, vf:1, df:1, inf:1, zf:1, cf:1;

	// flag set by opcode function to determine if addressing should
	// increase cycles
	bool extra_cycle_adr : 1;
	bool opcode_extra_cycle : 1;

	MainRAM& mem;

	void step();
	// Non maskable interrupt
	void nmi();
	// Interrupt request, optional
	void irq();
	void reset();

	uint8_t read_byte(uint16_t addr) {

		return mem.read_byte(addr);
	}
	void write_byte(uint16_t addr, uint8_t val) {
		
		mem.write_byte(addr, val);
	}
	uint16_t read_word(uint16_t addr) {
		return (read_byte(addr + 1) << 8) | read_byte(addr);
	}
	void write_word(uint16_t addr, uint16_t val) {
		write_byte(addr, val & 0xFF);
		write_byte(addr + 1, val >> 8);
	}
};
#endif // !CPU_H