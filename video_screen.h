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
		// TEMPORARY, Shouldn't clear+display window
		image_texture.loadFromImage(image);
		rect.setTexture(&image_texture);
		sf::Sprite s;
		s.setTexture(image_texture);
		s.scale(0.8f, 0.8f);
		window.clear({ 0x90,0x90,0x90 });
		window.draw(s);
		window.display();
	}
	void reset()
	{
		image.create(256, 240, { 0xFF, 0x00, 0xFF });
	}
private:
	sf::Image image;
	sf::Texture image_texture;
	sf::RenderWindow& window;
	sf::RectangleShape rect;
};

#endif // !VIDEOSCREEN_H