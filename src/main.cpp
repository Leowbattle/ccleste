#include <stddef.h>
#include <stdarg.h>

#include <fxcg/display.h>
#include <fxcg/keyboard.h>

#include "celeste.h"
#include "gfx/gfx.hpp"

void InitCalculator();
void ShowLoadingScreen();
bool keyDown_fast(unsigned char keyCode);

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);

static bool running = true;
static void mainLoop();

int main() {
	InitCalculator();
	ShowLoadingScreen();

	Celeste_P8_set_call_func(pico8emu);

	while (running) mainLoop();

	return 0;
}

void InitCalculator() {
	EnableStatusArea(3); // Disable the status area.
	Bdisp_EnableColor(1); // Enable 16-bit color.
}

void ShowLoadingScreen() {
	DrawFrame(0); // Draw a black screen border.
	Bdisp_Fill_VRAM(0, 3); // Fill screen with black.
	VRAM_CopySprite((const color_t*)loading_image.pixel_data, (LCD_WIDTH_PX - loading_image.width) / 2, (LCD_HEIGHT_PX - loading_image.height) / 2, loading_image.width, loading_image.height); // Draw loading image.
}

static void mainLoop() {
	static int x = 40;
	static int y = 40;

	if (keyDown_fast(KEY_PRGM_MENU)) {
		int key;
		GetKey(&key);
	}
	if (keyDown_fast(KEY_PRGM_LEFT)) {
		x--;
	}
	if (keyDown_fast(KEY_PRGM_RIGHT)) {
		x++;
	}
	if (keyDown_fast(KEY_PRGM_UP)) {
		y--;
	}
	if (keyDown_fast(KEY_PRGM_DOWN)) {
		y++;
	}

	Bdisp_SetPoint_VRAM(x, y, 0xffff);
	Bdisp_PutDisp_DD();
}

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...) {
	return 0;
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
			*(VRAM++) = *(sprite++);
		}
		VRAM += LCD_WIDTH_PX-width;
	}
}