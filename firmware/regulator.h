#ifndef REGULATOR_H_
#define REGULATOR_H_

#include "stm32f10x.h"

struct regulator {
	TIM_TypeDef *tim;
	volatile uint16_t *ccr;
	volatile const uint16_t *max;
	float errors[3];
	float k_p;
	float k_i;
	float k_d;
	float prev;
	float err_sum;
	uint8_t iter;
};

void regulate(struct regulator *r, float error);

#endif // REGULATOR_H_
