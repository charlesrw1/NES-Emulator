#include <iostream>
#include <cstring>
#include "cpu.h"
#include "log.h"

const int cpu_cycles[0x100] = {
	7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
	2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
	2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
	2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
	2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
	2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
	2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};

void execute_opcode(CPU& c, uint8_t opcode);
uint8_t next_byte(CPU& c);
void push_byte(CPU& c, uint8_t val);
void push_status(CPU& c, bool break_set=1);
void CPU::step()
{
	extra_cycle_adr = 0;
	opcode_extra_cycle = 0;

	uint8_t opcode = next_byte(*this);
	
	LOG(CPU_Info) << std::hex << "PC: " << +(pc - 1) << ", Opcode: " << +opcode << std::endl;

	execute_opcode(*this, opcode);

	cycles += cpu_cycles[opcode];
	cycles += (extra_cycle_adr & opcode_extra_cycle);
}
void CPU::irq()
{
	// If interrupts aren't disabled
	if (!inf)
	{
		LOG(CPU_Info) << "Maskable interrupt entered\n";
		push_byte(*this, (pc >> 8) & 0xFF);
		push_byte(*this, (pc) & 0xFF);
		inf = 1;
		push_status(*this, 0);

		// Interrupt vector address
		uint16_t addr = 0xFFFE;
		uint16_t lo = read_byte(addr);
		uint16_t hi = read_byte(addr + 1);
		pc = (hi << 8) | lo;
		
		cycles += 7;
	}
}
void CPU::nmi()
{
	//LOG(CPU_Info) << "Non-maskable interrupt entered\n";
	push_byte(*this, (pc >> 8) & 0xFF);
	push_byte(*this, (pc) & 0xFF);
	inf = 1;
	push_status(*this, 0);

	uint16_t addr = 0xFFFA;
	uint16_t lo = read_byte(addr);
	uint16_t hi = read_byte(addr + 1);
	pc = (hi << 8) | lo;

	cycles += 7;
}
void CPU::reset()
{
	LOG(CPU_Info) << "CPU reset\n";
	uint16_t lo = read_byte(0xFFFC);
	uint16_t hi = read_byte(0xFFFC + 1);
	pc = (hi << 8) | lo;
	ar = xr = yr = 0;
	sp = 0xFD;
	nf = vf = df = zf = cf = 0;
	inf = 1;

	cycles = 7;
}

