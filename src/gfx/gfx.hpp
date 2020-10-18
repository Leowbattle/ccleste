#ifndef GFX_HPP
#define GFX_HPP

typedef struct Image {
	unsigned int width;
	unsigned int height;
	unsigned int bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
	unsigned char pixel_data[];
} Image;

const extern Image loading_image;
const extern Image gfx_image;
const extern Image font_image;

#endif