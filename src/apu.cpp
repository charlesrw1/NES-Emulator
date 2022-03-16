#include "apu.h"

static constexpr bool DUTY_CYCLE_TABLE[][8] = {
  { 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 0, 0, 0 },
  { 1, 0, 0, 1, 1, 1, 1, 1 }
};
float pulse_table[31];
float tnd_table[203];

APU::APU()
{
	audio_buffer = new sf::Uint16[4096];
	for (int i = 0; i < 31; i++)
		pulse_table[i] = 95.52 / (8128.0 / i + 100);
	for (int i = 0; i < 203; i++)
		tnd_table[i] = 163.67 / (24329.0 / i + 100);
}
APU::~APU()
{
	delete[] audio_buffer;
}
void APU::write_register(uint16_t addr, uint8_t val)
{

}
unsigned int get_ticks()
{
	static sf::Clock clock;
	return clock.getElapsedTime().asMilliseconds();
}

void APU::test()
{
	int samples = 4'000;
	unsigned amplitude = 12'000;
	double twopi = 6.28318;
	double increment = 440.0 / samples;
	sf::SoundBuffer buffer;
	sf::Int16* raw = new sf::Int16[samples];
	double incy = 350.0 / samples;
	double x = 0;
	double y = 0;
	double z = 0;
	double incz = 100 / samples;
	for (int i = 0; i < samples; i++) {
		raw[i] = amplitude * (2*floor(sin(x*twopi*0.6))+1);
		x += increment;//* sin(get_ticks());
		y += incy; 
		z += incz;
	}
	buffer.loadFromSamples(raw, samples, 1, samples);
	sf::Sound sound;
	sound.setBuffer(buffer);
	sound.setLoop(true);
	sound.play();
	sf::sleep(sf::seconds(10));

}