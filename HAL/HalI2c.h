#ifndef __HAL_I2C_H__
#define __HAL_I2C_H__

#include <stdbool.h>
#include <stdint.h>

#define LCD_HEIGHT      64
#define LCD_WIDTH       128
#define PIXEL_PER_BYTE  8

extern uint8_t lcdBuffer[LCD_HEIGHT * LCD_WIDTH / PIXEL_PER_BYTE];

void halI2cLcdInit(void);
void halI2cLcdRefresh(void);
void halI2cLcdPrintString(uint32_t x, uint32_t y, const char str[]);

#endif
