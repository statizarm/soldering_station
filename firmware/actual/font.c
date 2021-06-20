#include "font.h"
#include <stdlib.h>

extern struct {
	uint8_t first;
	uint8_t last;
	uint8_t width;
	uint8_t height;
	const uint8_t (*bitmaps)[256];
} font_64;

extern struct {
	uint8_t first;
	uint8_t last;
	uint8_t width;
	uint8_t height;
	const uint8_t (*bitmaps)[48];
} font_24;

const uint8_t *get_bitmap(uint8_t c, uint8_t size, uint8_t *width, uint8_t *height)
{
	switch (size) {
		case 24:
			if (c < font_24.first || c > font_64.last) {
				return NULL;
			} else {
				*width = font_24.width;
				*height = font_24.height;
				return font_24.bitmaps[c - font_24.first];
			}
		case 64:
			switch (c) {
				case '0': case '1': case '2': case '3': case '4': 
				case '5': case '6': case '7': case '8': case '9':
					*width = font_64.width;
					*height = font_64.height;
					return font_64.bitmaps[c - '0'];
					break;
				case 'C':
					*width = font_64.width;
					*height = font_64.height;
					return font_64.bitmaps[10];
				case 0xB0: 
					*width = font_64.width;
					*height = font_64.height;
					return font_64.bitmaps[11];
			}
	}
			
	return NULL;
}