#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <cstdint>
#include "SFML/Graphics.hpp"

struct Controller
{
	const sf::Keyboard::Key input_keys[8] = { sf::Keyboard::A,sf::Keyboard::S,sf::Keyboard::Q,sf::Keyboard::W,
		sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Left, sf::Keyboard::Right };
	Controller() {
		button_state = 0;
		strobe = 0;
		index = 0;
	}
	void update() {
		uint8_t button_state = 0;
		for (int i = 0; i < 8; i++) {
			if (sf::Keyboard::isKeyPressed(input_keys[i])) {
				button_state |= (1 << i);
			}
		}
	}
	uint8_t read() {
		if (index > 7) {
			return 1;
		}
		uint8_t res = (button_state & ( 1 << index)) >> index;
		if (!strobe && index <= 7) {
			++index;
		}
		return res;
	}
	void write(uint8_t val)
	{
		strobe = val & 1;
		if (strobe) {
			index = 0;
		}
	}
	uint8_t button_state;
	bool strobe;
	uint8_t index;
};


#endif