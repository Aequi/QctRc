/*
**
**                           Main.c
**
**
**********************************************************************/
/*
   Last committed:     $Revision: 00 $
   Last changed by:    $Author: $
   Last changed date:  $Date:  $
   ID:                 $Id:  $

**********************************************************************/
#include "stm32f0xx_conf.h"
#include "HalCommon.h"
#include "HalGpio.h"
#include "HalUart.h"
#include "HalI2c.h"
#include "HalAdc.h"

#define SET_Rf_PARAMS   0

typedef enum {
    StartByte,
    CmdByte,
} ProtocolState;

void appProcessProtocolByte(uint8_t cmdByte)
{

}

void appProcessCmdBuffer(uint32_t lastByte)
{
    static uint32_t firstByte = 0;
    uint32_t byteCounter = firstByte;

    while (byteCounter != lastByte) {
        appProcessProtocolByte(uartCmdInBuffer[byteCounter]);
        byteCounter = (byteCounter + 1) & UART_CMD_BUFF_MASK;
    }

    firstByte = lastByte;
}

volatile bool isRfTxReady = false;

void rfTxReady()
{
    isRfTxReady = true;
}


int main(void)
{
    halCommonInit();
    halGpioInit();

    #if (SET_Rf_PARAMS == 1)
    halGpioEnableRfAt(true);

    volatile uint32_t cntr = 0;
    for (cntr = 0; cntr < 30000; cntr ++) {

    }

    uartRfInit(9600);

    uartRfDmaStartStopRx(true);

    // Set configuration
    isRfTxReady = false;
    uartRfDmaTx((uint8_t *) "AT+B115200", sizeof("AT+B115200") - 1, rfTxReady);
    while (!isRfTxReady);

    halGpioEnableRfAt(false);

    for (cntr = 0; cntr < 30000; cntr ++) {

    }
    #else
    uartRfInit(115200);
    uartRfDmaStartStopRx(true);
    isRfTxReady = true;
    #endif

    volatile uint32_t cntr = 0;
    for (cntr = 0; cntr < 300000; cntr ++) {

    }

    halI2cLcdInit();
    uartRfDmaStartStopRx(true);
    uint32_t p = 0;
    for (;;) {
        appProcessCmdBuffer(uartRfGetCurrentRxBuffIdx());

        uint32_t cntr = 0;
        for (cntr = 0; cntr < LCD_HEIGHT * LCD_WIDTH / PIXEL_PER_BYTE; cntr++) {
            lcdBuffer[cntr] = ((0x55 >> (p & 7)) ^ (cntr ^ p));
        }
        halI2cLcdRefresh();
        for (cntr = 0; cntr < 300000; cntr++) {

        }

        for (cntr = 0; cntr < LCD_HEIGHT * LCD_WIDTH / PIXEL_PER_BYTE; cntr++) {
            lcdBuffer[cntr] = ((0xAA >> (p & 7)) ^ (cntr ^ p));
        }

        halI2cLcdRefresh();

        for (cntr = 0; cntr < 300000; cntr++) {

        }
        p += 1;
    }
}
