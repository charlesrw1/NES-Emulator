#ifndef VIDEOSCREEN_H
#define VIDEOSCREEN_H
#include "SFML/Graphics.hpp"

struct VideoScreen
{
	VideoScreen(sf::RenderWindow& window) : window(window)
	{
		image.create(256, 240, { 0xFF, 0x00, 0xFF });
		rect.setSize({ 256.f, 240.f });
		rect.setTexture(&image_texture);
	}
	void set_pixel(int x, int y, sf::Color color) {
		image.setPixel(x, y, color);
	}
	void render() {
		image_texture.loadFromImage(image);
		window.draw(rect);
	}
private:
	sf::Image image;
	sf::Texture image_texture;
	sf::RenderWindow& window;
	sf::RectangleShape rect;
};

#endif // !VIDEOSCREEN_H