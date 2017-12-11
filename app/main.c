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

static char batteryVoltageString[5];
void adcDataReady(const uint16_t data[], uint32_t length)
{
    uint32_t batteryVoltage = 0;
    uint32_t bcnt = 0;
    for (bcnt = 0; bcnt < 64; bcnt++)
        batteryVoltage += *(data + (6 * bcnt));
    batteryVoltage = (batteryVoltage >> 6) * 6750 / 4095;
    batteryVoltageString[4] = 0;
    batteryVoltageString[0] = batteryVoltage / 1000 + '0';
    batteryVoltageString[1] = batteryVoltage % 1000 / 100 + '0';
    batteryVoltageString[2] = batteryVoltage % 100 / 10 + '0';
    batteryVoltageString[3] = batteryVoltage % 10 + '0';

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

    halAdcInit(adcDataReady);
    halI2cLcdPrintString(40, 1, "Power ON!");
    halI2cLcdRefresh();
    uint32_t refreshTimer = 0;
    for (;;) {
        appProcessCmdBuffer(uartRfGetCurrentRxBuffIdx());
        if (refreshTimer++ == 100000) {
            halI2cLcdPrintString(0, 3, "Battery voltage:");
            halI2cLcdPrintString(100, 3, batteryVoltageString);
            halI2cLcdRefresh();
            refreshTimer = 0;
        }
    }
}
