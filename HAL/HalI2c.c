#include "HalI2c.h"
#include "stm32f0xx_conf.h"

#include <stddef.h>

#define HAL_I2C_LCD_SDA_PIN                             GPIO_Pin_10
#define HAL_I2C_LCD_SDA_PIN_SOURCE                      GPIO_PinSource10

#define HAL_I2C_LCD_SCL_PIN                             GPIO_Pin_9
#define HAL_I2C_LCD_SCL_PIN_SOURCE                      GPIO_PinSource9

#define HAL_I2C_LCD_PORT                                GPIOA

#define I2C_PERIPH_HWUNIT                               I2C1

#define I2C_PERIPH_OWN_ADDRESS                          1u

#define LCD_ADDRESS                                     0x78

#define SSD1306_LCDWIDTH                                128
#define SSD1306_LCDHEIGHT                               64

#define SSD1306_SETCONTRAST                             0x81
#define SSD1306_DISPLAYALLON_RESUME                     0xA4
#define SSD1306_DISPLAYALLON                            0xA5
#define SSD1306_NORMALDISPLAY                           0xA6
#define SSD1306_INVERTDISPLAY                           0xA7
#define SSD1306_DISPLAYOFF                              0xAE
#define SSD1306_DISPLAYON                               0xAF
#define SSD1306_SETDISPLAYOFFSET                        0xD3
#define SSD1306_SETCOMPINS                              0xDA
#define SSD1306_SETVCOMDETECT                           0xDB
#define SSD1306_SETDISPLAYCLOCKDIV                      0xD5
#define SSD1306_SETPRECHARGE                            0xD9
#define SSD1306_SETMULTIPLEX                            0xA8
#define SSD1306_SETLOWCOLUMN                            0x00
#define SSD1306_SETHIGHCOLUMN                           0x10
#define SSD1306_SETSTARTLINE                            0x40
#define SSD1306_MEMORYMODE                              0x20
#define SSD1306_COLUMNADDR                              0x21
#define SSD1306_PAGEADDR                                0x22
#define SSD1306_COMSCANINC                              0xC0
#define SSD1306_COMSCANDEC                              0xC8
#define SSD1306_SEGREMAP                                0xA0
#define SSD1306_CHARGEPUMP                              0x8D
#define SSD1306_EXTERNALVCC                             0x01
#define SSD1306_SWITCHCAPVCC                            0x02
#define SSD1306_ACTIVATE_SCROLL                         0x2F
#define SSD1306_DEACTIVATE_SCROLL                       0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA                0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL                 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL                  0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL    0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL     0x2A

uint8_t lcdBuffer[SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / PIXEL_PER_BYTE];

static void i2cInit(void)
{
    GPIO_InitTypeDef gpioInitStructure = {
        .GPIO_Pin = HAL_I2C_LCD_SDA_PIN | HAL_I2C_LCD_SCL_PIN,
        .GPIO_Mode = GPIO_Mode_AF,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_OType = GPIO_OType_OD,
        .GPIO_PuPd = GPIO_PuPd_UP
    };

    GPIO_Init(HAL_I2C_LCD_PORT, &gpioInitStructure);

    GPIO_PinAFConfig(HAL_I2C_LCD_PORT, HAL_I2C_LCD_SDA_PIN_SOURCE, GPIO_AF_4);
    GPIO_PinAFConfig(HAL_I2C_LCD_PORT, HAL_I2C_LCD_SCL_PIN_SOURCE, GPIO_AF_4);

    I2C_InitTypeDef i2cInitStructure;
    I2C_StructInit(&i2cInitStructure);
    i2cInitStructure.I2C_Mode = I2C_Mode_I2C;
    i2cInitStructure.I2C_Timing = 0x00000102;
    i2cInitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2cInitStructure.I2C_OwnAddress1 = I2C_PERIPH_OWN_ADDRESS;
    i2cInitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_Init(I2C_PERIPH_HWUNIT, &i2cInitStructure);
    I2C_AcknowledgeConfig(I2C_PERIPH_HWUNIT, ENABLE);
    I2C_StretchClockCmd(I2C_PERIPH_HWUNIT, ENABLE);
    I2C_Cmd(I2C_PERIPH_HWUNIT, ENABLE);

}

static void i2cWrite(uint8_t chipAddress, uint8_t registerAddress, uint8_t data)
{
    while (I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_BUSY));

    I2C_TransferHandling(I2C_PERIPH_HWUNIT, chipAddress, 2, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
    while(I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_ISR_TXIS) == RESET);

	I2C_SendData(I2C_PERIPH_HWUNIT, registerAddress);
    while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_TXE));

	I2C_SendData(I2C_PERIPH_HWUNIT, data);
    while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_TXE));

    I2C_TransferHandling(I2C_PERIPH_HWUNIT, chipAddress, 0, I2C_AutoEnd_Mode,  I2C_Generate_Stop);
    while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_STOPF));
    I2C_ClearFlag(I2C_PERIPH_HWUNIT, I2C_FLAG_STOPF);
}

