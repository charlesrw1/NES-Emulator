#include <iostream>
#include <cstring>
#include "cpu.h"
#define LOG printf

uint8_t next_byte(CPU& c)
{
	return c.read_byte(c.pc++);
}
uint16_t next_word(CPU& c)
{
	c.pc += 2;
	return c.read_word(c.pc - 2);
}
void push_byte(CPU& c, uint8_t val)
{
	c.sp -= 1;
	c.write_byte(c.sp, val);
}
void push_word(CPU& c, uint16_t val)
{
	c.sp -= 2;
	c.write_word(c.sp, val);
}
uint16_t pop_word(CPU& c)
{
	c.sp += 2;
	return c.read_word(c.sp - 2);
}
uint8_t pop_byte(CPU& c)
{
	c.sp += 1;
	return c.read_word(c.sp - 1);
}
void execute_opcode(CPU& c, uint8_t opcode);
void CPU::step()
{
	execute_opcode(*this, next_byte(*this));
}
void jsr(CPU& c)
{
	push_word(c,c.pc+1);
	c.pc = next_word(c);
}
uint16_t get_relative(CPU& c, uint8_t rel)
{
	return c.pc + (int8_t)rel;
}
void branch(CPU& c, bool cond)
{
	uint16_t addr = get_relative(c, next_byte(c));
	if (cond) {
		c.pc = addr;
	}
}
#define GET_BIT(n, val) (((val) >> (n)) & 1)
// BIT bits 7 and 6 are transfered to n and v after & with a and val
void test_bit(CPU& c, uint8_t val)
{
	uint8_t res = c.ar & val;
	c.nf = GET_BIT(7, val);
	c.vf = GET_BIT(6, val);
	c.zf = res == 0;
}
void lda(CPU& c, uint8_t val)
{
	c.ar = val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;
}
void ldx(CPU& c, uint8_t val)
{
	c.xr = val;
	c.zf = c.xr == 0;
	c.nf = (c.xr >> 7) & 1;
}
void xy_reg_crement(CPU& c, uint8_t& z, int8_t dec_inc)
{
	z = z + dec_inc;
	c.zf = z == 0;
	c.nf = (z >> 7) & 1;
}
bool carry(uint8_t v1, uint8_t v2, bool cy, int bit)
{
	int32_t res = v1 + v2 + cy;
	int32_t carry = res ^ v1 ^ v2;
	return (carry) & (1 << bit);
}
// Add byte
uint8_t addb(CPU& c, uint8_t v1, uint8_t v2, bool cy)
{
	uint8_t res = v1 + v2 + cy;
	c.nf = (res >> 7) &1;
	c.zf = res == 0;
	c.cf = carry(v1, v2, cy, 8);
	// if carry out doesn't equal carry on last bit, overflow occured
	c.vf = carry(v1, v2, cy, 7) != carry(v1, v2, cy, 8);

	return res;
}
// Subtract bytes
uint8_t subb(CPU& c, uint8_t v1, uint8_t v2, bool cy)
{
	// twos complement, A-B = A+~B+1
	// cy will be 1 after !cy unless subtracting it too
	uint8_t res = addb(c, v1, ~v2, !cy);
	printf("Substracted: %hhi - %hhi = %hhi\n", v1, v2, res);
	//c.cf = !c.cf;

	return res;
}
void cmp(CPU& c, uint8_t val1, uint8_t val2)
{
	// Possible bug? Refrence says cmp doesn't change overflow flag,
	// so ill just store and restore if subb modifies it
	bool v_state = c.vf;
	subb(c, val1, val2, 0);
	c.vf = v_state;
}
void push_status(CPU& c)
{
	uint8_t res=0;
	res |= (c.cf << 0);
	res |= (c.zf << 1);
	res |= (c.inf << 2);
	res |= (c.df << 3);
	res |= (1 << 4);
	res |= (1 << 5);
	res |= (c.vf << 6);
	res |= (c.nf << 7);
	push_byte(c, res);
}
void pull_status(CPU& c)
{
	uint8_t st = pop_byte(c);
	c.cf = (st >> 0) & 1;
	c.zf = (st >> 1) & 1;
	c.inf = (st >> 2) & 1;
	c.df = (st >> 3) & 1;
	c.vf = (st >> 6) & 1;
	c.nf = (st >> 7) & 1;
}
void log_and(CPU& c, uint8_t val)
{
	c.ar = c.ar & val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;
}
void pla(CPU& c)
{
	c.ar = pop_byte(c);
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;
}
void log_or(CPU& c, uint8_t val)
{
	c.ar = c.ar | val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;
}
void log_xor(CPU& c, uint8_t val)
{
	c.ar = c.ar ^ val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;
}
void ldy(CPU& c, uint8_t val)
{
	c.yr = val;
	c.zf = c.yr == 0;
	c.nf = (c.yr >> 7) & 1;
}
uint8_t absolute(CPU& c, uint16_t addr)
{
	return c.read_byte(addr);
}
void transfer(CPU& c, uint8_t& dest, uint8_t val, bool set_flags = true)
{
	dest = val;
	if (set_flags) {
		c.zf = val == 0;
		c.nf = val & 0x80;
	}
}
void execute_opcode(CPU& c, uint8_t opcode)
{
	LOG("PC: %x, Opcode: %x\n", c.pc-1, opcode);

	switch (opcode)
	{
	case 0x4C: c.pc = next_word(c); break; // JMP abs

	case 0xA2: ldx(c, next_byte(c)); break;		// LDX #
	case 0xA9: lda(c, next_byte(c)); break;		// LDA #

	case 0xF0: branch(c, c.zf); break; // BEQ rel
	case 0xD0: branch(c, !c.zf); break;	// BNE rel
	case 0x85: c.write_byte(next_byte(c), c.ar); break; // STA zpg
	case 0x24: test_bit(c, c.read_byte(next_byte(c))); break; // BIT zpg
	case 0x70: branch(c, c.vf); break; // BVS rel
	case 0x50: branch(c, !c.vf); break; // BVC rel
	case 0x10: branch(c, !c.nf); break; // BPL rel
	case 0x60: c.pc = pop_word(c);
	
	case 0x86: c.write_byte(next_byte(c), c.xr); break; // STX zpg
	
	case 0x20: jsr(c); break;		// JSR abs

	case 0x38: c.cf = 1; break;		// SEC
	case 0x18: c.cf = 0; break;		// CLC
	case 0xD8: c.df = 0; break;		// CLD
	case 0x58: c.inf = 0; break;	// CLI
	case 0xB8: c.vf = 0; break;		// CLV

	case 0xB0: branch(c, c.cf); break;	// BCS rel

	case 0x90: branch(c, !c.cf); break; // BCC rel
	case 0xEA: LOG("NOP\n"); break; // NOP
	case 0x78: c.inf = 1; break;	// SEI
	case 0xF8: c.df = 1; break;		// SED

	case 0x08: push_status(c); break; // PHP
	case 0x68: pla(c); break; // PLA

	case 0x29: log_and(c, next_byte(c)); break; // AND #
	case 0xC9: cmp(c, c.ar, next_byte(c)); break; // CMP #

	case 0x48: push_byte(c, c.ar); break; // PHA

	case 0x28: pull_status(c); break; // PLP

	case 0x30: branch(c, c.nf); break; // BMI

	case 0x09: log_or(c, next_byte(c)); break; // ORA #

	case 0x49: log_xor(c, next_byte(c)); break; //EOR #

	case 0x69: c.ar = addb(c, c.ar, next_byte(c), c.cf); break; // ADC #

	case 0xA0: ldy(c,next_byte(c)); break; // LDY #

	case 0xC0: cmp(c, c.yr, next_byte(c)); break; // CPY #

	case 0xE0: cmp(c, c.xr, next_byte(c)); break; // CPX #

	case 0xCD: cmp(c, c.ar, c.read_byte(next_word(c))); break; // CMP abs

	case 0xE9: c.ar = subb(c, c.ar, next_byte(c), !c.cf); break; // SBC #

	case 0xC8: xy_reg_crement(c, c.yr, 1); break; // INY

	case 0xE8: xy_reg_crement(c, c.xr, 1); break; // INX

	case 0x88: xy_reg_crement(c, c.yr, -1); break; // DEY

	case 0xCA: xy_reg_crement(c, c.xr, -1); break; // DEX
	case 0xA8: transfer(c, c.yr, c.ar); break;	// TAY
	case 0xBA: transfer(c, c.xr, c.sp); break;	// TSX
	case 0x8A: transfer(c, c.ar, c.xr); break;	// TXA
	case 0x9A: transfer(c, c.sp, c.xr, false); break;	// TXS
	case 0x98: transfer(c, c.ar, c.yr); break;	// TYA
	case 0xAA: transfer(c, c.xr, c.ar); break;	// TAX


	case 0x8E: c.write_byte(next_word(c), c.xr); break; // STX abs
	case 0x8C: c.write_byte(next_word(c), c.yr); break; // STY abs


	// NO BUGS, WORKING

	default:
		throw std::runtime_error("UNKNOWN OPCODE");
	}
}