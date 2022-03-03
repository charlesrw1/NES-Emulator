#include "mainram.h"
#include "cartridge.h"
#include "mapper.h"
#include "ppu.h"

#include <bitset>


const int scanlines_pf = 262;
const int clocks_psl = 342;

MainRAM::MainRAM(PPU& ppu, Cartridge& cart) : ppu(ppu), cart(cart)
{
	memory = new uint8_t[0x2000];

	DMA_request = false;
}
MainRAM::~MainRAM()
{
	delete[] memory;
}
uint8_t MainRAM::read_byte(uint16_t addr)
{
	// 2KB zero page, stack, and ram mirrored to 0x2000
	if (addr < 0x2000) {
		if (addr == 0x0) {
			LOG(Debug) << "TEMP JOYPAD BITS READ: " << std::bitset<8>(memory[addr & 0x07FF]) << std::endl;
		}
		if (addr == 0x06fc) {
			LOG(Debug) << "JOYPAD BITS READ: " << std::bitset<8>(memory[addr & 0x07FF]) << std::endl;
		}
#ifdef EVIL_MARIO_HACK
		if (addr == 0x06fd) {
			// Hail Mary, full of grace, the lord is with you. Bless art thou amoungst women and blessed is the fruit of thy womb Jesus christ.
			// Holy Mary, mother of god, pray for us sinners, now and at the hour of our death. Amen.
			return 0;
		}
#endif
		if (addr == 0x074a) {
			LOG(Debug) << "JOYPAD BIT MASK READ: " << std::bitset<8>(memory[addr & 0x07FF]) << std::endl;
		}
		return memory[addr & 0x07FF];
	}
	// 8 PPU registers mirrored for 2KB
	else if (addr < 0x4000) {
		return ppu.RegisterRead(addr & 0x2007);
	}
	// IO and APU registers
	else if (addr < 0x4020) {
		if (addr == 0x4016) {
			LOG(Debug) << "INPUT READ " << std::bitset<8>(controller_port_1) << std::endl;
			uint8_t res = (controller_port_1&1);
			controller_port_1 >>= 1;
	
			return res;
		}
	}
	else {
		return cart.mapper->read_prg(addr);
	}
}
				const sf::Keyboard::Key input_keys[8] = { sf::Keyboard::A,sf::Keyboard::S,sf::Keyboard::Q,sf::Keyboard::W,
		sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Left, sf::Keyboard::Right };
void MainRAM::write_byte(uint16_t addr, uint8_t val)
{
	if (addr < 0x2000) {
		if (addr == 0x0) {
			LOG(Debug) << "TEMP JOYPAD BITS SAVED: " << std::bitset<8>(val) << std::endl;
		}
		if (addr == 0x06fc) {
			LOG(Debug) << "JOYPAD BITS SAVED " << std::bitset<8>(val) << std::endl;
		}
#ifdef EVIL_MARIO_HACK
		if (addr == 0x074a) {
			LOG(Debug) << "JOYPAD BIT MASK SAVED " << std::bitset<8>(val) << std::endl;
			// Hail Mary, full of grace, the lord is with you. Bless art thou amoungst women and blessed is the fruit of thy womb Jesus christ.
			// Holy Mary, mother of god, pray for us sinners, now and at the hour of our death. Amen.
			return;
		}
#endif
		memory[addr & 0x07FF] = val;
	}
	else if (addr < 0x4000) {
		ppu.RegisterWrite(addr & 0x2007, val);
	}
	else if (addr < 0x4020) {

		// PPU DMA register, CPU suspended for 512+1 cycles to transfer
		// data to PPU OAM
		if (addr == 0x4014) {
			DMA_request = true;
			DMA_page = val;
		}

		// accesses IO registers
		if (addr == 0x4016) {
			if (val == 0) {
				controller_port_1 = cached_controller_port1;
			}
		}
	}
	else {
		cart.mapper->write_prg(addr, val);
	}
}