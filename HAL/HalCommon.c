#include "halCommon.h"
#include "stm32f0xx_conf.h"
#include <stddef.h>

void (*dmaCh1Cb)(bool isHalf) = NULL;
void (*dmaCh2Cb)(bool isHalf) = NULL;
void (*dmaCh3Cb)(bool isHalf) = NULL;
void (*dmaCh4Cb)(bool isHalf) = NULL;
void (*dmaCh5Cb)(bool isHalf) = NULL;

void halCommonInit()
{
    SystemCoreClockUpdate();
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_DMAChannelRemapConfig(SYSCFG_DMARemap_USART1Tx, ENABLE);

    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
}

void DMA1_Channel4_5_IRQHandler()
{
    if (DMA_GetITStatus(DMA1_IT_TC4)) {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        if (dmaCh4Cb)
            dmaCh4Cb(false);
    }

    if (DMA_GetITStatus(DMA1_IT_HT4)) {
        DMA_ClearITPendingBit(DMA1_IT_HT4);
        if (dmaCh4Cb)
            dmaCh4Cb(true);
    }

    if (DMA_GetITStatus(DMA1_IT_TC5)) {
        DMA_ClearITPendingBit(DMA1_IT_TC5);
        if (dmaCh5Cb)
            dmaCh5Cb(false);
    }

    if (DMA_GetITStatus(DMA1_IT_HT5)) {
        DMA_ClearITPendingBit(DMA1_IT_HT5);
        if (dmaCh5Cb)
            dmaCh5Cb(true);
    }
}

void DMA1_Channel2_3_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC2)) {
        DMA_ClearITPendingBit(DMA1_IT_TC2);
        if (dmaCh2Cb)
            dmaCh2Cb(false);
    }

    if (DMA_GetITStatus(DMA1_IT_HT2)) {
        DMA_ClearITPendingBit(DMA1_IT_HT2);
        if (dmaCh2Cb)
            dmaCh2Cb(true);
    }

    if (DMA_GetITStatus(DMA1_IT_TC3)) {
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        if (dmaCh3Cb)
            dmaCh3Cb(false);
    }

    if (DMA_GetITStatus(DMA1_IT_HT3)) {
        DMA_ClearITPendingBit(DMA1_IT_HT3);
        if (dmaCh3Cb)
            dmaCh3Cb(true);
    }
}

void DMA1_Channel1_IRQHandler()
{
    if (DMA_GetITStatus(DMA1_IT_TC1)) {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        if (dmaCh1Cb)
            dmaCh1Cb(false);
    }

    if (DMA_GetITStatus(DMA1_IT_HT1)) {
        DMA_ClearITPendingBit(DMA1_IT_HT1);
        if (dmaCh1Cb)
            dmaCh1Cb(true);
    }
}
