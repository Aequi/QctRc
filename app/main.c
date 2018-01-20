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

static volatile uint32_t ovrCntr = 0;

uint32_t adcValues[ADC_CHANNEL_COUNT];
volatile bool adcValuesReady = false;
void adcDataReady(const uint16_t data[], uint32_t length)
{
    if (adcValuesReady == false) {

        uint32_t bcnt = 0, ccnt = 0;

        for (ccnt = 0; ccnt < ADC_CHANNEL_COUNT; ccnt++) {
            adcValues[ccnt] = 0;
        }

        for (bcnt = 0; bcnt < ADC_BUFF_SIZE; bcnt++) {
            for (ccnt = 0; ccnt < ADC_CHANNEL_COUNT; ccnt++) {
                adcValues[ccnt] += data[bcnt * ADC_CHANNEL_COUNT + ccnt];
            }
        }

        for (ccnt = 0; ccnt < ADC_CHANNEL_COUNT; ccnt++) {
            adcValues[ccnt] >>= 6;
        }

        adcValuesReady = true;
    } else {
        ovrCntr++;
    }
}

uint32_t getButtonState(uint32_t adcValue)
{
    #define JOU_LEFT    842
    #define JOU_RIGHT   958
    #define UP          1107
    #define DOWN        1298
    #define LEFT        1983
    #define RIGHT       1567
    #define MARGIN      15

    if (adcValue < (JOU_LEFT + MARGIN) && adcValue > (JOU_LEFT - MARGIN)) {
        return 0x01;
    }
    if (adcValue < (JOU_RIGHT + MARGIN) && adcValue > (JOU_RIGHT - MARGIN)) {
        return 0x02;
    }
    if (adcValue < (UP + MARGIN) && adcValue > (UP - MARGIN)) {
        return 0x03;
    }
    if (adcValue < (DOWN + MARGIN) && adcValue > (DOWN - MARGIN)) {
        return 0x04;
    }
    if (adcValue < (LEFT + MARGIN) && adcValue > (LEFT - MARGIN)) {
        return 0x5;
    }
    if (adcValue < (RIGHT + MARGIN) && adcValue > (RIGHT - MARGIN)) {
        return 0x6;
    }

    return 0;
}

uint32_t refreshTimer = 0;
uint32_t batteryVoltageRc, balletyVoltageQ, buttonState;
uint16_t joyData[4];
uint8_t rcData[9];

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

    uartRfDmaStartStopRx(true);
    halI2cLcdInit();

    #define BAT_START  128
    char batRc[] = {BAT_START, 0};
    char batQ[] = {BAT_START, 0};

    halI2cLcdPrintString(15, 0, "Rc   battery: ");
    halI2cLcdPrintString(108, 0, batRc);
    halI2cLcdPrintString(15, 2, "Quad battery: ");
    halI2cLcdPrintString(108, 2, batQ);
    halI2cLcdPrintString(15, 4, "Ch: 0");
    halI2cLcdPrintString(80, 4, "Conn: ");
    halI2cSetConn(false);

    halI2cInitJoyBars();
    halI2cInitButton();
    halI2cLcdRefresh();

    halAdcInit(adcDataReady);

    for (;;) {
        if (adcValuesReady) {

            batteryVoltageRc = (adcValues[0] * 6750) >> 12;
            batteryVoltageRc = (batteryVoltageRc - 3300) * 6 / 850;

            if (batteryVoltageRc > 6) {
                batteryVoltageRc = 6;
            }

            batRc[0] = BAT_START + batteryVoltageRc;
            halI2cLcdPrintString(108, 0, batRc);
            batQ[0] = BAT_START + balletyVoltageQ;
            halI2cLcdPrintString(108, 2, batQ);

            buttonState = adcValues[5];
            joyData[0] = adcValues[2];
            joyData[1] = adcValues[3];
            joyData[2] = adcValues[4];
            joyData[3] = adcValues[1];

            buttonState = getButtonState(buttonState);
            halI2cSetJoyBars(joyData[0], joyData[1], joyData[2], joyData[3]);
            halI2cSetButton(buttonState);

            adcValuesReady = false;
        }

        appProcessCmdBuffer(uartRfGetCurrentRxBuffIdx());

        if (refreshTimer++ >= 1000) {
            halI2cLcdRefreshMin();
            refreshTimer = 0;
        }

    }
}
