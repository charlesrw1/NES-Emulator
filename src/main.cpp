#include "main.h"
#include "cpu.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include "log.h"
#include "cartridge.h"
#include "mapper.h"
#include "emulator.h"
#include "video_screen.h"

#include "apu.h"

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

int run_nestest(Emulator& app)
{

	app.load_cartridge("nestest.nes");
	app.cpu.pc = 0xC000;

	std::ifstream nestestlog("nestest.log");
	if (!nestestlog) return 1;
	std::string line;

	int counter = 0;
	while (1)
	{
		getline(nestestlog, line);
		assert_log_and_cpu(app.cpu, line);
		app.cpu.step();

		// 0xC6BD is start of illegal opcode tests, I haven't implmented
		// those in my emulator
		if (app.cpu.pc == 0xc6bd) {
			printf("NESTEST Complete\n");
			return 0;
		}
		++counter;
	}
	return 0;
}
sf::FloatRect calcView(const sf::Vector2f& windowSize, float pacRatio)
{
	sf::Vector2f viewSize = windowSize;
	sf::FloatRect viewport(0.f, 0.f, 1.f, 1.f);
	float ratio = viewSize.x / viewSize.y;

	// too high
	if (ratio < pacRatio) { 
		viewSize.y = viewSize.x / pacRatio;
		float vp_h = viewSize.y / windowSize.y;
		viewport.height = vp_h;
		viewport.top = (1.f - vp_h) / 2.f;
	}
	// too wide
	else if (ratio > pacRatio) {
		viewSize.x = viewSize.y * pacRatio;
		float vp_w = viewSize.x / windowSize.x;

		viewport.width = vp_w;
		viewport.left = (1.f - vp_w) / 2.f;
	}
	return viewport;
}
void OnResize(sf::RenderWindow& window, sf::Event& event)
{
	float h = event.size.height;
	float w = event.size.width;
	sf::View view = window.getView();
	view.setViewport(calcView({ w,h }, 254.f/240.f));
	window.setView(view);
}


#include "SFML/Audio.hpp"

void compare(const std::string& my, const std::string& mesen)
{
	std::stringstream ss;
	std::string temp,temp1;
	temp = my.substr(5, 4);
	for (int i = 0; i < 4; i++) {
		temp.at(i) = toupper(temp.at(i));
	}
	ss << mesen;
	ss >> temp1;
	assert(temp == temp1);
	uint64_t my_cycles = 0;
	uint64_t his_cycles = 0;

	int pos = my.find("C CYC:");
	temp = my.substr(pos + 6);
	ss.str("");
	ss << temp;
	ss >> my_cycles;
	pos = mesen.find("CPU Cycle:");
	temp1 = mesen.substr(pos + 10);
	ss.str("");

	ss << temp1;
	ss >> his_cycles;
	std::cout << my_cycles << " " << his_cycles << "\n";
	assert(my_cycles == his_cycles);

}

int compare_mesen_log()
{
	std::ifstream my_log("log_dump.txt");
	if (!my_log)
		return 1;
	std::ifstream mesen_log("mesen_loz.txt");
	if (!mesen_log)
		return 1;

	std::string my_line, mesen_line;
	for (int i = 0; i < 7; i++) {
		getline(my_log, my_line);
	}
	unsigned int counter = 7;
	while (1)
	{
		getline(my_log, my_line);
		getline(mesen_log, mesen_line);
		compare(my_line, mesen_line);
		counter++;
	}
}
int main()
{

	Log::log_level = Info;
	std::ofstream log_file("bad_dump.txt");
	Log::set_stream(&std::cout);

	sf::RenderWindow window(sf::VideoMode(256*3, 240*3), "NES-EMULATOR");
	window.setFramerateLimit(60);
	window.setView(sf::View(sf::FloatRect(0, 0, 256, 240)));
	Emulator app(window);
	if (!app.load_cartridge("loz.nes")) {
		return 1;
	}

	app.cpu.reset();

	uint64_t cycles=0;
	VideoScreen vs(window);
	sf::Clock clock;
	sf::Time elapsed;
	while (window.isOpen()) 
	{
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
			case sf::Event::KeyPressed:
				switch (event.key.code)
				{
				case sf::Keyboard::Key::F1:
					app.cpu.reset();
					break;
				case sf::Keyboard::Key::F2:
					Log::log_level = CPU_Info;
					break;
				}
				break;
			case sf::Event::Resized:
				OnResize(window, event);
				break;
			default:
				break;
			}
		}
		app.step();
		elapsed = clock.restart();
		int ms = elapsed.asMilliseconds();
		if (elapsed.asMilliseconds() < 16)
			sf::sleep(sf::milliseconds(16) - elapsed);

	}
	if (app.cart.battery_ram) {
		std::ofstream save_file("../../../saves/" + app.cart.rom_name + ".sav");
		save_file.write((char*)app.cart.extended_ram.data(), 0x2000);
	}
	
	return 0;
}