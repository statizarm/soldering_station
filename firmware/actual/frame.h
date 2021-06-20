#ifndef FRAME_H_
#define FRAME_H_

#include <stdint.h>
#include <stddef.h>

enum {TEXTURE, POLYGON, ANIMATION, TEXT};

struct frame;

typedef struct {
	uint16_t x;
	uint16_t y;
} position_t;

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t *data;
} texture_t;

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t color;
} polygon_t;

typedef struct {
	struct frame *frames;
	uint8_t curr;
	uint8_t n;
} animation_t;

typedef struct {
	char *str;
	uint16_t font_color;
	uint16_t bg_color;
	uint8_t font_size;
} text_t;

typedef struct frame {
	union {
		texture_t	texture;
		polygon_t	polygon;
		animation_t	animation;
		text_t		text;
	};
	
	uint8_t type;
} frame_t;

void init_graphics(void);
void draw_frame(frame_t *, position_t pos);
		
#endif // FRAME_H_
