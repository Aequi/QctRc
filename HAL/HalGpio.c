#include "HalGpio.h"
#include "stm32f0xx_conf.h"

#define HAL_GPIO_RF_MODE_PIN        GPIO_Pin_1
#define HAL_GPIO_RF_MODE_PORT       GPIOA

void halGpioInit(void)
{
    GPIO_InitTypeDef gpioInitStructure = {.GPIO_Pin =  HAL_GPIO_RF_MODE_PIN,
                                          .GPIO_Mode = GPIO_Mode_OUT,
                                          .GPIO_Speed = GPIO_Speed_50MHz,
                                          .GPIO_OType = GPIO_OType_PP,
                                          .GPIO_PuPd = GPIO_PuPd_DOWN };

    GPIO_Init(HAL_GPIO_RF_MODE_PORT, &gpioInitStructure);

    GPIO_SetBits(HAL_GPIO_RF_MODE_PORT, gpioInitStructure.GPIO_Pin);
}

void halGpioEnableRfAt(bool isEnabled)
{
    GPIO_WriteBit(HAL_GPIO_RF_MODE_PORT, HAL_GPIO_RF_MODE_PIN, isEnabled ? Bit_RESET : Bit_SET);
}

