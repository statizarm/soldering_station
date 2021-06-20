#include "stm32f10x.h"
#include "ui.h"
#include "ili9341_driver.h"
#include "task.h"
#include "frame.h"
#include <string.h>
#include <stdio.h>
#include "application.h"

// Display controller - ili9341

// TODO: add menu

void delay(uint32_t);

void init(void);

void write_spi(char *str);
void write_byte_spi(char byte);
char read_byte_spi(void);

void usart_read_line(char *buf);
void usart_write_byte(char byte);
void usart_write_line(char *buf);

int main(void)

{
	init_app();
	
	start_app();
}

void delay(uint32_t clocks)
{
	while(clocks > 0) {
		--clocks;
	}
}

void EXTI2_IRQHandler(void)
{
	EXTI->PR |= EXTI_PR_PR0;
	
	add_task(HANDLE_BTN_TASK_ID, 2);
}

void TIM3_IRQHandler()
{
	TIM3->SR &= ~TIM_SR_CC2IF;
	
	GPIOC->ODR ^= GPIO_ODR_ODR8;
		
	ADC1->CR2 |= ADC_CR2_SWSTART;
}

void ADC1_2_IRQHandler()
{
	ADC1->SR &= ~ADC_SR_EOC;
		
	add_task(FILTER_ADC_TASK_ID, 0);
}

void SysTick_Handler(void)
{
	// Частота возникающих прерываний - 24 KHz
	// Частота контроля режима нагрева - 12000Hz => anim_count_divisor = 24 KHz / 12 KHz = 2
	// Частота обновления температуры на дисплее - 10Hz => temp_count_divisor = 24KHz / 10 = 2400
	// Частота обновления анимации - 60Hz => anim_count_divisor = 24 KHz / 60 Hz = 400
	// Частота запросов на преобразование ADC - 24KHz
	// возможно что-то еще...
	static uint32_t count = 0;
	
	++count;
	
	if (count % 24000 == 0) {
		count = 0;
	}
	
	if (count % 1600 == 0) {
		add_task(REDRAW_ANIMATION_TASK_ID, 5);
	}
}

void TIM2_IRQHandler()
{
	TIM2->SR &= ~TIM_SR_UIF;
	
	add_task(HANDLE_ENC_TASK_ID, 3);
}