static void sendCommand(uint8_t c)
{
    uint8_t controlData[] = {0x00, c};
    i2cWrite(LCD_ADDRESS, controlData[0], controlData[1]);
}

void halI2cLcdRefresh(void)
{
    sendCommand(SSD1306_COLUMNADDR);
    sendCommand(0);
    sendCommand(SSD1306_LCDWIDTH - 1);

    sendCommand(SSD1306_PAGEADDR);
    sendCommand(0);
    sendCommand(7);

    uint32_t blockCntr = 0;
    for (blockCntr = 0; blockCntr < 8; blockCntr++) {
        while (I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_BUSY));
        I2C_TransferHandling(I2C_PERIPH_HWUNIT, LCD_ADDRESS, 129, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
        while(I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_ISR_TXIS) == RESET);
        I2C_SendData(I2C_PERIPH_HWUNIT, 0x40);
        while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_TXE));

        uint32_t byteCntr = 0;
        for (byteCntr = 0; byteCntr < 128; byteCntr++) {
            I2C_SendData(I2C_PERIPH_HWUNIT, lcdBuffer[blockCntr * 128 + byteCntr]);
            while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_TXE));
        }

        I2C_TransferHandling(I2C_PERIPH_HWUNIT, LCD_ADDRESS, 0, I2C_AutoEnd_Mode,  I2C_Generate_Stop);
        while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_STOPF));
        I2C_ClearFlag(I2C_PERIPH_HWUNIT, I2C_FLAG_STOPF);

    }

}

void halI2cLcdInit(void)
{
    i2cInit();
    sendCommand(SSD1306_DISPLAYOFF);
    sendCommand(SSD1306_SETDISPLAYCLOCKDIV);
    sendCommand(0x80);
    sendCommand(SSD1306_SETMULTIPLEX);
    sendCommand(SSD1306_LCDHEIGHT - 1);
    sendCommand(SSD1306_SETDISPLAYOFFSET);
    sendCommand(0x00);
    sendCommand(SSD1306_SETSTARTLINE | 0x00);
    sendCommand(SSD1306_CHARGEPUMP);
    sendCommand(0x14);
    sendCommand(SSD1306_MEMORYMODE);
    sendCommand(0x00);
    sendCommand(SSD1306_SEGREMAP | 0x01);
    sendCommand(SSD1306_COMSCANDEC);
    sendCommand(SSD1306_SETCOMPINS);
    sendCommand(0x12);
    sendCommand(SSD1306_SETCONTRAST);
    sendCommand(0x9f);
    sendCommand(SSD1306_SETPRECHARGE);
    sendCommand(0xF1);
    sendCommand(SSD1306_SETVCOMDETECT);
    sendCommand(0x40);
    sendCommand(SSD1306_DISPLAYALLON_RESUME);
    sendCommand(SSD1306_NORMALDISPLAY);
    sendCommand(SSD1306_DEACTIVATE_SCROLL);
    sendCommand(SSD1306_DISPLAYON);
}

extern const uint8_t * getFont(void);

static void inline halI2cPutPixel(uint32_t x, uint32_t y)
{
    lcdBuffer[((0x07 - y / 8) << 0x07) + (127 - x)] |= (1 << (0x07 - (y & 0x07)));
}

static void inline halI2cClrPixel(uint32_t x, uint32_t y)
{
    lcdBuffer[((0x07 - y / 8) << 0x07) + (127 - x)] &= ~(1 << (0x07 - (y & 0x07)));
}


void halI2cLcdPrintString(uint32_t x, uint32_t y, const char str[])
{
    if (str == NULL)
        return;

    x = 127 - x;
    y = 7 - y;
    const uint8_t * font = getFont();
    uint32_t byteCntr = 0;
    while (*str) {
        uint32_t chrCounter = 0;
        for (chrCounter = 0; chrCounter < 5; chrCounter++) {
            uint8_t b = font[(*str - ' ') * 5 + chrCounter];
            b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
            b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);

            lcdBuffer[(y << 7) + x - byteCntr++] = b;
        }
        str++;
        lcdBuffer[(y << 7) + x - byteCntr++] = 0;
    }

}

