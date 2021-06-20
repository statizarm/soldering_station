#ifndef TEMPERATURE_H_
#define TEMPERATURE_H_

#include <stdint.h>

float get_temperature_by_adc_value(float adc_val);
void init_temperature_calibration();

enum calibration_status {
	CAL_FINISHED = 1,
	CAL_WAITING_NEXT,
	CAL_NOT_INITIALIZED
};

int push_calibration_value(float temp);

#define MAX_TEMPERATURE 350
#define MAX_SETABLE_TEMPERATURE 300

#endif // TEMPERATURE_H_