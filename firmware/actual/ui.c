#include "ui.h"
#include "frame.h"
#include "global.h"
#include "string.h"
#include "temperature.h"

static uint8_t __state = 0;
static uint8_t __digit = 2;
static float __temp_value = 110;

typedef uint8_t (*action_t)(void);

uint8_t _start_temp_setting(void);
uint8_t _select_calib_btn(void);
uint8_t _anim_next_frame(void);
uint8_t _next_digit(void);
uint8_t _dec_digit(void);
uint8_t _inc_digit(void);
uint8_t _dummy_action(void);
uint8_t _start_calib(void);
uint8_t _unselect_calib_btn(void);

static struct {
	uint8_t next;
	action_t action;
} __trans_table[][4] = {
	{
		{
			.next = 1,
			.action = _start_temp_setting
		},
		{
			.next = 2,
			.action = _select_calib_btn
		},
		{
			.next = 2,
			.action = _select_calib_btn
		},
		{
			.next = 0,
			.action = _anim_next_frame
		}
	},
	{
		{
			.next = 0xFF,
			.action = _next_digit
		},
		{
			.next = 1,
			.action = _dec_digit
		},
		{
			.next = 1,
			.action = _inc_digit
		},
		{
			.next = 1,
			.action = _dummy_action
		}
	},
	{
		{
			.next = 1,
			.action = _start_calib
		},
		{
			.next = 0,
			.action = _unselect_calib_btn
		},
		{
			.next = 0,
			.action = _unselect_calib_btn
		},
		{
			.next = 2,
			.action = _anim_next_frame
		}
	}
};

static frame_t __frames[] = {
	{
		.polygon = {
			.width = 160,
			.height = 240,
			.color = 0x0000
		},
		.type = POLYGON
	},
	{
		.polygon = {
			.width = 160,
			.height = 240,
			.color = 0x0000
		},
		.type = POLYGON
	},
	{
		.polygon = {
			.width = 160,
			.height = 3,
			.color = 0xFFFF
		},
		.type = POLYGON
	},
	{
		.polygon = {
			.width = 3,
			.height = 28,
			.color = 0xFFFF
		},
		.type = POLYGON
	},
	{
		.polygon = {
			.width = 157,
			.height = 3,
			.color = 0xFFFF
		},
		.type = POLYGON
	},
	{
		.polygon = {
			.width = 3,
			.height = 25,
			.color = 0xFFFF
		},
		.type = POLYGON
	},
	{
		.polygon = {
			.width = 154,
			.height = 25,
			.color = 0x0000
		},
		.type = POLYGON
	},
	{
		.text = {
			.str = "calibration",
			.font_color = 0xFFFF,
			.bg_color = 0x0000,
			.font_size = 24
		},
		.type = TEXT
	},
	{
		.text = {
			.str = "cal running",
			.font_color = 0xFFFF,
			.bg_color = 0x000,
			.font_size = 24
		},
		.type = TEXT
	},
	{
		.polygon = {
			.width = 29,
			.height = 3,
			.color = 0xFFFF
		},
		.type = POLYGON
	},
	{
		.text = {
			.str = "o",
			.font_color = 0xFFFF,
			.bg_color = 0x000,
			.font_size = 24
		},
		.type = TEXT
	},
	{
		.text = {
			.str = "C",
			.font_color = 0xFFFF,
			.bg_color = 0x000,
			.font_size = 64
		},
		.type = TEXT
	}
};

static position_t __frame_positions[] = {
	{
		.x = 0,
		.y = 0
	},
	{
		.x = 160,
		.y = 0
	},
	{
		.x = 0,
		.y = 209
	},
	{
		.x = 0,
		.y = 212
	},
	{
		.x = 3,
		.y = 237
	},
	{
		.x = 157,
		.y = 212
	},
	{
		.x = 3,
		.y = 212
	},
	{
		.x = 19,
		.y = 213
	},
	{
		.x = 19,
		.y = 213
	},
	{
		.x = 7,
		.y = 127
	},
	{
		.x = 112,
		.y = 63
	},
	{
		.x = 123,
		.y = 63
	}
};