inline uint8_t peek_byte(CPU& c)
{
	return c.read_byte(c.pc);
}
inline uint8_t next_byte(CPU& c)
{
	return c.read_byte(c.pc++);
}
inline uint16_t next_word(CPU& c)
{
	c.pc += 2;
	return c.read_word(c.pc - 2);
}
// Stack ops, stack ranges form 0x0100 to 0x01FF
inline void push_byte(CPU& c, uint8_t val)
{
	c.write_byte(c.sp | 0x0100, val);
	--c.sp;
}
inline uint8_t pop_byte(CPU& c)
{
	c.sp += 1;
	return c.read_byte(c.sp | 0x100);
}
inline void push_word(CPU& c, uint16_t val)
{
	c.write_word((c.sp) | 0x0100, val);
	c.sp -= 2;
}
inline uint16_t pop_word(CPU& c)
{
	uint16_t val = pop_byte(c);
	val |= pop_byte(c) << 8;
	//c.sp += 2;
	//return c.read_word(c.sp | 0x0100);
	return val;
}
inline void jsr(CPU& c)
{
	//push_word(c,c.pc+1);
	push_byte(c, (c.pc + 1) >> 8 & 0xFF);
	push_byte(c, (c.pc + 1) & 0xFF);
	c.pc = next_word(c);
}
inline uint16_t get_relative(CPU& c, uint8_t rel)
{
	return c.pc + (int8_t)rel;
}
inline void branch(CPU& c, bool cond)
{
	uint16_t addr = get_relative(c, next_byte(c));
	if (cond) {
		// If branch occurs on same page, add a cycle, if different,
		// add 2 cycles
		if ((c.pc & 0xFF00) != (addr & 0xFF00)) {
			c.cycles += 2;
		}
		else {
			c.cycles++;
		}
		c.pc = addr;
	}
}
#define GET_BIT(n, val) (((val) >> (n)) & 1)
// BIT bits 7 and 6 are transfered to n and v after & with a and val
inline void test_bit(CPU& c, uint8_t val)
{
	uint8_t res = c.ar & val;
	c.nf = GET_BIT(7, val);
	c.vf = GET_BIT(6, val);
	c.zf = res == 0;
}
inline void lda(CPU& c, uint8_t val)
{
	c.ar = val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;

	c.opcode_extra_cycle = 1;
}
inline void ldx(CPU& c, uint8_t val)
{
	c.xr = val;
	c.zf = c.xr == 0;
	c.nf = (c.xr >> 7) & 1;

	c.opcode_extra_cycle = 1;
}
inline void xy_reg_crement(CPU& c, uint8_t& z, int8_t dec_inc)
{
	z = z + dec_inc;
	c.zf = z == 0;
	c.nf = (z >> 7) & 1;
}
inline void dec_increment(CPU& c, uint16_t addr, int8_t dec_inc)
{
	uint8_t temp = c.read_byte(addr);
	temp = temp + dec_inc;
	c.zf = temp == 0;
	c.nf = (temp >> 7) & 1;
	c.write_byte(addr, temp);
}
inline bool carry(uint8_t v1, uint8_t v2, bool cy, int bit)
{
	int32_t res = v1 + v2 + cy;
	int32_t carry = res ^ v1 ^ v2;
	return (carry) & (1 << bit);
}
// Add byte
inline uint8_t addb(CPU& c, uint8_t v1, uint8_t v2, bool cy)
{
	uint8_t res = v1 + v2 + cy;
	c.nf = (res >> 7) &1;
	c.zf = res == 0;
	c.cf = carry(v1, v2, cy, 8);
	// if carry out doesn't equal carry on last bit, overflow occured
	c.vf = carry(v1, v2, cy, 7) != carry(v1, v2, cy, 8);

	c.opcode_extra_cycle = 1;
	return res;
}
// Subtract bytes
inline uint8_t subb(CPU& c, uint8_t v1, uint8_t v2, bool cy)
{
	// twos complement, A-B = A+~B+1
	// cy will be 1 after !cy unless subtracting it too
	uint8_t res = addb(c, v1, ~v2, cy);
	//c.cf = !c.cf;

	return res;
}
inline void cmp(CPU& c, uint8_t val1, uint8_t val2)
{
	// Possible bug? Refrence says cmp doesn't change overflow flag,
	// so ill just store and restore if subb modifies it
	bool v_state = c.vf;
	subb(c, val1, val2, 1);
	c.vf = v_state;
}
inline void push_status(CPU& c, bool break_set)
{
	uint8_t res=0;
	res |= (c.cf << 0);
	res |= (c.zf << 1);
	res |= (c.inf << 2);
	res |= (c.df << 3);
	// BRK and PHP pushes 1 on to the stack,
	// Hardware interrupts IRQ and NMI push 0
	res |= (break_set << 4);
	res |= (1 << 5);
	res |= (c.vf << 6);
	res |= (c.nf << 7);
	push_byte(c, res);
}
inline void pull_status(CPU& c)
{
	uint8_t st = pop_byte(c);
	c.cf = (st >> 0) & 1;
	c.zf = (st >> 1) & 1;
	c.inf = (st >> 2) & 1;
	c.df = (st >> 3) & 1;
	c.vf = (st >> 6) & 1;
	c.nf = (st >> 7) & 1;
}
inline void log_and(CPU& c, uint8_t val)
{
	c.ar = c.ar & val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;

	c.opcode_extra_cycle = 1;
}
inline void pla(CPU& c)
{
	c.ar = pop_byte(c);
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;
}
inline void log_or(CPU& c, uint8_t val)
{
	c.ar = c.ar | val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;

	c.opcode_extra_cycle = 1;
}
inline void log_xor(CPU& c, uint8_t val)
{
	c.ar = c.ar ^ val;
	c.zf = c.ar == 0;
	c.nf = (c.ar >> 7) & 1;

	c.opcode_extra_cycle = 1;
}
inline void ldy(CPU& c, uint8_t val)
{
	c.yr = val;
	c.zf = c.yr == 0;
	c.nf = (c.yr >> 7) & 1;

	c.opcode_extra_cycle = 1;
}
inline uint8_t absolute(CPU& c, uint16_t addr)
{
	return c.read_byte(addr);
}
inline void transfer(CPU& c, uint8_t& dest, uint8_t val, bool set_flags = true)
{
	dest = val;
	if (set_flags) {
		c.zf = val == 0;
		c.nf = val & 0x80;
	}
}
inline uint8_t lsr(CPU& c, uint8_t val)
{
	c.cf = val & 1;
	c.zf = (val >> 1) == 0;
	val = val >> 1;
	c.nf = 0;
	return val;
}
inline uint8_t asl(CPU& c, uint8_t val)
{
	c.cf = val & (1 << 7);
	val = val << 1;
	c.zf = val == 0;
	c.nf = val & (1 << 7);
	return val;
}
inline uint8_t ror(CPU& c, uint8_t val)
{
	uint8_t temp;
	temp = val & 1;
	val = (val >> 1) | (c.cf << 7);
	c.nf = c.cf;
	c.cf = temp;
	c.zf = val == 0;
	return val;
}
inline uint8_t rol(CPU& c, uint8_t val)
{
	uint8_t temp;
	temp = val & (1 << 7);
	val = (val << 1) | c.cf;
	c.nf = val & (1 << 7);
	c.cf = temp;
	c.zf = val == 0;
	return val;
}
inline uint8_t zpg(CPU& c)
{
	return c.read_byte(next_byte(c));
}
inline uint8_t zpg_x(CPU& c)
{
	return (next_byte(c) + c.xr);
}
inline uint8_t zpg_y(CPU& c)
{
	return (next_byte(c) + c.yr);
}
inline uint16_t indirect_x(CPU& c)
{
	uint8_t val = next_byte(c);
	uint16_t lo = c.read_byte(((uint16_t)val + c.xr) & 0xFF);
	uint16_t hi = c.read_byte(((uint16_t)val + c.xr + 1) & 0xFF);
	uint16_t res = (hi << 8) | lo;

	return res;
}
inline uint16_t indirect_y(CPU& c)
{
	uint16_t val = next_byte(c);
	uint16_t lo = c.read_byte(val & 0xFF);
	uint16_t hi = c.read_byte((val +1)& 0xFF);
	uint16_t res = (hi << 8) | lo;
	res += c.yr;
	
	// extra cycle for crossed page boundary
	if ((res & 0xFF00) != (hi << 8))
		c.extra_cycle_adr = 1;
	return res;
}
inline uint16_t indirect(CPU& c)
{
	uint16_t ptr = next_word(c);

	if ((ptr&0xFF) == 0x00FF) // Simulate page boundary hardware bug
	{
		return (c.read_byte(ptr & 0xFF00) << 8) | c.read_byte(ptr);
	}
	return (c.read_byte(ptr + 1) << 8) | c.read_byte(ptr);
}
inline uint16_t absolute_y(CPU& c)
{
	uint16_t res = next_word(c);
	// detect carry in address + y register
	uint8_t hi = res >> 8;
	res += c.yr;
	if ((res & 0xFF00) != (hi << 8))
		c.extra_cycle_adr = 1;
	return res;
}
inline uint16_t absolute_x(CPU& c)
{
	uint16_t res = next_word(c);
	// detect carry in address + y register
	uint8_t hi = res >> 8;
	res += c.xr;
	if ((res & 0xFF00) != (hi << 8))
		c.extra_cycle_adr = 1;
	return res;
}
inline void brk(CPU& c)
{
	push_byte(c, (c.pc >> 8) & 0xFF);
	push_byte(c, (c.pc) & 0xFF);
	c.inf = 1;
	push_status(c);

	uint16_t addr = 0xFFFE;
	uint16_t lo = c.read_byte(addr);
	uint16_t hi = c.read_byte(addr + 1);
	c.pc = (hi << 8) | lo;
}
#define ZPGV c.read_byte(next_byte(c))
#define ABSV c.read_byte(next_word(c))
#define INDYV c.read_byte(indirect_y(c))
#define ABSYV c.read_byte(absolute_y(c))
#define ZPGXV c.read_byte(zpg_x(c))
#define ZPGYV c.read_byte(zpg_y(c))
#define ABSXV c.read_byte(absolute_x(c))
void execute_opcode(CPU& c, uint8_t opcode)
{
	if (c.pc == 0xc22c+1)
	{
		printf("");
	}
	switch (opcode)
	{
	case 0x65: c.ar = addb(c, c.ar, c.read_byte(next_byte(c)), c.cf); break; // ADC zpg
	case 0x69: c.ar = addb(c, c.ar, next_byte(c), c.cf); break; // ADC #
	case 0x61: c.ar = addb(c, c.ar, c.read_byte(indirect_x(c)), c.cf); break; // ADC x, ind
	case 0x6D: c.ar = addb(c, c.ar, ABSV, c.cf); break;		// ADC abs
	case 0x71: c.ar = addb(c, c.ar, INDYV, c.cf); break;	// ADC ind, y
	case 0x79: c.ar = addb(c, c.ar, ABSYV, c.cf); break;	// ADC abs, y
	case 0x75: c.ar = addb(c, c.ar, ZPGXV, c.cf); break;	// ADC zpg, x
	case 0x7D: c.ar = addb(c, c.ar, ABSXV, c.cf); break;	// ADC abs, x

	case 0x29: log_and(c, next_byte(c)); break;					// AND #
	case 0x21: log_and(c, c.read_byte(indirect_x(c))); break;	// AND x, ind
	case 0x25: log_and(c, zpg(c)); break;						// AND zpg
	case 0x2D: log_and(c, ABSV); break;							// AND abs
	case 0x31: log_and(c, INDYV); break;						// AND ind,y
	case 0x39: log_and(c, ABSYV); break;						// AND abs, y
	case 0x35: log_and(c, ZPGXV); break;						// AND zpg, x
	case 0x3D: log_and(c, ABSXV); break;						// AND abs, x

	
	case 0xF0: branch(c, c.zf); break;	// BEQ rel
	case 0xD0: branch(c, !c.zf); break;	// BNE rel
	case 0x70: branch(c, c.vf); break;	// BVS rel
	case 0x50: branch(c, !c.vf); break; // BVC rel
	case 0x10: branch(c, !c.nf); break; // BPL rel
	case 0xB0: branch(c, c.cf); break;	// BCS rel
	case 0x90: branch(c, !c.cf); break; // BCC rel
	case 0x30: branch(c, c.nf); break;	// BMI
	
	case 0x24: test_bit(c, c.read_byte(next_byte(c))); break; // BIT zpg
	case 0x2C: test_bit(c, c.read_byte(next_word(c))); break; // BIT abs
	
	case 0x4C: c.pc = next_word(c); break; // JMP abs
	case 0x60: c.pc = pop_word(c); 
		c.pc += 1; break;					// RTS
	case 0x20: jsr(c); break;				// JSR abs
	case 0x40: pull_status(c); 
			c.pc = pop_word(c); break;		// RTI
	case 0x6C: c.pc = indirect(c); break;	// JMP ind
	case 0x00: brk(c); break;				// BRK

	case 0x38: c.cf = 1; break;		// SEC
	case 0x18: c.cf = 0; break;		// CLC
	case 0xD8: c.df = 0; break;		// CLD
	case 0x58: c.inf = 0; break;	// CLI
	case 0xB8: c.vf = 0; break;		// CLV
	case 0x78: c.inf = 1; break;	// SEI
	case 0xF8: c.df = 1; break;		// SED


	case 0xEA: break; // NOP

	case 0x08: push_status(c); break; // PHP
	case 0x28: pull_status(c); break; // PLP

	case 0x68: pla(c); break;				// PLA
	case 0x48: push_byte(c, c.ar); break;	// PHA

	case 0xC9: cmp(c, c.ar, next_byte(c)); break;				// CMP #
	case 0xC5: cmp(c, c.ar, c.read_byte(next_byte(c))); break;	// CMP zpg
	case 0xCD: cmp(c, c.ar, c.read_byte(next_word(c))); break;	// CMP abs
	case 0xC1: cmp(c, c.ar, c.read_byte(indirect_x(c))); break; // CMP x, ind
	case 0xD1: cmp(c, c.ar, INDYV); break;						// CMP ind, y
	case 0xD9: cmp(c, c.ar, ABSYV); break;						// CMP abs, y
	case 0xD5: cmp(c, c.ar, ZPGXV); break;						// CMP zpg, x
	case 0xDD: cmp(c, c.ar, ABSXV); break;						// CMP abs, x

	case 0xE4: cmp(c, c.xr, ZPGV); break;				// CPX zpg
	case 0xE0: cmp(c, c.xr, next_byte(c)); break;		// CPX #
	case 0xEC: cmp(c, c.xr, ABSV); break;				// CPX abs

	case 0xC0: cmp(c, c.yr, next_byte(c)); break;		// CPY #
	case 0xC4: cmp(c, c.yr, ZPGV); break;				// CPY zpg
	case 0xCC: cmp(c, c.yr, ABSV); break;				// CPY abs




	case 0x09: log_or(c, next_byte(c)); break;					// ORA #
	case 0x01: log_or(c, c.read_byte(indirect_x(c))); break;	// ORA x, ind
	case 0x05: log_or(c, zpg(c)); break;						// ORA zpg
	case 0x0D: log_or(c, ABSV); break;							// ORA abs
	case 0x11: log_or(c, INDYV); break;							// ORA ind, y
	case 0x19: log_or(c, ABSYV); break;							// ORA abs, y
	case 0x15: log_or(c, ZPGXV); break;							// ORA zpg, x
	case 0x1D: log_or(c, ABSXV); break;							// ORA abs, x
	
	case 0x49: log_xor(c, next_byte(c)); break;					// EOR #
	case 0x41: log_xor(c, c.read_byte(indirect_x(c))); break;	// EOR x, ind
	case 0x45: log_xor(c, zpg(c)); break;						// EOR zpg
	case 0x4D: log_xor(c, ABSV); break;							// EOR abs
	case 0x51: log_xor(c, INDYV); break;						// EOR ind, y
	case 0x59: log_xor(c, ABSYV); break;						// EOR abs, y
	case 0x55: log_xor(c, ZPGXV); break;						// EOR zpg, x
	case 0x5D: log_xor(c, ABSXV); break;						// EOR abs, x


	case 0xA9: lda(c, next_byte(c)); break;					// LDA #
	case 0xA5: lda(c, c.read_byte(next_byte(c))); break;	// LDA zpg
	case 0xA1: lda(c, c.read_byte(indirect_x(c))); break;	// LDA x, ind
	case 0xAD: lda(c, c.read_byte(next_word(c))); break;	// LDA abs
	case 0xB1: lda(c, c.read_byte(indirect_y(c))); break;	// LDA ind, y
	case 0xB9: lda(c, c.read_byte(absolute_y(c))); break;	// LDA abs, y
	case 0xB5: lda(c, ZPGXV); break;						// LDA zpg, x
	case 0xBD: lda(c, ABSXV); break;						// LDA abs, x

	case 0xA2: ldx(c, next_byte(c)); break;					// LDX #
	case 0xA6: ldx(c, zpg(c)); break;						// LDX zpg
	case 0xAE: ldx(c, c.read_byte(next_word(c))); break;	// LDX abs
	case 0xB6: ldx(c, ZPGYV); break;						// LDX zpg, y
	case 0xBE: ldx(c, ABSYV); break;						// LDX abs, y
	
	case 0xA0: ldy(c,next_byte(c)); break;					// LDY #
	case 0xA4: ldy(c, zpg(c));	break;						// LDY zpg
	case 0xAC: ldy(c, c.read_byte(next_word(c))); break;	// LDY abs
	case 0xB4: ldy(c, c.read_byte(zpg_x(c))); break;		// LDY zpg, x
	case 0xBC: ldy(c, ABSXV); break;						// LDY abs, x




	case 0xE9: c.ar = subb(c, c.ar, next_byte(c), c.cf); break;					// SBC #
	case 0xE5: c.ar = subb(c, c.ar, c.read_byte(next_byte(c)), c.cf); break;	// SBC zpg
	case 0xE1: c.ar = subb(c, c.ar, c.read_byte(indirect_x(c)), c.cf); break;	// SBC x, ind
	case 0xED: c.ar = subb(c, c.ar, ABSV, c.cf); break;							// SBC abs
	case 0xF1: c.ar = subb(c, c.ar, INDYV, c.cf); break;						// SBC ind, y
	case 0xF9: c.ar = subb(c, c.ar, ABSYV, c.cf); break;						// SBC abs, y
	case 0xF5: c.ar = subb(c, c.ar, ZPGXV, c.cf); break;						// SBC zpg, x
	case 0xFD: c.ar = subb(c, c.ar, ABSXV, c.cf); break;						// SBC abs, x

	case 0xC8: xy_reg_crement(c, c.yr, 1); break;	// INY
	case 0xE8: xy_reg_crement(c, c.xr, 1); break;	// INX
	case 0x88: xy_reg_crement(c, c.yr, -1); break;	// DEY
	case 0xCA: xy_reg_crement(c, c.xr, -1); break;	// DEX

	case 0xA8: transfer(c, c.yr, c.ar); break;		// TAY
	case 0xBA: transfer(c, c.xr, c.sp); break;		// TSX
	case 0x8A: transfer(c, c.ar, c.xr); break;		// TXA
	case 0x9A: transfer(c, c.sp, c.xr, 0); break;	// TXS
	case 0x98: transfer(c, c.ar, c.yr); break;		// TYA
	case 0xAA: transfer(c, c.xr, c.ar); break;		// TAX



	case 0x85: c.write_byte(next_byte(c), c.ar); break;		// STA zpg
	case 0x81: c.write_byte(indirect_x(c), c.ar); break;	// STA x, ind
	case 0x8D: c.write_byte(next_word(c), c.ar); break;		// STA abs
	case 0x99: c.write_byte(absolute_y(c), c.ar); break;	// STA abs, y
	case 0x91: c.write_byte(indirect_y(c), c.ar); break;	// STA ind, y
	case 0x95: c.write_byte(zpg_x(c), c.ar); break;			// STA zpg, x
	case 0x9D: c.write_byte(absolute_x(c), c.ar); break;	// STA abs, x

	case 0x86: c.write_byte(next_byte(c), c.xr); break; // STX zpg
	case 0x8E: c.write_byte(next_word(c), c.xr); break; // STX abs
	case 0x96: c.write_byte(zpg_y(c), c.xr); break;		// STX zpg, y

	case 0x8C: c.write_byte(next_word(c), c.yr); break; // STY abs
	case 0x84: c.write_byte(next_byte(c), c.yr); break;	// STY zpg
	case 0x94: c.write_byte(zpg_x(c), c.yr); break;		// STY zpg, x




	case 0xEE: dec_increment(c, next_word(c), 1); break;	// INC abs
	case 0xCE: dec_increment(c, next_word(c), -1); break;	// DEC abs
	case 0xC6: dec_increment(c, next_byte(c), -1); break;	// DEC zpg
	case 0xE6: dec_increment(c, next_byte(c), 1); break;	// INC zpg
	case 0xD6: dec_increment(c, zpg_x(c), -1); break;		// DEC zpg, x
	case 0xF6: dec_increment(c, zpg_x(c), 1); break;		// INC zpg, x
	case 0xDE: dec_increment(c, absolute_x(c), -1); break;	// DEC abs, x
	case 0xFE: dec_increment(c, absolute_x(c), 1); break;	// INC abs, x


	
	case 0x4A: c.ar = lsr(c, c.ar); break;		// LSR A
	case 0x46: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, lsr(c, c.read_byte(addr)));
	} break;									// LSR zpg
	case 0x4E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, lsr(c, c.read_byte(addr)));
	} break;									// LSR abs
	case 0x56: {
		uint16_t addr = zpg_x(c);
		c.write_byte(addr, lsr(c, c.read_byte(addr)));
	} break;									// LSR zpg, x
	case 0x5E: {
		uint16_t addr = absolute_x(c);
		c.write_byte(addr, lsr(c, c.read_byte(addr)));
	} break;									// LSR abs, x


	case 0x0A: c.ar = asl(c, c.ar); break;		// ASL A
	case 0x0E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, asl(c, c.read_byte(addr)));
	} break;									// ASL abs
	case 0x06: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, asl(c, c.read_byte(addr)));
	} break;									// ASL zpg
	case 0x16: {
		uint16_t addr = zpg_x(c);
		c.write_byte(addr, asl(c, c.read_byte(addr)));
	} break;									// ASL zpg, x
	case 0x1E: {
		uint16_t addr = absolute_x(c);
		c.write_byte(addr, asl(c, c.read_byte(addr)));
	} break;									// ASL abs, x



	case 0x2A: c.ar = rol(c, c.ar); break;		// ROL A
	case 0x26: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, rol(c, c.read_byte(addr)));
	} break;									// ROL zpg
	case 0x2E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, rol(c, c.read_byte(addr)));
	} break;									// ROL abs
	case 0x36: {
		uint16_t addr = zpg_x(c);
		c.write_byte(addr, rol(c, c.read_byte(addr)));
	} break;									// ROL zpg, x
	case 0x3E: {
		uint16_t addr = absolute_x(c);
		c.write_byte(addr, rol(c, c.read_byte(addr)));
	} break;									// ROL abs, x


	case 0x6A: c.ar = ror(c, c.ar); break;		// ROR A
	case 0x6E: {
		uint16_t addr = next_word(c);
		c.write_byte(addr, ror(c, c.read_byte(addr)));
	} break;									// ROR abs
	case 0x66: {
		uint8_t addr = next_byte(c);
		c.write_byte(addr, ror(c, c.read_byte(addr)));
	}break;										// ROR zpg
	case 0x76: {
		uint16_t addr = zpg_x(c);
		c.write_byte(addr, ror(c, c.read_byte(addr)));
	} break;									// ROR zpg, x
	case 0x7E: {
		uint16_t addr = absolute_x(c);
		c.write_byte(addr, ror(c, c.read_byte(addr)));
	} break;									// ROR abs, x

	default:
		LOG(Error) << "Unknown Opcode: " << +opcode << std::endl;
		//throw std::runtime_error("UNKNOWN OPCODE");
	}
 }