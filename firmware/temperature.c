#include "temperature.h"


static float __temp_tab[8] = {
	25 , 81 , 117 , 166 , 202 , 252 , 450 , 450
};

static int __calib_index = 0;

static float __temp_k = (float) (sizeof(__temp_tab) / sizeof(__temp_tab[0]) - 1) / 1800.0;

float get_temperature_by_adc_value(float adc_val)
{
	uint32_t i = adc_val * __temp_k;
	
	if (i >= sizeof(__temp_tab) / sizeof(__temp_tab[0]) - 1) {
		return MAX_TEMPERATURE;
	}
	
	float frac = (float) adc_val * __temp_k - (float) i;
	float greatest_le = __temp_tab[i];
	float lowest_ge = __temp_tab[i + 1];
	return greatest_le + (lowest_ge - greatest_le) * frac;
}

void init_temperature_calibration()
{
	extern float required_adc_val;
	required_adc_val = 1;
	__calib_index = 0;
}

int push_calibration_value(float temp)
{
	extern float required_adc_val;
	if (__calib_index > sizeof(__temp_tab) / sizeof(__temp_tab[0])) {
		return CAL_NOT_INITIALIZED;
	}
	
	__temp_tab[__calib_index] = temp;
	
	if (temp >= MAX_TEMPERATURE) {
		return CAL_FINISHED;
	}
	
	if (++__calib_index == sizeof(__temp_tab) / sizeof(__temp_tab[0])) {
		return CAL_FINISHED;
	}
	
	required_adc_val = (float) (__calib_index) / __temp_k;
	return CAL_WAITING_NEXT;
}
