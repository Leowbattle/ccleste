#include <stddef.h>
#include "stdint.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/rtc.h>

#include "celeste.h"
#include "tilemap.h"
#include "gfx/gfx.hpp"

void InitCalculator();
void ShowLoadingScreen();
bool keyDown_fast(unsigned char keyCode);

typedef struct SDL_Rect {
	int x;
	int y;
	int w;
	int h;
} SDL_Rect;

void fillRect(int x, int y, int w, int h, color_t colour);
void drawSprite(Image* image, SDL_Rect src, int x, int y);

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);

static int gettileflag(int tile, int flag) {
	return tile < sizeof(tile_flags)/sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

#define PICO8_W 128
#define PICO8_H 128

static const int scale = 1;

// http://www.rinkydinkelectronics.com/calc_rgb565.php
static const color_t base_palette[16] = {
	0x0000,
	0x194A,
	0x792A,
	0x042A,
	0xAA86,
	0x5AA9,
	0xC618,
	0xFF9D,
	0xF809,
	0xFD00,
	0xFF64,
	0x0726,
	0x2D7F,
	0x83B3,
	0xFBB5,
	0xFE75
};
static color_t palette[16];

static inline color_t getcolor(char idx) {
	return palette[idx%16];
}

static void ResetPalette(void) {
	//SDL_SetPalette(surf, SDL_PHYSPAL|SDL_LOGPAL, (SDL_Color*)base_palette, 0, 16);
	//memcpy(screen->format->palette->colors, base_palette, 16*sizeof(SDL_Color));
	memcpy(palette, base_palette, sizeof palette);
}

static uint8_t buttons_state = 0; // The pico8 button state.
static bool enable_screenshake = true;
static bool paused = false;
static bool running = true;
static void mainLoop();

static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static void p8_print(const char* str, int x, int y, int col);

//on-screen display (for info, such as loading a state, toggling screenshake, toggling fullscreen, etc)
static char osd_text[200] = "";
static int osd_timer = 0;
static void OSDset(const char* fmt, ...) {
	sprintf(osd_text, fmt);
	osd_text[sizeof osd_text - 1] = '\0'; //make sure to add NUL terminator in case of truncation
	//printf("%s\n", osd_text);
	osd_timer = 30;
}
static void OSDdraw(void) {
	if (osd_timer > 0) {
		--osd_timer;
		const int x = 4;
		const int y = 120 + (osd_timer < 10 ? 10-osd_timer : 0); //disappear by going below the screen
		p8_rectfill(x-2, y-2, x+4*strlen(osd_text), y+6, 6); //outline
		p8_rectfill(x-1, y-1, x+4*strlen(osd_text)-1, y+5, 0);
		p8_print(osd_text, x, y, 7);
	}
}

int main() {
	InitCalculator();
	
	LoadImages();

	Celeste_P8_set_call_func(pico8emu);
	Celeste_P8_set_rndseed(RTC_GetTicks());
	Celeste_P8_init();

	while (running) {
		mainLoop();

		// printf("a        %d %d", gfx_image.width, gfx_image.height);

		// color_t* vram = (color_t*)GetVRAMAddress();
		// for (int i = 0; i < gfx_image.height; i++) {
		// 	for (int j = 0; j < gfx_image.width; j++) {
		// 		int offset = i * gfx_image.width + j;
		// 		uint8_t b = gfx_image.pixels[offset / 2 + offset % 2];

		// 		vram[i * LCD_WIDTH_PX + j] = getcolor(b);
		// 	}
		// }

		// drawSprite(&gfx_image, {0, 0, 128, 64}, 0, 64);

		// Bdisp_PutDisp_DD();

		// int key;
		// GetKey(&key);
	}

	return 0;
}

void InitCalculator() {
	EnableStatusArea(3); // Disable the status area.
	Bdisp_EnableColor(1); // Enable 16-bit color.

	DrawFrame(0); // Draw a black screen border.
	Bdisp_Fill_VRAM(0, 3); // Fill screen with black.
}

static void mainLoop() {
	int frameStart = RTC_GetTicks();

	//Bdisp_Fill_VRAM(0, 3); // Fill screen with black.

	if (keyDown_fast(KEY_PRGM_MENU)) {
		// GetKey checks if the Menu key is pressed.
		// If it is, the OS halts the process and goes to the uhh... menu.
		int key;
		GetKey(&key);
	}

	if (keyDown_fast(KEY_PRGM_EXIT)) {
		paused = !paused;
	}
	else if (keyDown_fast(KEY_PRGM_5)) {
		Celeste_P8__DEBUG();
	}
	else if (keyDown_fast(KEY_PRGM_6)) {
		enable_screenshake = !enable_screenshake;
	}

	buttons_state = 0;
	{
		// Update pico8 button state.
		if (keyDown_fast(KEY_PRGM_LEFT))  buttons_state |= (1<<0);
		if (keyDown_fast(KEY_PRGM_RIGHT)) buttons_state |= (1<<1);
		if (keyDown_fast(KEY_PRGM_UP))    buttons_state |= (1<<2);
		if (keyDown_fast(KEY_PRGM_DOWN))  buttons_state |= (1<<3);
		if (keyDown_fast(KEY_PRGM_7)) buttons_state |= (1<<4);
		if (keyDown_fast(KEY_PRGM_8)) buttons_state |= (1<<5);
	}

	if (paused) {
		const int x0 = PICO8_W/2-3*4, y0 = 8;

		p8_rectfill(x0-1,y0-1, 6*4+x0+1,6+y0+1, 6);
		p8_rectfill(x0,y0, 6*4+x0,6+y0, 0);
		p8_print("paused", x0+1, y0+1, 7);
	} else {
		Celeste_P8_update();
		Celeste_P8_draw();
	}
	OSDdraw();

	Bdisp_PutDisp_DD(); // Display backbuffer on screen.

	// Wait for the end of the frame.
	while (!RTC_Elapsed_ms(frameStart, 32));
}

// The calculator is big endian. Swap bytes in images.
inline color_t color_correct(color_t color) {
	return ((color & 0xFF) << 8) | ((color & 0xFF00) >> 8);
}

void fillRect(int x, int y, int w, int h, color_t colour) {
	color_t* vram = (color_t*)GetVRAMAddress();
	for (int i = y; i < y + h; i++) {
		for (int j = x; j < x + w; j++) {
			vram[i * LCD_WIDTH_PX + j] = colour;
		}
	}
}

void drawSprite(Image* image, SDL_Rect src, int x, int y) {
	color_t* vram = (color_t*)GetVRAMAddress();
	for (int i = 0; i < src.h; i++) {
		for (int j = 0; j < src.w; j++) {
			int offset = (i + src.y) * image->width + j + src.x;
			//int offset = (src.y + i) * image->width + src.x + j;
			uint8_t b = image->pixels[offset] >> 4;

			vram[(i + y) * LCD_WIDTH_PX + j + x] = getcolor(b);
		}
	}
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col) {
	int w = (x1 - x0 + 1)*scale;
	int h = (y1 - y0 + 1)*scale;
	if (w > 0 && h > 0) {
		fillRect(x0, y0, w, h, getcolor(col));
	}
}

static void p8_print(const char* str, int x, int y, int col) {
	for (char c = *str; c; c = *(++str)) {
		c &= 0x7F;
		SDL_Rect srcrc = {8*(c%16), 8*(c/16)};
		srcrc.x *= scale;
		srcrc.y *= scale;
		srcrc.w = srcrc.h = 8*scale;

		// TODO col
		drawSprite(&font_image, srcrc, x, y);
		x += 4;
	}
}

int abs(int x) {
	return x > 0 ? x : -x;
}

//coordinates should NOT be scaled before calling this
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color) {
	#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
	CLAMP(x0,0,PICO8_W);
	CLAMP(y0,0,PICO8_H);
	CLAMP(x1,0,PICO8_W);
	CLAMP(y1,0,PICO8_H);

	uint32_t realcolor = getcolor(color);

	color_t* vram = (color_t*)GetVRAMAddress();
	#define PLOT(x, y) vram[y * LCD_HEIGHT_PX + x] = realcolor;

	#undef CLAMP
	int sx, sy, dx, dy, err, e2;
	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	if (!dx && !dy) return;

	if (x0 < x1) sx = 1; else sx = -1;
	if (y0 < y1) sy = 1; else sy = -1;
	err = dx - dy;
	if (!dy && !dx) return;
	else if (!dx) { //vertical line
		for (int y = y0; y != y1; y += sy) PLOT(x0,y);
	} else if (!dy) { //horizontal line
		for (int x = x0; x != x1; x += sx) PLOT(x,y0);
	} while (x0 != x1 || y0 != y1) {
		PLOT(x0, y0);
		e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
	#undef PLOT
}

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...) {
	static int camera_x = 0, camera_y = 0;
	if (!enable_screenshake) {
		camera_x = camera_y = 0;
	}

	va_list args;
	int ret = 0;
	va_start(args, call);
	
	#define   INT_ARG() va_arg(args, int)
	#define  BOOL_ARG() (Celeste_P8_bool_t)va_arg(args, int)
	#define RET_INT(_i)   do {ret = (_i); goto end;} while (0)
	#define RET_BOOL(_b) RET_INT(!!(_b))
	#define CASE(t, ...) case t: {__VA_ARGS__;} break;

	switch (call) {
		CASE(CELESTE_P8_MUSIC, //music(idx,fade,mask)
			// No sound on the calculator.
		)
		CASE(CELESTE_P8_SPR, //spr(sprite,x,y,cols,rows,flipx,flipy)
			int sprite = INT_ARG();
			int x = INT_ARG();
			int y = INT_ARG();
			int cols = INT_ARG();
			int rows = INT_ARG();
			int flipx = BOOL_ARG();
			int flipy = BOOL_ARG();

			(void)cols;
			(void)rows;

			//assert(rows == 1 && cols == 1);

			if (sprite >= 0) {
				SDL_Rect srcrc = {
					8*(sprite % 16),
					8*(sprite / 16)
				};
				srcrc.x *= scale;
				srcrc.y *= scale;
				srcrc.w = srcrc.h = scale*8;
				SDL_Rect dstrc = {
					(x - camera_x)*scale, (y - camera_y)*scale,
					scale, scale
				};
				//Xblit(gfx, &srcrc, screen, &dstrc, 0,flipx,flipy);
				// TODO Flip
				drawSprite(&gfx_image, srcrc, x, y);
			}
		)
		CASE(CELESTE_P8_BTN, //btn(b)
			RET_BOOL(buttons_state & (1 << (INT_ARG())));
		)
		CASE(CELESTE_P8_SFX, //sfx(id)
			// No sound on the calculator.
		)
		CASE(CELESTE_P8_PAL, //pal(a,b)
			int a = INT_ARG();
			int b = INT_ARG();
			if (a >= 0 && a < 16 && b >= 0 && b < 16) {
				//swap palette colors
				palette[a] = base_palette[b];
			}
		)
		CASE(CELESTE_P8_PAL_RESET, //pal()
			ResetPalette();
		)
		CASE(CELESTE_P8_CIRCFILL, //circfill(x,y,r,col)
			int cx = INT_ARG() - camera_x;
			int cy = INT_ARG() - camera_y;
			int r = INT_ARG();
			int col = INT_ARG();

			int realcolor = getcolor(col);

			if (r <= 1) {
				fillRect(cx - 1, cy, 3, 1, realcolor);
				fillRect(cx, cy - 1, 1, 3, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*(cx-1), scale*cy, scale*3, scale}, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*cx, scale*(cy-1), scale, scale*3}, realcolor);
			} else if (r <= 2) {
				fillRect(cx - 2, cy - 1, 5, 3, realcolor);
				fillRect(cx - 1, cy - 2, 3, 5, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*(cx-2), scale*(cy-1), scale*5, scale*3}, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*(cx-1), scale*(cy-2), scale*3, scale*5}, realcolor);
			} else if (r <= 3) {
				fillRect(cx - 3, cy - 1, 7, 3, realcolor);
				fillRect(cx - 1, cy - 3, 3, 7, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*(cx-3), scale*(cy-1), scale*7, scale*3}, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*(cx-1), scale*(cy-3), scale*3, scale*7}, realcolor);
				//SDL_FillRect(screen, &(SDL_Rect){scale*(cx-2), scale*(cy-2), scale*5, scale*5}, realcolor);
			} else { //i dont think the game uses this
				int f = 1 - r; //used to track the progress of the drawn circle (since its semi-recursive)
				int ddFx = 1; //step x
				int ddFy = -2 * r; //step y
				int x = 0;
				int y = r;

				//this algorithm doesn't account for the diameters
				//so we have to set them manually
				p8_line(cx,cy-y, cx,cy+r, col);
				p8_line(cx+r,cy, cx-r,cy, col);

				while (x < y) {
					if (f >= 0) {
						y--;
						ddFy += 2;
						f += ddFy;
					}
					x++;
					ddFx += 2;
					f += ddFx;

					//build our current arc
					p8_line(cx+x,cy+y, cx-x,cy+y, col);
					p8_line(cx+x,cy-y, cx-x,cy-y, col);
					p8_line(cx+y,cy+x, cx-y,cy+x, col);
					p8_line(cx+y,cy-x, cx-y,cy-x, col);
				}
			}
		)
		CASE(CELESTE_P8_PRINT, //print(str,x,y,col)
			const char* str = va_arg(args, const char*);
			int x = INT_ARG() - camera_x;
			int y = INT_ARG() - camera_y;
			int col = INT_ARG() % 16;

#ifdef _3DS
			if (!strcmp(str, "x+c")) {
				//this is confusing, as 3DS uses a+b button, so use this hack to make it more appropiate
				str = "a+b";
			}
#endif

			p8_print(str,x,y,col);
		)
		CASE(CELESTE_P8_RECTFILL, //rectfill(x0,y0,x1,y1,col)
			int x0 = INT_ARG() - camera_x;
			int y0 = INT_ARG() - camera_y;
			int x1 = INT_ARG() - camera_x;
			int y1 = INT_ARG() - camera_y;
			int col = INT_ARG();

			p8_rectfill(x0,y0,x1,y1,col);
		)
		CASE(CELESTE_P8_LINE, //line(x0,y0,x1,y1,col)
			int x0 = INT_ARG() - camera_x;
			int y0 = INT_ARG() - camera_y;
			int x1 = INT_ARG() - camera_x;
			int y1 = INT_ARG() - camera_y;
			int col = INT_ARG();

			p8_line(x0,y0,x1,y1,col);
		)
		CASE(CELESTE_P8_MGET, //mget(tx,ty)
			int tx = INT_ARG();
			int ty = INT_ARG();

			RET_INT(tilemap_data[tx+ty*128]);
		)
		CASE(CELESTE_P8_CAMERA, //camera(x,y)
			if (enable_screenshake) {
				camera_x = INT_ARG();
				camera_y = INT_ARG();
			}
		)
		CASE(CELESTE_P8_FGET, //fget(tile,flag)
			int tile = INT_ARG();
			int flag = INT_ARG();

			RET_INT(gettileflag(tile, flag));
		)
		CASE(CELESTE_P8_MAP, //map(mx,my,tx,ty,mw,mh,mask)
			int mx = INT_ARG(), my = INT_ARG();
			int tx = INT_ARG(), ty = INT_ARG();
			int mw = INT_ARG(), mh = INT_ARG();
			int mask = INT_ARG();
			
			for (int x = 0; x < mw; x++) {
				for (int y = 0; y < mh; y++) {
					int tile = tilemap_data[x + mx + (y + my)*128];
					//hack
					if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask-1 : mask)) {
						//al_draw_bitmap(sprites[tile], tx+x*8 - camera_x, ty+y*8 - camera_y, 0);
						SDL_Rect srcrc = {
							8*(tile % 16),
							8*(tile / 16)
						};
						srcrc.x *= scale;
						srcrc.y *= scale;
						srcrc.w = srcrc.h = scale*8;
						SDL_Rect dstrc = {
							(tx+x*8 - camera_x)*scale, (ty+y*8 - camera_y)*scale,
							scale*8, scale*8
						};

						if (0) {
							srcrc.x = srcrc.y = 0;
							srcrc.w = srcrc.h = 8;
							dstrc.x = x*8, dstrc.y = y*8;
							dstrc.w = dstrc.h = 8;
						}

						//SDL_BlitSurface(gfx, &srcrc, screen, &dstrc);
						//Xblit(gfx, &srcrc, screen, &dstrc, 0, 0, 0);
						drawSprite(&gfx_image, srcrc, x * 8, y * 8);
						//fillRect(x * 8, y * 8, 8, 8, 0xffff);
					}
				}
			}
		)
	}

	end:
	va_end(args);
	return ret;
}

// https://github.com/tswilliamson/nesizm/blob/56eb48f2915033a94f48046f02e8733a0b591d4e/src/nes_input.cpp#L12
bool keyDown_fast(unsigned char keyCode) {
	static const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;

	int row, col, word, bit;
	row = keyCode % 10;
	col = keyCode / 10 - 1;
	word = row >> 1;
	bit = col + 8 * (row & 1);
	return (keyboard_register[word] & (1 << bit));
}

void VRAM_CopySprite(const color_t* sprite, int x, int y, int width, int height) {
	color_t* VRAM = (color_t*)GetVRAMAddress();
	VRAM += LCD_WIDTH_PX*y + x;
	for(int j=y; j<y+height; j++) {
		for(int i=x; i<x+width; i++) {
			*(VRAM++) = color_correct(*(sprite++));
		}
		VRAM += LCD_WIDTH_PX-width;
	}
}