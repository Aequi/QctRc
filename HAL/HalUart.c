#include "HalUart.h"
#include "HalCommon.h"
#include "stm32f0xx_conf.h"

#include <stddef.h>

#define UART_RF_TX_PIN      GPIO_Pin_3
#define UART_RF_TX_PIN_SRC  GPIO_PinSource3
#define UART_RF_TX_PORT     GPIOA

#define UART_RF_RX_PIN      GPIO_Pin_2
#define UART_RF_RX_PIN_SRC  GPIO_PinSource2
#define UART_RF_RX_PORT     GPIOA

#define UART_RF_PORT        GPIOA

uint8_t uartCmdInBuffer[UART_CMD_BUFF_SIZE];
static volatile bool uartRfDmaTxActive = false;
static void (*uartRfDmaTxCompleteCb)() = NULL;

void uartRfInit(uint32_t baudRate)
{
    GPIO_InitTypeDef gpioInitStructure = {.GPIO_Pin = UART_RF_TX_PIN | UART_RF_RX_PIN,
                                          .GPIO_Mode = GPIO_Mode_AF,
                                          .GPIO_Speed = GPIO_Speed_50MHz,
                                          .GPIO_OType = GPIO_OType_PP,
                                          .GPIO_PuPd = GPIO_PuPd_UP};

    GPIO_Init(UART_RF_PORT, &gpioInitStructure);

    GPIO_PinAFConfig(UART_RF_PORT, UART_RF_TX_PIN_SRC, GPIO_AF_1);
    GPIO_PinAFConfig(UART_RF_PORT, UART_RF_RX_PIN_SRC, GPIO_AF_1);

    USART_InitTypeDef usartInitStructure = {.USART_BaudRate = baudRate,
                                            .USART_WordLength = USART_WordLength_8b,
                                            .USART_StopBits = USART_StopBits_1,
                                            .USART_Parity = USART_Parity_No,
                                            .USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
                                            .USART_HardwareFlowControl = USART_HardwareFlowControl_None};

    USART_Init(USART1, &usartInitStructure);
    USART_Cmd(USART1, ENABLE);
    USART_DMACmd(USART1, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);

    DMA_InitTypeDef dmaInitStructure;

    dmaInitStructure.DMA_BufferSize = sizeof(uartCmdInBuffer);
    dmaInitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    dmaInitStructure.DMA_M2M = DMA_M2M_Disable;
    dmaInitStructure.DMA_MemoryBaseAddr = (uint32_t) uartCmdInBuffer;
    dmaInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmaInitStructure.DMA_Mode = DMA_Mode_Circular;
    dmaInitStructure.DMA_PeripheralBaseAddr = (uint32_t) &USART1->RDR;
    dmaInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmaInitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_Init(DMA1_Channel3, &dmaInitStructure);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_HT, ENABLE);
}

static void uartRfDmaTxCompleteHandler(bool isHalf)
{
    if (!isHalf) {
        DMA_Cmd(DMA1_Channel4, DISABLE);
        uartRfDmaTxActive = false;
        if (uartRfDmaTxCompleteCb)
            uartRfDmaTxCompleteCb();
    }
}

void uartRfDmaTx(uint8_t *dataBuffer, uint32_t bufferSize, void (*txCompleteCb)())
{
    if (uartRfDmaTxActive)
        return;

    if (txCompleteCb)
        uartRfDmaTxCompleteCb = txCompleteCb;

    dmaCh4Cb = uartRfDmaTxCompleteHandler;

    DMA_InitTypeDef dmaInitStructure;

    dmaInitStructure.DMA_BufferSize = bufferSize;
    dmaInitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    dmaInitStructure.DMA_M2M = DMA_M2M_Disable;
    dmaInitStructure.DMA_MemoryBaseAddr = (uint32_t) dataBuffer;
    dmaInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmaInitStructure.DMA_Mode = DMA_Mode_Normal;
    dmaInitStructure.DMA_PeripheralBaseAddr = (uint32_t) &USART1->TDR;
    dmaInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmaInitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_Init(DMA1_Channel4, &dmaInitStructure);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
    uartRfDmaTxActive = true;
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

uint32_t uartRfGetCurrentRxBuffIdx()
{
    return UART_CMD_BUFF_SIZE - DMA1_Channel3->CNDTR;
}

void uartRfDmaStartStopRx(bool isStart)
{
    DMA_Cmd(DMA1_Channel3, isStart ? ENABLE : DISABLE);
}
