// DRIVER
#define BMI088_I2C
// #define NotifyNMEASentence
#define MAGNETIC_DEVIATION 15

// SCREEN
#define SCREEN_EINK
// #define SCREEM_ILI9488
// #define EInk_DoubleBuffer

// FREERTOS
#define TASK_KALMAN_PRIORITY 10
#define TASK_HEIGHT_PRIORITY 5
#define TASK_ACC_PRIORITY 2
#define TASK_LV_TASK_HANDLE_PRIORITY 2
#define TASK_SCREEN_RENDER_PRIORITY 8

#define TakeOffSpeed 1   // km/h
#define LandingSpeed 2   // km/h
#define LandingTimout 10 // s

#define SOUND_ON

#define VARIO_SPI_RETRYS_INIT 5
#define VARIO_SPI_RETRYS 3
#define VARIO_I2C_RETRYS_INIT 5
#define VARIO_I2C_RETRYS 3