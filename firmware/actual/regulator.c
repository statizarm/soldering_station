#include "regulator.h"
#define MAX_FACTOR 0.5

static void _pid_regulate(struct regulator *r, float mult);

void regulate(struct regulator *r, float error)
{
	_pid_regulate(r, error);
}

static void _pid_regulate(struct regulator *r, float err)
{

	float res;
	switch (r->iter) {
		case 0:
			r->errors[1] = err;
			res = err * r->k_p;
			++r->iter;
			break;
		case 1:
			r->errors[2] = err;
			res = r->prev + r->k_p * (err - r->errors[1]) + r->k_d * (err - r->errors[1]);
			++r->iter;
			break;
		case 2:
			r->errors[0] = r->errors[1];
			r->errors[1] = r->errors[2];
			r->errors[2] = err;
		
			res = r->prev + r->k_p * (err - r->errors[1]) + r->k_d * (err - 2 * r->errors[1] + r->errors[0]);
			break;
		default:
			r->iter = 0;
	}
	r->err_sum += err * r->k_i;
	
	if (r->err_sum > 0) {
		res += r->err_sum;
	}
	
	r->prev = res;
	
	uint16_t max = (float) *(r->max) * MAX_FACTOR;
	if (*r->enable_reg & r->enable_mask) {
		res = 0;
	} else {
		res = (float) *(r->ccr) + res * (float) r->tim->ARR;
	}
	if (res < 0) {
		res = 0;
	} else if (res > max) {
		res = max;
	}
	
	*(r->ccr) = (uint16_t) res;
}
