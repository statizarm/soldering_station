#include "stm32f10x.h"
#include "task.h"

static void init_led(void);
static void init_spi2(void);
static void init_dma(void);
static void init_disp_io(void);

static void init_enc(void);
static void init_term_adc(void);
static void init_heater_pwm(void);
static void init_disp_pwm(void);
static void init_systick(void);

static void init_control_btns(void);

void delay(uint32_t clocks);

void init(void)
{
	init_heater_pwm();
	init_control_btns();
	
	init_term_adc();
	
	init_enc();
	
	// For firmware debug
	init_led();

	// Initialise display
	init_dma();
	init_spi2();
	init_disp_io();
	init_disp_pwm();
	
	init_systick();
}


static void init_control_btns(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	
	GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_CNF14 | GPIO_CRH_CNF15);
	GPIOC->CRH |= GPIO_CRH_CNF13_1 | GPIO_CRH_CNF14_1 | GPIO_CRH_CNF15_1;
	
	GPIOC->BSRR |= GPIO_BSRR_BS13 | GPIO_BSRR_BS14 | GPIO_BSRR_BS15;
}

static void init_systick()
{
	SysTick->CTRL |= SysTick_CTRL_TICKINT; // Разрешение прерывания по переполнению
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE; // Выбор системной частоты в качестве источника тактирования для системного таймера

	// Желаемая частота - 24kHz => LOAD = 72MHz / 24kHz = 3000
	SysTick->LOAD = 3000;
	
	SysTick->VAL = 1; // Обнуляем записью любого значения
	
	NVIC_SetPriority(SysTick_IRQn, 1);
}

static void init_disp_pwm()
{
	// led drived by TIM1_CH2 - PA9
	
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_IOPAEN;
	
	GPIOA->CRH &= ~(GPIO_CRH_CNF9);
	GPIOA->CRH |= GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9_1; // Alt func push-pull 2MHz
	
	// freq = 36 kHz => 72 000 000 / 20 / 100 = 36kHz
	TIM1->PSC = 19;
	TIM1->ARR = 100;
	
	TIM1->CCR2 = 100;

	TIM1->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
	TIM1->CCMR1 |= TIM_CCMR1_OC2PE;
	
	TIM1->CCER |= TIM_CCER_CC2E;
	
	TIM1->BDTR |= TIM_BDTR_MOE;
	
	TIM1->CR1 |= TIM_CR1_ARPE;
	TIM1->CR1 |= TIM_CR1_CEN;
}

static void init_heater_pwm()
{
	// INIT TIM3_CH4 PWM
	// TIM3_CH4 - PB1
	// TIM3_CH1 - PA6
	// TIM3_CH2 - for adc trigger
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPAEN;
	
	GPIOB->CRL &= ~(GPIO_CRL_CNF1);
	GPIOB->CRL |= GPIO_CRL_CNF1_1 | GPIO_CRL_MODE1_1; // Alternate function push-pull 2MHz
	GPIOA->CRL &= ~(GPIO_CRL_CNF6);
	GPIOA->CRL |= GPIO_CRL_CNF6_1 | GPIO_CRL_MODE6_1; // Alt func push-pull 2MHz

	// freq = 60 => 36 000 000 / 300 = 120 000 / 2000
	TIM3->PSC = 299;
	TIM3->ARR = 2000;
	
	TIM3->CCR4 = 1;
	TIM3->CCR2 = 1980;
	
	TIM3->CCMR2 |= TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2;
	TIM3->CCMR2 |= TIM_CCMR2_OC4PE;
	
	TIM3->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
	TIM3->CCMR1 |= TIM_CCMR1_OC1PE;
	
	TIM3->DIER |= TIM_DIER_CC2IE;
	TIM3->CCER |= TIM_CCER_CC4E | TIM_CCER_CC2E;
	
	TIM3->CR1 |= TIM_CR1_ARPE;
	TIM3->CR1 |= TIM_CR1_CEN;
	
	NVIC_EnableIRQ(TIM3_IRQn);
}

