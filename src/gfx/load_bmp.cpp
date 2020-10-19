#include "gfx.hpp"

extern "C" const uint8_t gfx_bmp[];
extern "C" const uint8_t font_bmp[];

Image gfx_image;
Image font_image;

// https://en.wikipedia.org/wiki/BMP_file_format
typedef struct BMP_Header {
	uint16_t boring_magic_number;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;

	uint32_t dib_size;
	uint32_t width;
	uint32_t height;
	// We don't really need the rest, so I ignore them ;)
} BMP_Header;

uint32_t little_to_big_endian_u32(uint32_t x) {
	return x >> 8;
}

Image loadImage(const uint8_t* bmp) {
	const BMP_Header* header = (const BMP_Header*)bmp;

	Image image;
	image.pixels = bmp + little_to_big_endian_u32(header->offset);
	image.width = little_to_big_endian_u32(header->width);
	image.height = little_to_big_endian_u32(header->height);
	return image;
}

void LoadImages() {
	gfx_image = loadImage(gfx_bmp);
	font_image = loadImage(font_bmp);
}