static char __anim_temp_buffer[8] = {"025"};
static frame_t __first_heater_status = {
	.text = {
		.str = "OFF",
		.font_color = 0xFFFF,
		.bg_color = 0x0000,
		.font_size = 24
	},
	.type = TEXT
};

static frame_t __second_heater_status = {
	.text = {
		.str = "OFF",
		.font_color = 0xFFFF,
		.bg_color = 0x0000,
		.font_size = 24
	},
	.type = TEXT
};

static frame_t __anim_frame = {
	.text = {
		.str = __anim_temp_buffer,
		.font_color = 0xFFFF,
		.bg_color = 0x0000,
		.font_size = 64
	},
	.type = TEXT
};

static position_t __first_heater_status_position = {
	.x = 4,
	.y = 4
};

static position_t __second_heater_status_position = {
	.x = 283,
	.y = 4
};

static position_t __anim_frame_pos = {
	.x = 7,
	.y = 63
};

void delay(uint32_t clocks);

void init_ui()
{
	init_graphics();
	for (int i = 0; i < 8; ++i) {
		draw_frame(&__frames[i], __frame_positions[i]);
	}
	
	draw_frame(&__anim_frame, __anim_frame_pos);
	draw_frame(&__frames[10], __frame_positions[10]);
	draw_frame(&__frames[11], __frame_positions[11]);
	draw_frame(&__first_heater_status, __first_heater_status_position);
	
	position_t tmp;
	for (int i = 0; i < 8; ++i) {
		tmp = __frame_positions[i];
		tmp.x += 160;
		draw_frame(&__frames[i], tmp);
	}
	
	tmp = __anim_frame_pos;
	tmp.x += 160;
	draw_frame(&__anim_frame, tmp);
	
	tmp =  __frame_positions[10];
	tmp.x += 160;
	draw_frame(&__frames[10], tmp);
	
	tmp =  __frame_positions[11];
	tmp.x += 160;
	draw_frame(&__frames[11], tmp);
	
	draw_frame(&__second_heater_status, __second_heater_status_position);
	
	__state = 0;
}

void ui_next(uint32_t event)
{
	uint8_t tmp_state;
	if (event >= DUMMY_EVENT) {
		return;
	}
	
	tmp_state = __trans_table[__state][event].action();
	if ((__state = __trans_table[__state][event].next) == 0xFF) {
		__state = tmp_state;
	}
}

void ftoa(float val, char *dst, uint32_t prec);
void __print_number_to_anim_buffer(float val, uint32_t prec);

uint8_t _start_temp_setting(void)
{
	__temp_value = required_temperature;
	__print_number_to_anim_buffer(__temp_value, 0);
	
	draw_frame(&__anim_frame, __anim_frame_pos);
	
	__digit = 2;
	
	draw_frame(&__frames[9], __frame_positions[9]);
	
	return 0xFF;
}

uint8_t _select_calib_btn(void)
{
	frame_t *f = &__frames[2];
	position_t *fp = &__frame_positions[2];
	while (f != &__frames[6]) {
		f->polygon.color = 0x9219;
		
		draw_frame(f, *fp);
		
		++f;
		++fp;
	}
	__frames[7].text.font_color = 0x9219;
	draw_frame(&__frames[7], __frame_positions[7]);
	return 0xFF;
}

uint8_t _anim_next_frame(void)
{
	__print_number_to_anim_buffer(current_temperature, 0);
	
	draw_frame(&__anim_frame, __anim_frame_pos);
	if (GPIOC->IDR & GPIO_IDR_IDR15 || GPIOC->IDR & GPIO_IDR_IDR14) {
		__first_heater_status.text.str = "OFF";
	} else {
		__first_heater_status.text.str = "ON ";
	}
	draw_frame(&__first_heater_status, __first_heater_status_position);
	
	if (GPIOC->IDR & GPIO_IDR_IDR13 || GPIOC->IDR & GPIO_IDR_IDR14) {
		__second_heater_status.text.str = "OFF";
	} else {
		__second_heater_status.text.str = "ON ";
	}
	
	draw_frame(&__second_heater_status, __second_heater_status_position);
	return 0xFF;
}