static void init_term_adc()
{
	// INIT ADC12
	// PB0 - IN8
	// PA5 - IN5
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
	
	GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
	
	GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
	
	ADC1->SMPR2 |= ADC_SMPR2_SMP5 | ADC_SMPR2_SMP8 ;
	
	ADC1->CR2 |= ADC_CR2_EXTTRIG; // EXTERNAL triger select
	ADC1->CR2 |= ADC_CR2_EXTSEL; // SWSTART triger to start conv
	ADC1->CR1 |= ADC_CR1_EOCIE; // EOC interrupt request enable
	
	ADC1->SQR1 &= ~ADC_SQR1_L;
	ADC1->SQR3 |= ADC_SQR3_SQ1_3; // IN8 id into SQR3_SQ1
	
	ADC1->CR2 |= ADC_CR2_ADON;
	delay(10);
	ADC1->CR2 |= ADC_CR2_CAL;
	
	while(ADC1->CR2 & ADC_CR2_CAL)
		;
}

static void init_enc()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

	GPIOA->CRL &= ~(GPIO_CRL_CNF2 | GPIO_CRL_MODE2);
	GPIOA->CRL |= GPIO_CRL_CNF2_1;
	
	GPIOA->BSRR |= GPIO_BSRR_BS2;
	
	AFIO->EXTICR[0] &= ~AFIO_EXTICR1_EXTI2;
	
	EXTI->IMR |= EXTI_IMR_MR2;
	EXTI->FTSR |= EXTI_FTSR_TR2;
	
	NVIC_EnableIRQ(EXTI2_IRQn);
	NVIC_SetPriority(EXTI2_IRQn, 1);
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	
	GPIOA->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_CNF0);
	GPIOA->CRL |= GPIO_CRL_CNF1_1 | GPIO_CRL_CNF0_1;
	GPIOA->BSRR |= GPIO_BSRR_BS1 | GPIO_BSRR_BS0;
	
	TIM2->ARR = 50;
	TIM2->PSC = 0;
	
	TIM2->CCMR1 |= TIM_CCMR1_IC1F | TIM_CCMR1_IC2F;
	TIM2->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;
	TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
	TIM2->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;
	
	TIM2->DIER |= TIM_DIER_UIE;
	
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 2);
	
	TIM2->CR1 |= TIM_CR1_CEN;
	TIM2->CNT = 0;
}

static void init_disp_io(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	
	GPIOB->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_CNF11);
	GPIOB->CRH |= GPIO_CRH_MODE10;
	GPIOB->CRH |= GPIO_CRH_MODE11;
	
	GPIOB->BSRR |= GPIO_BSRR_BR10 | GPIO_BSRR_BR11; // Set to low logic level reset and cs pins
}
	
static void init_led(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	
	GPIOC->CRH &= ~(GPIO_CRH_CNF8);
	GPIOC->CRH |= GPIO_CRH_MODE8;
	
	GPIOC->ODR |= GPIO_ODR_ODR8;
}

static void init_dma(void)
{
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	
	//DMA for spi2
	DMA1_Channel5->CCR |= DMA_CCR5_TCIE; // Interupt on transmition complete
	DMA1_Channel5->CCR |= DMA_CCR5_PL; // Highest priority

	NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	NVIC_SetPriority(DMA1_Channel5_IRQn, 0);
}

static void init_spi2(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	// NSS - PB12
	// SCK - PB13
	// MISO - PB14
	// MOSI - PB15
	GPIOB->CRH &= ~(GPIO_CRH_CNF12
		| GPIO_CRH_CNF13
		| GPIO_CRH_CNF14
		| GPIO_CRH_CNF15
		| GPIO_CRH_MODE15
		| GPIO_CRH_MODE12
		| GPIO_CRH_MODE13);
	
	GPIOB->CRH |= GPIO_CRH_MODE12; // push-pull
	GPIOB->CRH |= GPIO_CRH_CNF13_1 | GPIO_CRH_MODE13; // alt func push-pull
	GPIOB->CRH |= GPIO_CRH_CNF14_1; // pull-up
	GPIOB->BSRR |= GPIO_BSRR_BS14;
	GPIOB->CRH |= GPIO_CRH_CNF15_1 | GPIO_CRH_MODE15; // alt func push-pull
	
	//SPI2->CR2 |= SPI_CR2_TXDMAEN;
	SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
	SPI2->CR1 |= SPI_CR1_MSTR;
	SPI2->CR1 |= SPI_CR1_SPE;
}
