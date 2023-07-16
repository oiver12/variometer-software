#define VERSION2n2n1

#ifdef VERSION2n2
#define POWSW_ENABLE 13

#define DPS310I2CPORT I2C_NUM_0
#define BMI088I2CPort I2C_NUM_0
#define QMC588LI2CPort I2C_NUM_0

#define I2C_MASTER_FREQ_HZ 80000
#define I2C_SDA 37
#define I2C_SCL 36

#define USART2_TX 17
#define USART2_RX 18

#define PresSens_INT 38

#define I2S_BCLK 11
#define I2S_LRCLK 12
#define I2S_DIN 10
#define AUDIO_EN 21
#define AUDIO_GAIN 47

#define SD_CLK 4
#define SD_CMD 5
#define SD_D0 6
#define SD_D1 7
#define SD_D2 15
#define SD_D3 16

#define Button_Enter 1 // Button 1
#define Button_Menu 2  // Button 2
#define Button_Up 9    // Button 3
#define Button_Down 48 // Button 4

#define BAT_MS 3
#define BAT_ADCUNIT ADC_UNIT_2
#define BAT_ADCCHANNEL ADC_CHANNEL_3

/**
 * e-Paper GPIO
 **/
#define EPD_RST_PIN 45
#define EPD_DC_PIN 42
#define EPD_CS_PIN 39
#define EPD_BUSY_PIN 40
#define EPD_MOSI_PIN 41
#define EPD_CLK_PIN 46

#elif defined VERSION2n2n1

#define POWSW_ENABLE 42

#define DPS310I2CPORT I2C_NUM_1
#define BMI088I2CPort I2C_NUM_0
#define QMC588LI2CPort I2C_NUM_0

#define I2C_MASTER_FREQ_HZ 80000
#define I2C1_SDA 40
#define I2C1_SCL 41
#define I2C2_SDA 48
#define I2C2_SCL 47

#define USART2_TX 17
#define USART2_RX 18

#define PresSens_INT 21

#define I2S_BCLK 11
#define I2S_LRCLK 12
#define I2S_DIN 10

#define SD_CLK 4
#define SD_CMD 5
#define SD_D0 6
#define SD_D1 7
#define SD_D2 15
#define SD_D3 16

#define Button_Enter 1 // Button 1
#define Button_Menu 2  // Button 2
#define Button_Up 9    // Button 3
#define Button_Down 43 // Button 4

#define BAT_MS 46
#define BAT_ADCUNIT ADC_UNIT_1
#define BAT_ADCCHANNEL ADC_CHANNEL_2

/**
 * e-Paper GPIO
 **/
#define EPD_RST_PIN 45
#define EPD_DC_PIN 38
#define EPD_CS_PIN 35
#define EPD_BUSY_PIN 36
#define EPD_MOSI_PIN 37
#define EPD_CLK_PIN 39
#endif