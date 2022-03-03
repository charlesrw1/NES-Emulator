#include "mainram.h"
#include "cartridge.h"
#include "mapper.h"
#include "ppu.h"

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
		return memory[addr & 0x07FF];
	}
	// 8 PPU registers mirrored for 2KB
	else if (addr < 0x4000) {
		return ppu.RegisterRead(addr & 0x2007);
	}
	// IO and APU registers
	else if (addr < 0x4020) {
		if (addr == 0x4016) {
			uint8_t res = (controller_port_1&(1<<7))>0;
			controller_port_1 <<= 1;
	
			if (res != 0) {
				LOG(Debug) << "INPUT DETECTED\n";
			}
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
			if (val == 1) {
				//controller_port_1 = cached_controller_port1;
					uint8_t input = 0;
					for (int i = 0; i < 8; i++) {
						if (sf::Keyboard::isKeyPressed(input_keys[7-i])) {
							input |= (1 << i);
						}
					}
					controller_port_1 = input;
			}
		}
	}
	else {
		cart.mapper->write_prg(addr, val);
	}
}