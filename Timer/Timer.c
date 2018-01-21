#include "Timer.h"
#include "stm32f0xx.h"

#include <stddef.h>

static TimerCallback timerCallback;

void TIM2_IRQHandler(void)
{
    if (TIM_GetFlagStatus(TIM2, TIM_FLAG_Update)) {
        TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    }
    if (timerCallback != NULL) {
        timerCallback();
    }
}

void timerInit(TimerCallback timerCb)
{
    timerCallback = timerCb;

    TIM_TimeBaseInitTypeDef timInitStructure;
    TIM_TimeBaseStructInit(&timInitStructure);
    timInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    timInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timInitStructure.TIM_Period = 999;
    timInitStructure.TIM_Prescaler = 47;
    TIM_TimeBaseInit(TIM2, &timInitStructure);
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}
