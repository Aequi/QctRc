#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#include <stdbool.h>
#include <stdint.h>

void halGpioInit();
void halGpioEnableRfAt(bool isEnabled);

#endif
