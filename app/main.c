#include "HalCommon.h"
#include "HalGpio.h"
#include "HalUart.h"
#include "HalI2c.h"
#include "HalAdc.h"
#include "Timer.h"
#include <stddef.h>

typedef enum {
    StartByte,
    CmdByte,
} ProtocolState;


static uint8_t batteryLevel = 0;
static volatile uint32_t cmdTimer = 0;
static bool connectionStatus = false;

void appProcessProtocolByte(uint8_t cmdByte)
{
    cmdTimer = 0;
    batteryLevel = cmdByte;
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

static volatile bool isLcdRefreshNeeded = true, isRfRefreshNeeded = true;

void timerCb(void)
{
    if (cmdTimer++ >= 1000) {
        cmdTimer = 1000;
    }

    static uint32_t lcdRefreshTimer = 0;
    static uint32_t rfRefreshTimer = 0;
    if (lcdRefreshTimer++ >= 2) {
        isLcdRefreshNeeded = true;
        lcdRefreshTimer = 0;
    }

    if (rfRefreshTimer++ >= 10) {
        isRfRefreshNeeded = true;
        rfRefreshTimer = 0;
    }

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

#define PACKET_LENGTH   9
#define START_BIT       0x80

#pragma pack(push, 1)
typedef struct ProtocolJoystickPacket {
    uint16_t leftX;
    uint16_t leftY;
    uint16_t rightX;
    uint16_t rightY;
    uint8_t buttonCode;
} ProtocolJoystickPacket;
#pragma pack(pop)

static uint8_t crcUpdate(uint8_t crc, uint8_t byte)
{
    const uint8_t poly = 0x07;
    crc ^= byte;
    for (uint32_t i = 0; i < 0x08; i++) {
        crc = (crc << 1) ^ ((crc & 0x80) ? poly : 0);
    }

    return crc;
}

static uint8_t crc8(uint8_t buffer[], uint32_t length)
{
    uint8_t crc8 = 0, bcnt = 0;
    for (bcnt = 0; bcnt < length; bcnt++) {
        crc8 = crcUpdate(crc8, buffer[bcnt]);
    }

    return crc8;
}

static void protocolPack(uint8_t data[PACKET_LENGTH], const ProtocolJoystickPacket *joystickData)
{
    if (data == NULL || joystickData == NULL) {
        return;
    }

    data[0] = ((joystickData->leftX >> 4) | START_BIT);
    data[1] = (((joystickData->leftX << 4) | (joystickData->leftY >> 8)) & ~START_BIT);
    data[2] = (joystickData->leftY & ~START_BIT);
    data[3] = ((joystickData->rightX >> 4) & ~START_BIT);
    data[4] = (((joystickData->rightX << 4) | (joystickData->rightY >> 8)) & ~START_BIT);
    data[5] = (joystickData->rightY & ~START_BIT);
    data[6] = joystickData->buttonCode & 0x7F;
    data[7] = ((joystickData->leftX >> 11) & 0x01) | ((joystickData->leftX >> 2) & 0x02) | ((joystickData->leftY >> 5) & 0x04) |
            ((joystickData->rightX >> 8) & 0x08) | ((joystickData->rightX << 1) & 0x10) | ((joystickData->rightY >> 2) & 0x20);

    uint8_t crc = crc8(data, 8);
    data[8] = crc & 0x7F;
    data[7] |= (crc >> 1) & 0x40;
}

ProtocolJoystickPacket joyData;
uint32_t batteryVoltageRc, batteryVoltageQ;
uint8_t rcData[PACKET_LENGTH];

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
    timerInit(timerCb);
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

            batteryVoltageQ = batteryLevel * 6 / 100;
            if (batteryVoltageQ > 6) {
                batteryVoltageQ = 6;
            }
            batRc[0] = BAT_START + batteryVoltageRc;
            halI2cLcdPrintString(108, 0, batRc);
            batQ[0] = BAT_START + batteryVoltageQ;
            halI2cLcdPrintString(108, 2, batQ);

            halI2cSetConn(connectionStatus);
            uint16_t buttonData = adcValues[5];
            joyData.leftX = adcValues[2];
            joyData.leftY = adcValues[3];
            joyData.rightX = adcValues[4];
            joyData.rightY = adcValues[1];

            joyData.buttonCode = getButtonState(buttonData);
            halI2cSetJoyBars(joyData.leftX, joyData.leftY, joyData.rightX, joyData.rightY);
            halI2cSetButton(joyData.buttonCode);
            protocolPack(rcData, &joyData);

            if (isRfTxReady && isRfRefreshNeeded) {
                uartRfDmaTx(rcData, sizeof(rcData), rfTxReady);
                isRfTxReady = false;
                isRfRefreshNeeded = false;
            }

            adcValuesReady = false;
        }

        appProcessCmdBuffer(uartRfGetCurrentRxBuffIdx());
        if (cmdTimer >= 1000) {
            batteryLevel = 0;
            connectionStatus = false;
        } else {
            connectionStatus = true;
        }

        if (isLcdRefreshNeeded) {
            halI2cLcdRefreshMin();
            isLcdRefreshNeeded = false;
        }

    }
}
