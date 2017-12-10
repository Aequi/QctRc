#include "HalAdc.h"
#include "stm32f0xx_conf.h"

#define HAL_ADC_JOY_L_X_PIN                             GPIO_Pin_6
#define HAL_ADC_JOY_L_Y_PIN                             GPIO_Pin_5
#define HAL_ADC_JOY_R_X_PIN                             GPIO_Pin_4
#define HAL_ADC_JOY_R_Y_PIN                             GPIO_Pin_7
#define HAL_ADC_BUTTONS_PIN                             GPIO_Pin_1
#define HAL_ADC_BATTERY_PIN                             GPIO_Pin_0

#define HAL_ADC_BUTTONS_PORT                            GPIOB
#define HAL_ADC_OTHER_PORT                              GPIOA


void halAdcInit(void)
{
    GPIO_InitTypeDef gpioInitStructure = {
        .GPIO_Pin =  HAL_ADC_JOY_L_X_PIN | HAL_ADC_JOY_L_Y_PIN | HAL_ADC_JOY_R_X_PIN | HAL_ADC_JOY_R_Y_PIN | HAL_ADC_BATTERY_PIN,
        .GPIO_Mode = GPIO_Mode_AN,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_OType = GPIO_OType_PP,
        .GPIO_PuPd = GPIO_PuPd_UP
    };

    GPIO_Init(HAL_ADC_OTHER_PORT, &gpioInitStructure);
    gpioInitStructure.GPIO_Pin = HAL_ADC_BUTTONS_PIN;
    GPIO_Init(HAL_ADC_BUTTONS_PORT, &gpioInitStructure);

    ADC_InitTypeDef adcInitStructure;
    ADC_StructInit(&adcInitStructure);
    adcInitStructure.ADC_ContinuousConvMode = ENABLE;
    adcInitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    adcInitStructure.ADC_Resolution = ADC_Resolution_12b;
    adcInitStructure.ADC_ScanDirection = ADC_ScanDirection_Upward;
    ADC_Init(ADC1, &adcInitStructure);
    ADC_ChannelConfig()
}
