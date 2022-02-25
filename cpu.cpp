#include <iostream>
#include <cstring>
#include "cpu.h"
#define LOG printf

uint8_t peek_byte(CPU& c)
{
	return c.read_byte(c.pc);
}
uint8_t next_byte(CPU& c)
{
	return c.read_byte(c.pc++);
}
uint16_t next_word(CPU& c)
{
	c.pc += 2;
	return c.read_word(c.pc - 2);
}
// Stack ops, stack ranges form 0x0100 to 0x01FF
void push_byte(CPU& c, uint8_t val)
{
	c.write_byte(c.sp | 0x0100, val);
	--c.sp;
}
uint8_t pop_byte(CPU& c)
{
	c.sp += 1;
	return c.read_byte(c.sp | 0x100);
}
void push_word(CPU& c, uint16_t val)
{
	c.write_word((c.sp) | 0x0100, val);
	c.sp -= 2;
}
uint16_t pop_word(CPU& c)
{
	uint16_t val = pop_byte(c);
	val |= pop_byte(c) << 8;
	//c.sp += 2;
	//return c.read_word(c.sp | 0x0100);
	return val;
}
void execute_opcode(CPU& c, uint8_t opcode);
void CPU::step()
{
	execute_opcode(*this, next_byte(*this));
}
void jsr(CPU& c)
{
	//push_word(c,c.pc+1);
	push_byte(c, (c.pc + 1) >> 8 & 0xFF);
	push_byte(c, (c.pc + 1) & 0xFF);
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
void dec_increment(CPU& c, uint16_t addr, int8_t dec_inc)
{
	uint8_t temp = c.read_byte(addr);
	temp = temp + dec_inc;
	c.zf = temp == 0;
	c.nf = (temp >> 7) & 1;
	c.write_byte(addr, temp);
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
uint8_t lsr(CPU& c, uint8_t val)
{
	c.cf = val & 1;
	c.zf = (val >> 1) == 0;
	val = val >> 1;
	c.nf = 0;
	return val;
}
uint8_t asl(CPU& c, uint8_t val)
{
	c.cf = val & (1 << 7);
	val = val << 1;
	c.zf = val == 0;
	c.nf = val & (1 << 7);
	return val;
}
uint8_t ror(CPU& c, uint8_t val)
{
	uint8_t temp;
	temp = val & 1;
	val = (val >> 1) | (c.cf << 7);
	c.nf = c.cf;
	c.cf = temp;
	c.zf = val == 0;
	return val;
}
uint8_t rol(CPU& c, uint8_t val)
{
	uint8_t temp;
	temp = val & (1 << 7);
	val = (val << 1) | c.cf;
	c.nf = val & (1 << 7);
	c.cf = temp;
	c.zf = val == 0;
	return val;
}
uint8_t zpg(CPU& c)
{
	return c.read_byte(next_byte(c));
}
uint8_t zpg_x(CPU& c)
{
	return (next_byte(c) + c.xr);
}
uint16_t indirect_x(CPU& c)
{
	uint8_t val = next_byte(c);
	uint16_t lo = c.read_byte(((uint16_t)val + c.xr) & 0xFF);
	c.zf = lo == 0;
	uint16_t hi = c.read_byte(((uint16_t)val + c.xr + 1) & 0xFF);
	// nes test says it affects these flags I guess
	c.zf = hi == 0;
	c.nf = hi & (1 << 7);
	return (hi << 8) | lo;
}
#define ZPGV c.read_byte(next_byte(c))
#define ABSV c.read_byte(next_word(c))
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
	case 0x60: c.pc = pop_word(c); 	c.pc += 1; break;
	
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

	case 0xAE: ldx(c, c.read_byte(next_word(c))); break; // LDX abs
	case 0xAD: lda(c, c.read_byte(next_word(c))); break; // LDA abs

	case 0x40: pull_status(c); c.pc = pop_word(c); break;	// RTI
	case 0x4A: c.ar = lsr(c, c.ar); break;		// LSR A

	case 0x0A: c.ar = asl(c, c.ar); break;		// ASL A

	case 0x6A: c.ar = ror(c, c.ar); break;		// ROR A

	case 0x2A: c.ar = rol(c, c.ar); break;		// ROL A

	case 0xA5: lda(c, c.read_byte(next_byte(c))); break;		// LDA zpg

	case 0x8D: c.write_byte(next_word(c), c.ar); break;		// STA abs
	case 0xA1: lda(c, c.read_byte(indirect_x(c))); break;	// LDA x, ind

	case 0x81: c.write_byte(indirect_x(c), c.ar); break;	// STA x, ind
	case 0x01: log_or(c, c.read_byte(indirect_x(c))); break; // ORA x, ind


	case 0x21: log_and(c, c.read_byte(indirect_x(c))); break; // AND x, ind
	case 0x41: log_xor(c, c.read_byte(indirect_x(c))); break; // EOR x, ind
	case 0x61: c.ar = addb(c, c.ar, c.read_byte(indirect_x(c)), c.cf); break; // ADC x, ind
	case 0xC1: cmp(c, c.ar, c.read_byte(indirect_x(c))); break; // CMP x, ind
	case 0xE1: c.ar = subb(c, c.ar, c.read_byte(indirect_x(c)), !c.cf); break; // SBC x, ind
	case 0xA4: ldy(c, zpg(c));	break; // LDY zpg

	case 0x84: c.write_byte(next_byte(c), c.yr); break;	// STY zpg
	case 0xA6: ldx(c, zpg(c)); break; // LDX zpg
	case 0x05: log_or(c, zpg(c)); break; // ORA zpg
	case 0x25: log_and(c, zpg(c)); break;	// AND zpg
	case 0x45: log_xor(c, zpg(c)); break;	// EOR zpg
	case 0xC6: dec_increment(c, next_byte(c), -1); break; // DEC zpg
	case 0xE6: dec_increment(c, next_byte(c), 1); break; // INC zpg
	case 0x65: c.ar = addb(c, c.ar, c.read_byte(next_byte(c)), c.cf); break; // ADC zpg

	case 0xC5: cmp(c, c.ar, c.read_byte(next_byte(c))); break;	//CMP zpg
	case 0xE5: c.ar = subb(c, c.ar, c.read_byte(next_byte(c)), !c.cf); break; // SBC zpg
	case 0xE4: cmp(c, c.xr, ZPGV); break; // CMP zpg
	case 0xC4: cmp(c, c.yr, ZPGV); break; // CMP ypg
	
	case 0x46: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, lsr(c, c.read_byte(addr)));
	} break;	// LSR zpg
	case 0x26: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, rol(c, c.read_byte(addr)));
	} break;	// ROL zpg
	case 0x06: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, asl(c, c.read_byte(addr)));
	} break;	// ASL zpg
	case 0x66: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, ror(c, c.read_byte(addr)));
	}break;	// ROR zpg

	case 0xAC: ldy(c, c.read_byte(next_word(c))); break;	// LDY abs
	case 0x2C: test_bit(c, c.read_byte(next_word(c))); break; // BIT abs

	case 0x0D: log_or(c, ABSV); break; // ORA abs
	case 0x2D: log_and(c, ABSV); break; // AND abs
	case 0x4D: log_xor(c, ABSV); break; // EOR abs
	case 0x6D: c.ar = addb(c, c.ar, ABSV, c.cf); break; // ADC abs
	case 0xED: c.ar = subb(c, c.ar, ABSV, !c.cf); break; // SBC abs
