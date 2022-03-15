#include "emulator.h"
#include <sstream>

//#define RING_BUFFER_LOGGING

bool Emulator::load_cartridge(std::string file)
{
	if (!cart.load_from_file(file, ppu_ram)) {
		return false;
	}
	ppu_ram.update_mirroring();
	return true;
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
		if (cart.mapper->generate_IRQ) {
			cpu.irq();
			cart.mapper->generate_IRQ = false;
		}
		
//>Logging
		ring_buffer.at(ring_buffer_index).save_state(*this);
		if(ring_buffer_elements < ring_buffer.size())
			ring_buffer_elements += 1;

		if (cpu.dump_log) {
			for (int i = 0; i < ring_buffer_elements; i++) {
				ring_buffer.at((ring_buffer_index + 1 + i) % ring_buffer.size()).print(Log::get_stream());
			}
			ring_buffer_elements = 0;
			cpu.dump_log = false;
			Log::get_stream() << "END LOG DUMP >> START CURRENT\n";
		}

		Level state = Log::log_level;
		if (cpu.log_next_cycles > 0) {
			Log::log_level = CPU_Info;
			cpu.log_next_cycles--;
		}
		if (CPU_Info >= Log::log_level) {
			ring_buffer.at(ring_buffer_index).print(Log::get_stream());
		}
		Log::log_level = state;
		ring_buffer_index = (ring_buffer_index + 1) % ring_buffer.size();
//<Logging

		cpu.step();


		total_cycles += cpu.cycles;
		if (in_nmi) {
			cycles_since_nmi += cpu.cycles;
		}
	}
}
void EmulatorStatus::save_state(Emulator& e)
{
	pc = e.cpu.pc;
	sp = e.cpu.sp;
	ar = e.cpu.ar;
	xr = e.cpu.xr;
	yr = e.cpu.yr;
	nf = e.cpu.nf;
	vf = e.cpu.vf;
	df = e.cpu.df;
	inf = e.cpu.inf;
	zf = e.cpu.zf;
	cf = e.cpu.cf;
	total_cycles = e.cpu.total_cycles;
	rom_addr = e.cart.mapper->get_prg_rom_address(e.cpu.pc);
	ppu_addr = e.ppu.data_address;
	ppu_temp_addr = e.ppu.temp_addr;
	scanline = e.ppu.scanline;
	ppu_cycle = e.ppu.cycle;
	opcode = e.main_ram.read_byte(e.cpu.pc);
}
void EmulatorStatus::print(std::ostream& stream)
{
	stream << std::hex << +(pc)
		<< " $" << +opcode
		<< " A: " << +ar
		<< " X: " << +xr
		<< " Y: " << +yr
		<< " S: " << ((nf) ? 'N' : '-') << ((vf) ? 'V' : '-') << ((inf) ? 'I' : '-') << ((zf) ? 'Z' : '-') << ((cf) ? 'C' : '-');
		stream << " C CYC:" << std::dec << total_cycles
		<< " SL: " << scanline << " P CYC " << ppu_cycle
		<< " DTAADR: " << std::hex << +ppu_addr
		<< " TEMPADR: " << std::hex << +ppu_temp_addr
		<< " PRG ADR: " << rom_addr << '\n';
}