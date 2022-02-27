#ifndef MAPPER_H
#define MAPPER_H
#include <cstdint>

// Virtual class for mapper implmentations to inherit from
class Cartridge;
class Mapper
{
public:
	Mapper(Cartridge& cart) : cart(cart) {}
	virtual uint8_t read_prg(uint16_t addr) = 0;
	virtual void write_prg(uint16_t addr, uint8_t val) = 0;
	virtual uint8_t read_chr(uint16_t addr) = 0;
	virtual void write_chr(uint16_t addr, uint8_t val) = 0;

	Cartridge& cart;
};
#endif // !MAPPER_H