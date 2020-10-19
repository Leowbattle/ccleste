#ifndef GFX_HPP
#define GFX_HPP

#include "../stdint.h"

#include <fxcg/display.h>

typedef struct Image {
	const uint8_t* pixels;
	int width;
	int height;
} Image;

extern Image gfx_image;
extern Image font_image;

void LoadImages();

#endif