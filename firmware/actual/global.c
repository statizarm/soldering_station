#include <stddef.h>
#include <stdint.h>
#include "regulator.h"

float current_temperature = 0.0f;
float prev_volt = 0.0f;

float required_temperature = 130.0f;

struct regulator temperature_regulator = {
	.tim = TIM3,
	.ccr = &(TIM3->CCR4),
	.max = &(TIM3->CCR2),
	.enable_reg = &(GPIOC->IDR),
	.enable_mask = GPIO_IDR_IDR15 | GPIO_IDR_IDR14,
	.k_p = 0.2,
	.k_d = 0.2,
	.k_i = 0,
	.errors = {0, 0, 0},
	.prev = 0,
	.iter = 0,
	.err_sum = 0
};

char cmd_buffer[256] = {0};

uint16_t disp_brightness = 0;

float required_adc_val = 1; // for temperature calibration
