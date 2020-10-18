#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/rtc.h>

#include <stdio.h>

int main() {
	int frameCount = 0;
	int ticks = RTC_GetTicks();
	int fps = 0;

	while (true) {
		Bdisp_PutDisp_DD();
		
		if (PRGM_GetKey() == KEY_PRGM_MENU) {
			int key;
			GetKey(&key);
		}

		char msg[32];

		// There must be spaces at the end because the text does not get cleared every frame.
		// This means that if one frame there were 100 fps, then the next frame there were 60, because
		// the 100 is one character longer it would not get overwritten and it would look like "FPS: 600", which is obviously not true.
		sprintf(msg, "FPS: %d     ", fps);

		locate_OS(1, 1);
		Print_OS(msg, 0, 0);

		frameCount++;

		if (RTC_Elapsed_ms(ticks, 1000)) {
			ticks = RTC_GetTicks();
			fps = frameCount;
			frameCount = 0;
		}
	}

	return 0;
}

int PRGM_GetKey(void){
	unsigned char buffer[12];
	PRGM_GetKey_OS( buffer );
	return ( buffer[1] & 0x0F ) * 10 + ( ( buffer[2] & 0xF0 ) >> 4 );
}
