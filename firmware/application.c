#include "application.h"
#include "ui.h"
#include "task.h"
#include "stm32f10x.h"
#include "global.h"
#include <stdlib.h>

static uint16_t __tmp_heater_ccr;
static uint8_t is_running = 0;

void init(void);

static void _enable_listeners(void);
static void _disable_listeners(void);
static void _flush_tasks(void);

static void _off_heater(void);
static void _off_display(void);

static void _on_heater(void);
static void _on_display(void);

void init_app()
{
	init();
	DMA1->IFCR |= DMA_IFCR_CGIF5;
	init_ui();
}

void start_app()
{
	task_func_t task_func;

	_enable_listeners();
	is_running = 1;
		
	_on_display();
	while (1) {
		if (!is_running) {
			continue;
		}
		if ((task_func = get_task()) != NULL) {
			task_func();
		}
	}
}

void sleep_app()
{
	is_running = 0;

	_flush_tasks();
	_off_heater();
	_off_display();
	_disable_listeners();
}

static void _enable_listeners(void)
{
	SysTick->CTRL |= SysTick_CTRL_ENABLE;
	
	NVIC_EnableIRQ(ADC1_IRQn);
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_EnableIRQ(TIM2_IRQn);
}

static void _disable_listeners(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE;
	
	NVIC_DisableIRQ(ADC1_IRQn);
	NVIC_DisableIRQ(EXTI2_IRQn);
	NVIC_DisableIRQ(TIM2_IRQn);
	
	NVIC_ClearPendingIRQ(ADC1_IRQn);
	NVIC_ClearPendingIRQ(EXTI2_IRQn);
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	NVIC_ClearPendingIRQ(SysTick_IRQn);
}

void unsleep_app(void)
{
	is_running = 1;

	_enable_listeners();
	_on_display();
	_on_heater();
}

static void _flush_tasks(void)
{
	task_func_t f;
	
	while ((f = get_task()) != NULL) {
		f();
	}
}

static void _off_display(void)
{
	TIM1->CCR2 = 0;
}

static void _off_heater(void)
{
	__tmp_heater_ccr = TIM4->CCR1;
	
	TIM4->CCR1 = 0;
}

static void _on_display(void)
{
	TIM1->CCR2 = disp_brightness;
}

static void _on_heater(void)
{
	TIM4->CCR1 = __tmp_heater_ccr;
}