uint8_t _next_digit(void)
{
	__frames[9].polygon.color = 0x0000;
	draw_frame(&__frames[9], __frame_positions[9]);
	__frames[9].polygon.color = 0xFFFF;
	
	if (__digit != 0) {
		__frame_positions[9].x += __frames[9].polygon.width;
		draw_frame(&__frames[9], __frame_positions[9]);
		
		--__digit;
		return 1;
	} else {
		uint32_t res = push_calibration_value(__temp_value);
		__digit = 2;
		__frame_positions[9].x -= 2 * __frames[9].polygon.width;
		
		if (res == CAL_WAITING_NEXT) {
			draw_frame(&__frames[9], __frame_positions[9]);
			__temp_value = get_temperature_by_adc_value(required_adc_val);
			__print_number_to_anim_buffer(__temp_value, 0);
			draw_frame(&__anim_frame, __anim_frame_pos);
			return 1;
		}
		
		if (res == CAL_NOT_INITIALIZED) {
			required_temperature = __temp_value;
		} else {
			draw_frame(&__frames[6], __frame_positions[6]);
			draw_frame(&__frames[7], __frame_positions[7]);
		}
	}
	
	return 0;
}

uint8_t _dec_digit(void)
{
	switch(__digit) {
		case 2:
			__temp_value -= 100;
			break;
		case 1:
			__temp_value -= 10;
			break;
		case 0:
			__temp_value -= 1;
			break;
	}
	
	if (__temp_value < 0) {
		__temp_value = 0;
	}
	
	__print_number_to_anim_buffer(__temp_value, 0);
	draw_frame(&__anim_frame, __anim_frame_pos);
	return 0xFF;
}

uint8_t _inc_digit(void)
{
	switch(__digit) {
		case 2:
			__temp_value += 100;
			break;
		case 1:
			__temp_value += 10;
			break;
		case 0:
			__temp_value += 1;
			break;
	}
	
	if (__temp_value > MAX_SETABLE_TEMPERATURE) {
		__temp_value = MAX_SETABLE_TEMPERATURE;
	}
	
	__print_number_to_anim_buffer(__temp_value, 0);
	draw_frame(&__anim_frame, __anim_frame_pos);
	return 0xFF;
}

uint8_t _dummy_action(void)
{
	return 0xFF;
}

uint8_t _start_calib(void)
{
	_unselect_calib_btn();
	init_temperature_calibration();
	
	draw_frame(&__frames[6], __frame_positions[6]);
	
	__temp_value = get_temperature_by_adc_value(required_adc_val);
	__digit = 2;
	__print_number_to_anim_buffer(__temp_value, 0);
	
	draw_frame(&__frames[8], __frame_positions[8]);
	draw_frame(&__frames[9], __frame_positions[9]);
	
	draw_frame(&__anim_frame, __anim_frame_pos);
	return 0xFF;
}

uint8_t _unselect_calib_btn(void)
{
	frame_t *f = &__frames[2];
	position_t *fp = &__frame_positions[2];
	while (f != &__frames[6]) {
		f->polygon.color = 0xFFFF;
		
		draw_frame(f, *fp);
		
		++f;
		++fp;
	}
	__frames[7].text.font_color = 0xFFFF;
	draw_frame(&__frames[7], __frame_positions[7]);
	
	return 0xFF;
}

void __print_number_to_anim_buffer(float val, uint32_t prec)
{
	char buf[6];
	char *p_buf = buf;
	
	// draw_frame(&__frames[8], __frame_positions[8]);
	
	ftoa(val, p_buf, prec);
	
	for (int j = 0; j < 3 - strlen(buf); ++j) {
		__anim_temp_buffer[j] = '0';
	}
	
	for (int i = 3 - strlen(buf); *p_buf != '\0'; ++i, ++p_buf) {
		__anim_temp_buffer[i] = *p_buf;
	}
}
