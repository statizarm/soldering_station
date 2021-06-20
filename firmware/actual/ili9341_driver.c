#include "ili9341_driver.h"
#include <stddef.h>

typedef struct {
	uint8_t cmd;
	uint8_t argc;
	uint8_t argv[];
} _cmd_t;

static ili9341_driver_t *_driver_in_use = NULL;

uint8_t init_cmds[] = {
	0xEF, 3, 0x03, 0x80, 0x02,
	0xCF, 3, 0x00, 0xC1, 0x30,
	0xED, 4, 0x64, 0x03, 0x12, 0x81,
	0xE8, 3, 0x85, 0x00, 0x78,
	0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
	0xF7, 1, 0x20,
	0xEA, 2, 0x00, 0x00,
	0xC0, 1, 0x23,             // Power control VRH[5:0]
	0xC1, 1, 0x10,             // Power control SAP[2:0];BT[3:0]
	0xC5, 2, 0x3e, 0x28,       // VCM control
	0xC7, 1, 0x86,             // VCM control2
	0x36, 1, 0xE8,             // Memory Access Control
	0x37, 1, 0x00,             // Vertical scroll zero
	0x3A, 1, 0x55,
	0xB1, 2, 0x00, 0x18,
	0xB6, 3, 0x08, 0x82, 0x27, // Display Function Control
	0xF2, 1, 0x00,                         // 3Gamma Function Disable
	0x26, 1, 0x01,             // Gamma curve selected
	0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
		0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
	0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
		0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
};

void delay(uint32_t clocks);

void ili9341_driver_init(ili9341_driver_t *driver)
{
	cmd_t end_cmd = {
		.argc = 0
	};
	cmd_t *cmd = (cmd_t *) init_cmds;
	cmd_t *end = (cmd_t *) (init_cmds + sizeof(init_cmds));
	
	end_cmd.cmd = 0x28;
	ili9341_driver_wcmd(driver, &end_cmd);
	delay(1<< 20);
	while (cmd != end) {
		ili9341_driver_wcmd(driver, cmd);
		cmd = (cmd_t *)((uint8_t *) cmd + 2 + cmd->argc); // 2 is size of cmd + argc fields
	}
	
	end_cmd.cmd = 0x11;
	ili9341_driver_wcmd(driver, &end_cmd);
	delay(1 << 20);
	end_cmd.cmd = 0x29;
	ili9341_driver_wcmd(driver, &end_cmd);
	delay(1<< 20);
	end_cmd.cmd = 0x13;
	ili9341_driver_wcmd(driver, &end_cmd);
}

static void _send_byte(SPI_TypeDef *spi, uint8_t byte)
{
	while (!(spi->SR & SPI_SR_TXE))
		;
	
	spi->DR = byte;
	
	while ((spi->SR & SPI_SR_BSY))
		;
	
	while (!(spi->SR & SPI_SR_RXNE))
		;
	
	byte = spi->DR;
}

void ili9341_driver_wcmd(ili9341_driver_t *driver, cmd_t *cmd)
{
	sem_wait(&driver->sem);
	*driver->cs = 0;
	*driver->dc = 0;

	_send_byte(driver->spi, cmd->cmd);
	
	*driver->dc = 1;
	for (int i = 0, j = cmd->argc; i < j; ++i) {
		_send_byte(driver->spi, cmd->argv[i]);
	}
	
	*driver->cs = 1;
	sem_post(&driver->sem);
}

static uint8_t _read_byte(SPI_TypeDef *spi)
{
	uint8_t dummy = 0x00;
	
	while (!(spi->SR & SPI_SR_TXE))
		;
	
	spi->DR = dummy;
	
	while (!(spi->SR & SPI_SR_TXE))
		;
	
	while (!(spi->SR & SPI_SR_RXNE))
		;
	
	while ((spi->SR & SPI_SR_BSY))
		;
	
	return SPI1->DR;
}

void ili9341_driver_rans(ili9341_driver_t *driver, uint8_t *buf, uint8_t n)
{
	*driver->cs = 0;
	*driver->dc = 1;
	
	for(int i = 0; i < n; ++i) {
		buf[i] = _read_byte(driver->spi);
	}
	
	buf[n] = '\0';
	
	*driver->cs = 1;
	*driver->dc = 0;
}

