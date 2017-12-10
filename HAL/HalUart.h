#ifndef __HAL_UART_H__
#define __HAL_UART_H__

#include <stdbool.h>
#include <stdint.h>

#define UART_CMD_BUFF_SIZE  128
#define UART_CMD_BUFF_MASK  0x7F

extern uint8_t uartCmdInBuffer[UART_CMD_BUFF_SIZE];

void uartRfInit(uint32_t baudRate);
void uartRfDmaTx(uint8_t dataBuffer[], uint32_t bufferSize, void (*txCompleteCb)());
uint32_t uartRfGetCurrentRxBuffIdx(void);
void uartRfDmaStartStopRx(bool isStart);

#endif
