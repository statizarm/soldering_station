#include "frame.h"
#include "ili9341_driver.h"
#include "font.h"

static ili9341_driver_t _driver = {
	.spi = SPI2,
	.cs = (uint32_t *) (PERIPH_BB_BASE + ((uint32_t)&GPIOB->ODR - PERIPH_BASE) * 32 + 48),
	.dc = (uint32_t *) (PERIPH_BB_BASE + ((uint32_t)&GPIOB->ODR - PERIPH_BASE) * 32 + 40),
	.reset = (uint32_t *) (PERIPH_BB_BASE + ((uint32_t)&GPIOB->ODR - PERIPH_BASE) * 32 + 44),
	.sem = {.value = 1}
};

static uint8_t _cmd_buf[20];

void delay(uint32_t);

void init_graphics()
{
	*_driver.reset = 1;

	delay(10000);
	ili9341_driver_init(&_driver);
}

static void _draw_texture(texture_t *texture, position_t pos);
static void _draw_polygon(polygon_t *polygon, position_t pos);
static void _draw_animation(animation_t *animation, position_t pos);
static void _draw_text(text_t *text, position_t pos);

void draw_frame(frame_t *frame, position_t pos)
{
	switch (frame->type) {
		case TEXTURE:
			_draw_texture(&frame->texture, pos);
			break;
		case POLYGON:
			_draw_polygon(&frame->polygon, pos);
			break;
		case ANIMATION:
			_draw_animation(&frame->animation, pos);
			break;
		case TEXT:
			_draw_text(&frame->text, pos);
			break;
		default:
			break;
	}
	return;
}

static void __init_column_cmd(cmd_t *cmd, uint16_t start, uint16_t end)
{
	cmd->cmd = 0x2A;
	cmd->argc = 4;
	cmd->argv[0] = start >> 8;
	cmd->argv[1] = start & 0xFF;
	cmd->argv[2] = end >> 8;
	cmd->argv[3] = end & 0xFF;
}

static void __init_row_cmd(cmd_t *cmd, uint16_t start, uint16_t end)
{
	cmd->cmd = 0x2B;
	cmd->argc = 4;
	cmd->argv[0] = start >> 8;
	cmd->argv[1] = start & 0xFF;
	cmd->argv[2] = end >> 8;
	cmd->argv[3] = end & 0xFF;
}

static void __select_page(uint16_t xstart, uint16_t ystart, uint16_t xend, uint16_t yend)
{
	cmd_t *cmd = (cmd_t *) _cmd_buf;
	
	__init_column_cmd(cmd, xstart, xend - 1);
	ili9341_driver_wcmd(&_driver, cmd);

	__init_row_cmd(cmd, ystart, yend - 1);
	ili9341_driver_wcmd(&_driver, cmd);
}

static void _draw_texture(texture_t *texture, position_t pos)
{
	__select_page(pos.x, pos.y, pos.x + texture->width, pos.y + texture->height);

	ili9341_driver_wpixels(&_driver, texture->data, texture->height * texture->width);
}

static void _draw_polygon(polygon_t *poly, position_t pos)
{
	__select_page(pos.x, pos.y, pos.x + poly->width, pos.y + poly->height);
	
	ili9341_driver_wcolor(&_driver, poly->color, poly->height * poly->width);
}

static void _draw_animation(animation_t *anim, position_t pos)
{
	draw_frame(&anim->frames[anim->curr], pos);
	
	++anim->curr;

	if (anim->curr == anim->n) {
		anim->curr = 0;
	}
}

#define LETTER_TEXTURE_BUFFER_SIZE (128)

static inline void __fill_font_texture_frame(uint16_t *buf, uint8_t mask, uint8_t n, uint16_t font_color, uint16_t bg_color);

static void _draw_text(text_t *text, position_t pos)
{
	static uint16_t letter_texture_buffer[2][LETTER_TEXTURE_BUFFER_SIZE];
	static uint8_t curr_buffer_id = 0;
	
	texture_t letter_texture = {
		.data = letter_texture_buffer[curr_buffer_id]
	};
	position_t tmp_pos;
	
	const uint8_t *letter_bitmap;
	int8_t letter_width;
	int8_t letter_height;
	uint8_t step;
	
	uint16_t font_color = text->font_color << 8 | text->font_color >> 8;
	uint16_t bg_color = text->bg_color << 8 | text->bg_color >> 8;
	
	for (char *peek = text->str; *peek != '\0'; ++peek) {

		letter_bitmap = get_bitmap(*peek, text->font_size, (uint8_t *) &letter_width, (uint8_t *) &letter_height);
		
		step = LETTER_TEXTURE_BUFFER_SIZE / letter_width;
		
		tmp_pos.y = pos.y;
		tmp_pos.x = pos.x;
		
		letter_texture.width = letter_width;
		letter_texture.height = step;
		
		while ((letter_height -= step) > 0) {
			
			for (int i = 0; i < step; ++i) {
				int32_t j = 0;
				for (j = 0; j < letter_width - 8; j += 8) {
					__fill_font_texture_frame(&letter_texture_buffer[curr_buffer_id][i * letter_width + j], *letter_bitmap++, 8, font_color, bg_color);
				}
				__fill_font_texture_frame(&letter_texture_buffer[curr_buffer_id][i * letter_width + j], *letter_bitmap++, letter_width - j, font_color, bg_color);
			}
			
			_draw_texture(&letter_texture, tmp_pos);
				
			tmp_pos.y += step;
			curr_buffer_id ^= 0x01;
			letter_texture.data = letter_texture_buffer[curr_buffer_id];
		}
		
		for (int i = letter_height + step; i > 0; --i) {
			int32_t j = 0;
			for (j = 0; j < letter_width - 8; j += 8) {
				__fill_font_texture_frame(&letter_texture_buffer[curr_buffer_id][i * letter_width + j], *letter_bitmap++, 8, font_color, bg_color);
			}
			__fill_font_texture_frame(&letter_texture_buffer[curr_buffer_id][i * letter_width + j], *letter_bitmap++, letter_width - j, font_color, bg_color);
		}
		
		letter_texture.height = letter_height + step;
		_draw_texture(&letter_texture, tmp_pos);
				

		curr_buffer_id ^= 0x01;
		letter_texture.data = letter_texture_buffer[curr_buffer_id];
		
		pos.x += letter_width;
	}
}

static inline void __fill_font_texture_frame(uint16_t *buf, uint8_t mask, uint8_t n, uint16_t font_color, uint16_t bg_color)
{
	uint8_t bit_field_mask = 0x80;
	while (n-- > 0) {
		*buf++ = (mask & bit_field_mask) ? font_color : bg_color;
		bit_field_mask >>= 1;
	}
}
