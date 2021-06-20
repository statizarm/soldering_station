#ifndef TASK_H_
#define TASK_H_

#include <stdint.h>

// Нулевой id для NULL
enum {
	HANDLE_BTN_TASK_ID = 1, HANDLE_ENC_TASK_ID, FILTER_ADC_TASK_ID,
	REDRAW_ANIMATION_TASK_ID,
// -----------------------------------------------------------------------//
};

typedef void (*task_func_t)(void);

void add_task(uint8_t task_id, uint8_t priority);
task_func_t get_task(void);

#define ADC_VALUES_BUFFER_SIZE 32

#endif // TASK_H_