void ili9341_driver_w16data(ili9341_driver_t *driver, uint16_t data)
{
	*driver->cs = 0;
	*driver->dc = 1;
	
	_send_byte(driver->spi, (data & 0xFF00) >> 8);
	_send_byte(driver->spi, data & 0x00FF);
	
	*driver->cs = 1;
}

void ili9341_driver_wcolor(ili9341_driver_t *driver, uint16_t color, uint16_t n)
{
	static uint16_t color_buf;
	
	sem_wait(&driver->sem);
	
	*driver->cs = 0;
	*driver->dc = 0;
	
	_driver_in_use = driver;
	_send_byte(driver->spi, 0x2C);
	
	*driver->dc = 1;
	
	color_buf = (color >> 8) | (color << 8);
	
	// Reconfigure spi for 16 bit data format
	while (driver->spi->SR & SPI_SR_BSY)
		;
	
	driver->spi->CR1 &= ~SPI_CR1_SPE;
	driver->spi->CR1 |= SPI_CR1_DFF;
	driver->spi->CR1 |= SPI_CR1_SPE;
	driver->spi->CR2 |= SPI_CR2_TXDMAEN;
	
	DMA1_Channel5->CPAR = (uint32_t) &driver->spi->DR;
	DMA1_Channel5->CMAR = (uint32_t) &color_buf;
	DMA1_Channel5->CNDTR = n;

	
	DMA1_Channel5->CCR &= ~(DMA_CCR5_PSIZE
		| DMA_CCR5_MSIZE
		| DMA_CCR5_DIR
		| DMA_CCR5_MINC);
	
	DMA1_Channel5->CCR |= DMA_CCR5_DIR
		| DMA_CCR5_MSIZE_0
		| DMA_CCR5_PSIZE_0;
		
	DMA1_Channel5->CCR |= DMA_CCR5_EN;
}

void ili9341_driver_wpixels(ili9341_driver_t *driver, uint16_t *pixels, uint16_t n)
{
	sem_wait(&driver->sem);
	
	*driver->cs = 0;
	*driver->dc = 0;
	_driver_in_use = driver;
	
	_send_byte(driver->spi, 0x2C);
	*driver->dc = 1;
	
	// Reconfigure spi for 16 bit data format
	while (driver->spi->SR & SPI_SR_BSY)
		;
	
	driver->spi->CR1 &= ~SPI_CR1_SPE;
	driver->spi->CR1 |= SPI_CR1_DFF;
	driver->spi->CR1 |= SPI_CR1_SPE;
	driver->spi->CR2 |= SPI_CR2_TXDMAEN;
	
	DMA1_Channel5->CPAR = (uint32_t) &driver->spi->DR;
	DMA1_Channel5->CMAR = (uint32_t) pixels;
	DMA1_Channel5->CNDTR = n;

	
	DMA1_Channel5->CCR &= ~(DMA_CCR5_PSIZE
		| DMA_CCR5_MSIZE
		| DMA_CCR5_DIR
		| DMA_CCR5_MINC);
	
	DMA1_Channel5->CCR |= DMA_CCR5_DIR
		| DMA_CCR5_MSIZE_0
		| DMA_CCR5_PSIZE_0
		| DMA_CCR5_MINC;
		
	DMA1_Channel5->CCR |= DMA_CCR5_EN;
}

void DMA1_Channel5_IRQHandler(void)
{
	DMA1->IFCR |= DMA_IFCR_CGIF5;

	while(_driver_in_use->spi->SR & SPI_SR_BSY)
		;	
	
	DMA1_Channel5->CCR &= ~DMA_CCR5_EN;	
	
	*_driver_in_use->cs = 1;
	
	
	// Reconfigure spi for 8 bit data format
	_driver_in_use->spi->CR1 &= ~SPI_CR1_SPE;
	_driver_in_use->spi->CR1 &= ~SPI_CR1_DFF;
	_driver_in_use->spi->CR1 |= SPI_CR1_SPE;

	sem_post(&_driver_in_use->sem);
}