//	case 0xCD: cmp(c, c.ar, ABSV); break; // CMP abs
//	case 0xAD: lda(c, ABSV); break; // LDA abs
//	case 0x8D: c.write_byte(next_word(c), c.ar); break; // STA abs
	case 0x0E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, asl(c, c.read_byte(addr)));
	} break;	// ASL abs
	case 0x2E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, rol(c, c.read_byte(addr)));
	} break;	// ROL abs
	case 0x4E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, lsr(c, c.read_byte(addr)));
	} break;	// LSR abs
	case 0x6E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, ror(c, c.read_byte(addr)));
	} break;	// ROR abs
//	case 0x8E: c.write_byte(next_word(c), c.xr); break;	// STX abs
//	case 0xAE: ldx(c, ABSV); break; // LDX abs
	case 0xEC: cmp(c, c.xr, ABSV); break; // CPX abs
	case 0xCC: cmp(c, c.yr, ABSV); break; // CPY abs
	case 0xEE: dec_increment(c, next_word(c), 1); break; // INC abs
	case 0xCE: dec_increment(c, next_word(c), -1); break; // DEC abs


	case 0x55: log_xor(c, c.read_byte(zpg_x(c))); break;	// EOR zpg, x
	default:
		throw std::runtime_error("UNKNOWN OPCODE");
	}
}