void halI2cInitJoyBars(void)
{
    uint32_t pcnt = 0;
    for (pcnt = 0; pcnt < 7; pcnt++) {
        halI2cPutPixel(0 + pcnt, 1);
        halI2cPutPixel(0 + pcnt, 56);
        halI2cPutPixel(121 + pcnt, 1);
        halI2cPutPixel(121 + pcnt, 56);
        halI2cPutPixel(7, 57 + pcnt);
        halI2cPutPixel(120, 57 + pcnt);
        halI2cPutPixel(7 + 55, 57 + pcnt);
        halI2cPutPixel(7 + 58, 57 + pcnt);
    }
    for (pcnt = 0; pcnt < 55; pcnt++) {
        halI2cPutPixel(3, 2 + pcnt);
        halI2cPutPixel(124, 2 + pcnt);
    }
    for (pcnt = 0; pcnt < 55; pcnt++) {
        halI2cPutPixel(8 + pcnt, 60);
        halI2cPutPixel(65 + pcnt, 60);
    }
}

void halI2cSetJoyBars(uint32_t x, uint32_t y, uint32_t x1, uint32_t y1)
{
    static uint32_t prevVals[4], vals[4];

    vals[0] = 2 + 53 - (y * 53 / 4050);
    vals[1] = 8 + 53 - (x * 53 / 4050);
    vals[2] = 2 + 53 - (y1 * 53 / 4050);
    vals[3] = 66 + 53 - (x1 * 53 / 4050);


    uint32_t pcnt = 0;
    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cClrPixel(0 + pcnt, prevVals[0]);
    }

    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cPutPixel(0 + pcnt, vals[0]);
    }

    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cClrPixel(prevVals[1], 57 + pcnt);
    }

    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cPutPixel(vals[1], 57 + pcnt);
    }


    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cClrPixel(121 + pcnt, prevVals[2]);
    }

    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cPutPixel(121 + pcnt, vals[2]);
    }

    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cClrPixel(prevVals[3], 57 + pcnt);
    }

    for (pcnt = 0; pcnt < 7; pcnt++) {
        if (pcnt == 3) {
            continue;
        }

        halI2cPutPixel(vals[3], 57 + pcnt);
    }

    for (pcnt = 0; pcnt < 4; pcnt++) {
        prevVals[pcnt] = vals[pcnt];
    }

}

void halI2cLcdRefreshMin(void)
{
    static uint32_t blockCntr = 0;

    if (blockCntr == 0) {
        sendCommand(SSD1306_COLUMNADDR);
        sendCommand(0);
        sendCommand(SSD1306_LCDWIDTH - 1);

        sendCommand(SSD1306_PAGEADDR);
        sendCommand(0);
        sendCommand(7);
    }


    while (I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_BUSY));
    I2C_TransferHandling(I2C_PERIPH_HWUNIT, LCD_ADDRESS, 129, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
    while(I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_ISR_TXIS) == RESET);
    I2C_SendData(I2C_PERIPH_HWUNIT, 0x40);
    while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_TXE));

    uint32_t byteCntr = 0;
    for (byteCntr = 0; byteCntr < 128; byteCntr++) {
        I2C_SendData(I2C_PERIPH_HWUNIT, lcdBuffer[blockCntr * 128 + byteCntr]);
        while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_TXE));
    }

    I2C_TransferHandling(I2C_PERIPH_HWUNIT, LCD_ADDRESS, 0, I2C_AutoEnd_Mode,  I2C_Generate_Stop);
    while(!I2C_GetFlagStatus(I2C_PERIPH_HWUNIT, I2C_FLAG_STOPF));
    I2C_ClearFlag(I2C_PERIPH_HWUNIT, I2C_FLAG_STOPF);

    if (blockCntr++ == 7) {
        blockCntr = 0;
    }
}


static const char buttonEmptySymbol[] = {128 + 7, 0};
static const char buttonCheckedSymbol[] = {128 + 8, 0};

static const uint32_t buttonCx[] = {10, 114, 61, 61, 53, 69};
static const uint32_t buttonCy[] = {6, 6, 4, 6, 5, 5};

void halI2cSetConn(bool isConnected)
{
    halI2cLcdPrintString(108, 4, isConnected ? buttonCheckedSymbol : buttonEmptySymbol);
}

void halI2cInitButton(void)
{
    uint32_t cnt = 0;
    for (cnt = 0; cnt < 6; cnt++) {
        halI2cLcdPrintString(buttonCx[cnt], buttonCy[cnt], buttonEmptySymbol);
    }
}

void halI2cSetButton(uint32_t button)
{
    static uint32_t prevButton = 0;

    if (prevButton) {
        halI2cLcdPrintString(buttonCx[prevButton - 1], buttonCy[prevButton - 1], buttonEmptySymbol);
    }

    if (button) {
        halI2cLcdPrintString(buttonCx[button - 1], buttonCy[button - 1], buttonCheckedSymbol);
    }

    prevButton = button;
}
