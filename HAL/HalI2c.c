#include "HalI2c.h"
#include "stm32f0xx_conf.h"


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
