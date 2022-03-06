#include "emulator.h"
void Emulator::load_cartridge(std::string file)
{
	cart.load_from_file(file, ppu_ram);
	ppu_ram.update_mirroring();
}
uint8_t cpu_status(CPU& c)
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
void Emulator::step()
{
	static int cycles_since_nmi = 0;
	static bool in_nmi = false;
	uint64_t total_cycles = 0;
	// About 1 frame
	//main_ram.cont1.update();

	uint8_t input = 0;
	for (int i = 0; i < 8; i++) {
		if (sf::Keyboard::isKeyPressed(input_keys[i])) {
			input |= (1 << i);
		}
	}
	main_ram.cached_controller_port1 = input;

	while (total_cycles < 29780) {


		for (int i = 0; i < cpu.cycles * 3; i++) {
			ppu.clock();
		}
		cpu.cycles = 0;

		if (main_ram.DMA_request) {
			uint8_t* oam_ptr = reinterpret_cast<uint8_t*>(ppu.OAM);
			for (int i = 0; i < 256; i++) {
				// NESDEV wiki isn't clear, I assume the writes wrap around if oam_addr is anything greater than 0
				uint8_t data = main_ram.read_byte((main_ram.DMA_page << 8) | uint8_t(i));
				oam_ptr[uint8_t(i + ppu.oam_addr)] = data;
			}
			main_ram.DMA_request = false;
			// Actually this could be either 513/514, maybe a problem
			ppu.cycle += 513;
		}
		if (ppu.send_nmi_output) {
			if (ppu.generate_NMI) {
				LOG(CPU_Info) << "NMI ENTERED" << std::endl;
				in_nmi = true;
				cpu.nmi();
			}
			ppu.send_nmi_output = false;
		}

		LOG(CPU_Info) << "     " << std::hex << +(cpu.pc)
			<< " $" << +main_ram.read_byte(cpu.pc)
			<< " A: " << +cpu.ar
			<< " X: " << +cpu.xr
			<< " Y: " << +cpu.yr
			<< " P:" << +cpu_status(cpu)
			<< " C CYC:" << std::dec << cpu.total_cycles
			<< " SL: " << ppu.scanline << " P CYC " << ppu.cycle 
			<< " DTAADR: " << std::hex << +ppu.data_address
			<< " TEMPADR: " << std::hex << +ppu.temp_addr << std::endl;

		cpu.step();


		total_cycles += cpu.cycles;
		if (in_nmi) {
			cycles_since_nmi += cpu.cycles;
		}
	}
}