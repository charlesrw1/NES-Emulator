// NES-Emulator.cpp : Defines the entry point for the application.
//

#include "NES-Emulator.h"
#include "cpu.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include "mos6502/mos6502.h"
//#include "SimpleNES/include/CPU.h"
#include "olcNES/Part#2 - CPU/Bus.h"
#include "log.h"

std::ostream* Log::stream;
Level Log::log_level;

using namespace std;

uint8_t peek_byte(CPU& c, uint8_t offset) 
{
	return c.read_byte(c.pc + offset);
}
uint8_t hex_from_str(string s)
{
	assert(s.size() == 2);
	uint8_t res = (s.at(1)>='A')?(s.at(1)-'A'+10):(s.at(1) - '0');
	uint8_t temp = (s.at(0) >= 'A') ? (s.at(0) - 'A'+10) : (s.at(0) - '0');
	res |= temp << 4;

	return res;

}
uint8_t get_cpu_status_register(CPU& c)
{
	uint8_t res = 0;
	res |= (c.cf << 0);
	res |= (c.zf << 1);
	res |= (c.inf << 2);
	res |= (c.df << 3);
	res |= (0 << 4);
	res |= (1 << 5);
	res |= (c.vf << 6);
	res |= (c.nf << 7);
	return res;
}
void assert_log_and_cpu(CPU& c, std::string& log_line)
{

	stringstream ss, help;
	ss << log_line;
	string sub;
	uint16_t pc;
	ss >> hex >> pc;
	if (pc != c.pc) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("PC: %x != %x\n", c.pc, pc);
	}
	
	int ctr = 0;
	/*
	while (1) {
		ss >> sub;
		if (sub.size() >= 3) break;
		uint8_t sz = hex_from_str(sub);
	}
	*/


	int pos = log_line.rfind("A:");
	
	uint8_t temp_hex;
	string s = log_line.substr(pos+2, 2);
	temp_hex = hex_from_str(s);

	if (temp_hex != c.ar) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("AR: %x != %x\n", c.ar, temp_hex);
	}
	pos = log_line.rfind("SP:");
	s =  log_line.substr(pos+3, 2);
	temp_hex = hex_from_str(s);
	if (temp_hex != c.sp) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("SP: %x != %x\n", c.sp, temp_hex);
	}
	pos = log_line.find("P:");
	s = log_line.substr(pos + 2, 2);
	temp_hex = hex_from_str(s);
	uint8_t status = get_cpu_status_register(c);
	if (temp_hex != status) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("Status: %x != %x\n", status, temp_hex);
	}
	pos = log_line.rfind("X:");
	s = log_line.substr(pos + 2, 2);
	temp_hex = hex_from_str(s);
	if (temp_hex != c.xr) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("XR: %x != %x\n", c.xr, temp_hex);
	}
	pos = log_line.rfind("Y:");
	s = log_line.substr(pos + 2, 2);
	temp_hex = hex_from_str(s);
	if (temp_hex != c.yr) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("YR: %x != %x\n", c.yr, temp_hex);
	}
	pos = log_line.rfind("CYC:");
	s = log_line.substr(pos+4);
	help << s;
	uint64_t cycles;
	help >> cycles;
	if (cycles != c.cycles) {
		printf("\033[%dm%s\033[m ", 101, "ERROR");
		printf("CYC: %lld != %lld\n", c.cycles, cycles);
	}
}
uint8_t* mos_mem;

void compare(Bus c2, CPU& c)
{
	for (int i = 0; i < 0x07ff; i++){
		assert(c2.ram[i] == c.memory[i]);
	}
	assert(c2.cpu.a == c.ar);
	uint8_t mst = get_cpu_status_register(c);
	//assert(st == mst);
	assert(c2.cpu.pc == c.pc);
	assert(c2.cpu.stkp == c.sp);
	assert(c2.cpu.x == c.xr);
	assert(c2.cpu.y == c.yr);
}

uint8_t read(uint16_t adr)
{
	return mos_mem[adr];
}
void write(uint16_t adr, uint8_t val)
{
	mos_mem[adr] = val;
}
int main()
{
	Log::log_level = Error;
	Log::set_stream(&std::cout);

	CPU c{};
	c.memory = new uint8_t[0xFFFF];
	c.pc = 0xC000;
	c.sp = 0xFD;
	c.inf = 1;
	c.cycles = 7;
	mos_mem = new uint8_t[0xFFFF];

	mos6502 mos(&read, &write);
	mos.pc = 0xC000;
	mos.sp = 0xFD;
	mos.A = 0;
	mos.X = 0;
	mos.Y = 0;
	mos.status = 36;

	//sn::MainBus mb;
	//sn::CPU c2(mb);
	//c2.r_PC = 0xC000;
	
	Bus olc;
	olc.cpu.pc = 0xC000;
	olc.ram = new uint8_t[0xFFFF];
	olc.cpu.stkp = 0xFD;

	std::ifstream rom("nestest.nes", std::ios::binary);
	if (!rom) return 1;
	rom.seekg(0, std::ios_base::end);
	int length = rom.tellg();
	rom.seekg(0, ios_base::beg);
	rom.read((char*)&c.memory[0xC000 - 0x10], 0x4000);
	//mb.m_RAM.resize(0x4000);
	rom.seekg(0, ios_base::beg);
	rom.read((char*)&olc.ram[0xC000 - 0x10], 0x4000);


	std::ifstream nestestlog("nestest.log");
	if (!nestestlog) return 1;
	std::string line;
	uint64_t cycles;
	while (1) 
	{
		getline(nestestlog, line);
		assert_log_and_cpu(c, line);
		c.step();
		olc.cpu.clock();
		compare(olc,c);

	}
	
	cout << "Hello CMake." << endl;
	return 0;
}
