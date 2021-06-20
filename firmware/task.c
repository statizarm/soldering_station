#include "task.h"
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h"
#include "ui.h"
#include "global.h"
#include "math.h"
#include "temperature.h"

#define MAX_DELTA 0.1

struct __queue_chunk {
		uint8_t task_id;
		uint8_t priority;
}; 

struct _task_queue {
	struct __queue_chunk queue_mem[256];
	struct __queue_chunk *buf_top; // буфер это память в которую складываются таски прежде чем попасть в очередь
	struct __queue_chunk *first; // указатель на первый элемент в очереди
};

uint16_t adc_values[ADC_VALUES_BUFFER_SIZE] = {0};

static void handle_btn_task(void);
static void handle_enc_task(void);
static void redraw_animation_task(void);
static void filter_adc_task(void);
// ---------------------------------------//
static void handle_cmd_task(void);
static void print_current_temp_task(void);

static struct _task_queue _task_queue = {
	.queue_mem = {0},
	.buf_top = (struct __queue_chunk *) &_task_queue.buf_top - 1,
	.first = (struct __queue_chunk *) &_task_queue.buf_top - 1
};

// Порядок файлов см в заголовочном файле
static task_func_t _task_handlers[] = {
	NULL,
	handle_btn_task,
	handle_enc_task,
	filter_adc_task,
	redraw_animation_task,
// --------------------------- //
	handle_cmd_task,
	print_current_temp_task
};

static void __min_heapify_to_child(struct __queue_chunk *heap, uint32_t parent_id, uint32_t len) {
	uint32_t child_id;
	uint32_t tmp_id;
	struct __queue_chunk chunk = *heap;
	
	while ((child_id = parent_id * 2 + 1) < len) {
		tmp_id = child_id + 1;
		
		if (tmp_id < len && heap[tmp_id].priority < heap[child_id].priority) {
			child_id = tmp_id;
		}
		
		if (heap[child_id].priority > chunk.priority) {
			break;
		}
		
		heap[parent_id] = heap[child_id];
		parent_id = child_id;
	}
	
	heap[parent_id] = chunk;
}	

static uint8_t _dequeue(void)
{
	struct __queue_chunk *bt;
	struct __queue_chunk tmp;
	uint8_t task_id = (++_task_queue.first)->task_id;
	
	do {
		bt = (struct __queue_chunk *) __ldrex(&_task_queue.buf_top);
		tmp = bt[1];
	} while (__strex((unsigned int) (bt + 1), &_task_queue.buf_top));
	
	*_task_queue.first = tmp;
	
	return task_id;
}

static void _queue_flush_buf(void)
{
	struct __queue_chunk *bt;

	do {
		bt = (struct __queue_chunk *) __ldrex(&_task_queue.buf_top);
		while (_task_queue.first != bt) {
				
			__min_heapify_to_child(_task_queue.first, 0,
				(struct __queue_chunk *) &_task_queue.buf_top - _task_queue.first);
			--_task_queue.first;
		}
		
	} while (__strex((unsigned int) bt, &_task_queue.buf_top));
}

static void _queue_buf_push(uint8_t task_id, uint8_t priority)
{
	struct __queue_chunk *bt;
	
	do {
		bt = (struct __queue_chunk *) __ldrex(&_task_queue.buf_top);
	} while (__strex((unsigned int) (bt - 1), &_task_queue.buf_top)); 
	
	bt->task_id = task_id;
	bt->priority = priority;
}

task_func_t get_task(void) {
	_queue_flush_buf();
	
	if (_task_queue.first == (struct __queue_chunk *) &_task_queue.buf_top - 1) {
		return NULL;
	} else {
		return _task_handlers[_dequeue()];
	}
}

void add_task(uint8_t task_id, uint8_t priority) {
	if (_task_queue.buf_top == _task_queue.queue_mem) {
		_task_handlers[task_id]();
	} else {
		_queue_buf_push(task_id, priority);
	}
}

void handle_btn_task(void)
{
	uint32_t *bit_band = (uint32_t *) (PERIPH_BB_BASE + ((uint32_t) &GPIOA->IDR - PERIPH_BASE) * 32 + 8);
	uint8_t nzeros = 0;
	
	for (int i = 0; i < 128; ++i) {
		if (*bit_band == 0) {
			++nzeros;
		}
	}
	
	if (nzeros > 64) {
		ui_next(BTN_PRESSED);
	}
}

void handle_enc_task(void)
{
	uint16_t prev = TIM2->CNT;
	uint16_t curr;

	while ((curr = TIM2->CNT) == prev) {}

	if (curr < prev) {
		ui_next(ENC_LEFT);
	} else {
		ui_next(ENC_RIGHT);
	}
	
	TIM2->CNT = TIM2->ARR >> 1;
}

#define K ((float) 24)

static int cal_running = 0;

void filter_adc_task(void)
{
	static float dacc = 0;
	static float dout = 0;
	uint16_t adc_val = ADC1->DR;

	dacc = dacc + adc_val - dout;
	dout = dacc / K;
	
	if (!cal_running) {
		current_temperature = get_temperature_by_adc_value(dout);
	
		regulate(&temperature_regulator, 1 - current_temperature / required_temperature);
	} else {
		regulate(&temperature_regulator, 1 - dout / required_adc_val);
	}
}

#undef K

static void redraw_animation_task(void)
{
	ui_next(ANIM_NEXT_FRAME);
}

// ----------------------- DEV SUPPORT ----------------------------------------- //

static void handle_cmd_task(void)
{
	char *ptr;
	float val;
	if (ptr = strstr(cmd_buffer, "SET_TEMP")) {
		val = atof(ptr + 8);
		required_temperature = val;
	} else if (ptr = strstr(cmd_buffer, "START_CAL")) {
		cal_running = 1;
		init_temperature_calibration();
	} else if (ptr = strstr(cmd_buffer, "CAL_VALUE")) {
		val = atof(ptr + 9);
		if (push_calibration_value(val) == CAL_FINISHED) {
			cal_running = 0;
		}
	}
}

void usart_write_line(char *s);

void reverse(char *beg, char *end)
{
	for (char tmp; beg < --end; ++beg) {
		tmp = *beg;
		*beg = *end;
		*end = tmp;
	}
}

void ftoa(float val, char *dst, uint32_t prec)
{
	int32_t whole = val;
	float fractional = val - whole;
	
	if (whole < 0) {
		*dst++ = '-';
		whole = - whole;
	}
	
	char *to_rev = dst;
	do {
		*dst++ = whole % 10 + '0';
	} while ((whole /= 10) != 0);
	
	reverse(to_rev, dst);
	
	if (prec > 0) {
		
		*dst++ = '.';
		
		do {
			fractional *= 10;
			whole = fractional;
			
			*dst++ = whole + '0';
			
			fractional -= whole;
			
		} while (--prec >= 1);
	}
	*dst = '\0';
}

static void print_current_temp_task(void)
{
	char buf[20];
	
	if (!cal_running) {
		ftoa(current_temperature, buf, 8);
		usart_write_line(buf);
	}
}
