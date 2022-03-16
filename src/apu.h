#ifndef APU_H
#define APU_h
#include "SFML/Audio.hpp"
#include <cmath>

struct Pulse
{
	uint8_t duty;

	uint16_t timer_period;
};

class APU
{
public:
	APU();
	~APU();
	void write_register(uint16_t addr, uint8_t val);
	void test();
	
	int buf_index = 0;
	sf::Uint16* audio_buffer;
};

#endif // !APU_H
