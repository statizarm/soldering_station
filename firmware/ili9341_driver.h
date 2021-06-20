#ifndef ILI9341_DRIVER_H_
#define ILI9341_DRIVER_H_

#include "stm32f10x.h"
#include "sem.h"

typedef struct {
	SPI_TypeDef *spi;
	uint32_t *cs;
	uint32_t *dc;
	uint32_t *reset;
	sem_t sem;
} ili9341_driver_t;

typedef struct {
	uint8_t cmd;
	uint8_t argc;
	uint8_t argv[];
} cmd_t;

void ili9341_driver_init(ili9341_driver_t *);
void ili9341_driver_wcmd(ili9341_driver_t *, cmd_t *);
void ili9341_driver_rans(ili9341_driver_t *, uint8_t *, uint8_t);
void ili9341_driver_w16data(ili9341_driver_t *, uint16_t);
void ili9341_driver_wcolor(ili9341_driver_t *, uint16_t color, uint16_t n);
void ili9341_driver_wpixels(ili9341_driver_t *driver, uint16_t *pixels, uint16_t n);

#endif // ILI9341_DRIVER_H_
