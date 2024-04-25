#include "MainTasks.h"
#include "Sensor/BMI088.h"
#include "Sensor/DPS310.h"
#include "QuaternionFilter.h"
#include "Kalman/kalman.h"
#include "Sensor/QMC588L.h"
#include "DeviceContext.h"
#include "SoundManager.h"
#include "TestTools/FlightLogger.h"
#include "Watchdog/Watchdog.h"
#include "ErrorHandling.h"
#include "vario_config.h"
#include "pinDefines.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

#define R_SPECIFIC 287.053
#define GRAVITY_G 9.80665
#define TEMPGRAD_L 0.0065

typedef struct ACCMeas_t
{
    float accX;
    float rotation[4];
} ACCMeas_t;

/* Kalman Filter */
float dps310_stnd_height = 0.1298;    // Standardabweichung Höhenmessung
float bmi088_stnd_acc = 0.2;          // Standardabweichung Beschleunigungssensor
TaskHandle_t TaskKalmanFilter_Handle; // zum entsperen des Kalman Filter Task, wird von TaskMeasureHeight gemacht

QueueHandle_t accQueue;

float startPressure;
float startTemperature;
float pressure;
float temperature;
SemaphoreHandle_t pressure_temperature_Mutex = NULL; // Gemessen von TaskMeasureHeight und gebraucht in Kalman Filter

// FlightLogger
float lastAcc[3];
float lastGyr[3];
float lastMagn[3];

volatile bool hasMeasuredStartPressure = false;

static const char *TAG = "MainTasks";

static void IRAM_ATTR dps310ISR(void *arg)
{
    BaseType_t HPTaskWoken = pdFALSE;
    // unblock TaskMeasureAcc
    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &HPTaskWoken);
#ifndef ISSIMULATION
    if (HPTaskWoken == pdTRUE)
        portYIELD_FROM_ISR();
#endif
}

static void TaskMeasureHeight(void *parameters)
{
    // watchdog_add(NULL);
    startPressure = 0;
    startTemperature = 0;
    dps310_t DPS310_Data;
    esp_err_t err;
    ERROR_RETRYTIMES(dps310_init(&DPS310_Data, DPS310I2CPORT, 0x76), VARIO_I2C_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        // TODO: display these kinds of error messages
        error_handle(err, ERR_DPS310, "Can't initialize DPS310");
        vTaskDelete(NULL);
        return;
    }
    for (size_t i = 0; i < 10; i++)
    {
        err = dps310_get_Temperature(&DPS310_Data);
        if (err != ESP_OK)
            error_handle(err, ERR_DPS310, "Temperature reading failed");
        err = dsp310_get_Pressure(&DPS310_Data);
        if (err != ESP_OK)
            error_handle(err, ERR_DPS310, "Pressure reading failed");
        startPressure += DPS310_Data.Pressure;
        startTemperature += DPS310_Data.Temperature;
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
    startPressure /= 10;
    startTemperature /= 10;
    ESP_LOGI(TAG, "StartPressure: %.2f, StartTemp: %.2f", startPressure, startTemperature);
    hasMeasuredStartPressure = true;
    // Setup Interrupt
    gpio_config_t gpio_config_INT_DPS310 = {
        .pin_bit_mask = 1ULL << PresSens_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE};
    // set PresSens Pin as Interrupt
    gpio_config(&gpio_config_INT_DPS310);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_DPS310, "Can't configure DPS310 INT GPIO");
    }
    // Funktion bmiISR als Interrupt Funktion handle von dieser Funktion als Parameter, damit diese Funktion von der Interrupt aufgeweckt werden kann
    err = gpio_isr_handler_add((gpio_num_t)PresSens_INT, dps310ISR, xTaskGetCurrentTaskHandle());
    if (err != ESP_OK)
    {
        error_handle(err, ERR_DPS310, "Can't start INT on DPS310 GPIO");
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    int countTemperatureRead = 0;
    float lastPressure = startPressure;
    while (1)
    {
#ifdef ISSIMULATION
        vTaskDelay(30 / portTICK_PERIOD_MS);
#else
        // wait till unblocked from Interrupt
        uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, 500 / portTICK_PERIOD_MS);
        if (notificationValue == 0)
        {
            ESP_LOGW(TAG, "Interrupt didn't fire");
        }
#endif
        // watchdog_reset();
        err = dsp310_get_Pressure(&DPS310_Data);
        if (err != ESP_OK)
        {
            error_handle(err, ERR_DPS310, "Can't measure Pressure");
            continue;
        }
        if (countTemperatureRead % 34 == 0)
        {
            err = dps310_get_Temperature(&DPS310_Data);
            if (err != ESP_OK)
            {
                error_handle(err, ERR_DPS310, "Can't measure Temperature");
                // TODO: check if that happens often
                countTemperatureRead--;
            }
        }
        countTemperatureRead++;
        // Filter invalid pressure Results (200Pa = 19m -> change of 600 m in one second)
        if (fabsf(lastPressure - (float)DPS310_Data.Pressure) > 200)
        {
            ESP_LOGW(TAG, "Invalid Pressure Result! %.2f, %.2f", DPS310_Data.Pressure, lastPressure);
        }
        else
        {
            lastPressure = DPS310_Data.Pressure;
            xSemaphoreTake(pressure_temperature_Mutex, portMAX_DELAY);
            {
                pressure = DPS310_Data.Pressure;
                temperature = DPS310_Data.Temperature;
            }
            xSemaphoreGive(pressure_temperature_Mutex);
            float pressure = DPS310_Data.Pressure;
            vario_state_set_member(&pressure, vario_state_pressure);
        }
        // notify Kalman Filter of new Pressure Reading
        xTaskNotifyGiveIndexed(TaskKalmanFilter_Handle, 0);
    }
    vTaskDelete(NULL);
}

static void TaskMeasureAcc(void *parameters)
{
    // wait till StartPressure has read
    while (!hasMeasuredStartPressure)
    {
        vTaskDelay(2);
    }
    esp_err_t err;
    float rotation[4];
    /* Init BMI088 */
    bmi088_t BMI088_Data;
    ERROR_RETRYTIMES(bmi088_init(&BMI088_Data, BMI088I2CPort, 0x18, 0x68), VARIO_I2C_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_BMI088, "Can't initialize BMI088");
        vTaskDelete(NULL);
        return;
    }

    /* Init QMC588L */
    qmc588l_t *QMC588L_Data = qmc588l_getInstance();
    ERROR_RETRYTIMES(qmc588l_init(QMC588LI2CPort, 0xD, QMC588L_MR_200HZ, QMC588L_OSR_512, QMC588L_SCALE_2G), VARIO_I2C_RETRYS_INIT, err);
    if (err != ESP_OK)
    {
        error_handle(err, ERR_QMC588L, "Cant initialize QMC588L");
        vTaskDelete(NULL);
        return;
    }

    QuaternionFilter_init();
    // reset Rotation (on the head to start with)
    rotation[0] = 0.0f;
    rotation[1] = 1.0f;
    rotation[2] = 0.0f;
    rotation[3] = 0.0f;

    float lastTime = esp_timer_get_time();
    int count = 0;
    ERROR_RETRYTIMES(bmi088_ReadAccelerometer(&BMI088_Data), VARIO_I2C_RETRYS, err);
    ERROR_HANDLEBYERROR(err, ERR_BMI088, "Can't measure start Acc");

    ERROR_RETRYTIMES(bmi088_ReadGyroskop(&BMI088_Data), VARIO_I2C_RETRYS, err);
    ERROR_HANDLEBYERROR(err, ERR_BMI088, "Can't measure start Gyro");

    float lastAccX = 9.81f;
    int counterAccX = 0;
    watchdog_add(NULL);
    while (1)
    {
        watchdog_reset();
        // Clear Notification from Kalman Filter to 1 at index 1, so klaman Filter waits, till we are finished
        ulTaskNotifyValueClearIndexed(TaskKalmanFilter_Handle, 1, 0xffffffff);
        err = qmc588l_readMag();
        ERROR_HANDLEBYERROR(err, ERR_QMC588L, "QMC588L reading failed");

        err = bmi088_ReadAccelerometer(&BMI088_Data);
        ERROR_HANDLEBYERROR(err, ERR_BMI088, "Acc reading failed");
        err = bmi088_ReadGyroskop(&BMI088_Data);
        ERROR_HANDLEBYERROR(err, ERR_BMI088, "Gyro reading failed");

        float deltaT = (esp_timer_get_time() - lastTime) / 1000000;
        lastTime = esp_timer_get_time();
        /* Update Orientation */
        float an = BMI088_Data.acc_mps2[1] / 9.81;
        float ae = BMI088_Data.acc_mps2[0] / 9.81f;
        float ad = BMI088_Data.acc_mps2[2] / 9.81f;
        float gn = -BMI088_Data.gyr_rps[1];
        float ge = -BMI088_Data.gyr_rps[0];
        float gd = -BMI088_Data.gyr_rps[2];
        float mn = -QMC588L_Data->CorrectedMagnValuemG[0];
        float me = QMC588L_Data->CorrectedMagnValuemG[1];
        float md = -QMC588L_Data->CorrectedMagnValuemG[2];

        memcpy(lastAcc, BMI088_Data.acc_mps2, sizeof(lastAcc));
        memcpy(lastGyr, BMI088_Data.gyr_rps, sizeof(lastGyr));
        memcpy(lastMagn, QMC588L_Data->CorrectedMagnValuemG, sizeof(lastMagn));

        QuaternionFilter_Madgwick(an, ae, ad, gn, ge, gd, mn, me, md, deltaT, rotation);
        count++;
        // if(count % 10 == 0)
        // printf("Raw:%i,%i,%i,%i,%i,%i,%i,%i,%i\n", (int)(BMI088_Data.acc_mps2[0] * 1000), (int)(BMI088_Data.acc_mps2[1] * 1000), (int)(BMI088_Data.acc_mps2[2] * 1000), (int)BMI088_Data.gyr_rps[0], (int)BMI088_Data.gyr_rps[1], (int)BMI088_Data.gyr_rps[2], (int)QMC588L_Data->CorrectedMagnValuemG[0], (int)QMC588L_Data->CorrectedMagnValuemG[1], (int)QMC588L_Data->CorrectedMagnValuemG[2]);
        /* Calculate global Acc in X Direction */
        float accX = (2 * (rotation[1] * rotation[3] - rotation[0] * rotation[2])) * an + (2 * (rotation[2] * rotation[3] + rotation[0] * rotation[1])) * ae + (2 * (rotation[0] * rotation[0] + rotation[3] * rotation[3]) - 1) * ad;
        accX *= 9.81f;
        if (fabsf(accX - lastAccX) > 10 && counterAccX < 3)
        {
            counterAccX++;
            ESP_LOGE(TAG, "Can't accept this Acc %.2f", accX);
        }
        else
        {
            counterAccX = 0;
            lastAccX = accX;

            ACCMeas_t tempAccMeas;
            tempAccMeas.accX = accX;
            memcpy(tempAccMeas.rotation, rotation, sizeof(float) * 4);
            xQueueSend(accQueue, &tempAccMeas, (TickType_t)0);
        }
        xTaskNotifyGiveIndexed(TaskKalmanFilter_Handle, 1);
#ifdef ISSIMULATION
        vTaskDelay(30 / portTICK_PERIOD_MS);
#else
        // wait for 1 tick
        vTaskDelay((TickType_t)1);
#endif
    }
    vTaskDelete(NULL);
}
static void ResetKalman(kalman_t *kf)
{
    kalman_filter_reset(kf);
    // kalman_filter_reset(kf);
    float deltaT = 0.025;
    /* Set State Transition Matrix */
    /* F = 1 t
           0 1 */
    matrix_t *F = &kf->F;
    matrix_set(F, 0, 0, 1);
    matrix_set(F, 0, 1, deltaT);
    matrix_set(F, 1, 0, 0);
    matrix_set(F, 1, 1, 1);
    /* Set Control Matrix */
    /* G = 0.5*t^2
           t        */
    matrix_t *G = &kf->G;
    matrix_set(G, 0, 0, 0.5 * deltaT * deltaT);
    matrix_set(G, 1, 0, deltaT);
    /* Set  Process Noise Matrix*/
    /* Q = (t^4)/4  (t^3)/2
           (t^3)/2   t^2       * kalman_process_noise^2*/
    matrix_t *Q = &kf->Q;
    matrix_set(Q, 0, 0, ((deltaT * deltaT * deltaT * deltaT) / 4) * bmi088_stnd_acc * bmi088_stnd_acc);
    matrix_set(Q, 0, 1, ((deltaT * deltaT * deltaT) / 2) * bmi088_stnd_acc * bmi088_stnd_acc);
    matrix_set(Q, 1, 0, ((deltaT * deltaT * deltaT) / 2) * bmi088_stnd_acc * bmi088_stnd_acc);
    matrix_set(Q, 1, 1, (deltaT * deltaT) * bmi088_stnd_acc * bmi088_stnd_acc);
    /* Set Observation Matrix */
    /* H = [1   0] */
    matrix_t *H = &kf->H;
    matrix_set(H, 0, 0, 1);
    matrix_set(H, 0, 1, 0);
    /* Set Measurement covariance matrix */
    matrix_t *R = &kf->R;
    matrix_set(R, 0, 0, dps310_stnd_height * dps310_stnd_height);
    /* Set Estimate Uncertainty Matrix*/
    /* P = 500   0
            0   500 */
    matrix_t *P = &kf->P;
    matrix_set(P, 0, 0, 500);
    matrix_set(P, 0, 1, 0);
    matrix_set(P, 1, 0, 0);
    matrix_set(P, 1, 1, 500);
    /* Set  Input Vector*/
    /* u = (a + g) */
    matrix_t *u = &kf->u;
    matrix_set(u, 0, 0, 0);
    /* Initialize State Vector */
    matrix_t *x = &kf->x;
    matrix_set(x, 0, 0, 0);
    matrix_set(x, 1, 0, 0);
}

static void TaskKalmanFilter(void *parameters)
{
    // wait till StartPressure has read
    while (!hasMeasuredStartPressure)
    {
        vTaskDelay(200 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Wait Klaman Filter");
    }
    kalman_t kf;
    kalman_filter_init(&kf);
    ResetKalman(&kf);
    kalman_predict(&kf);
    float deltaT = 0.025;
    matrix_t *F = &kf.F;
    matrix_t *G = &kf.G;
    matrix_t *Q = &kf.Q;
    matrix_t *u = &kf.u;
    matrix_t *x = &kf.x;
    matrix_t *z = &kf.z;
    float lastTime = esp_timer_get_time();
    int countHeading = 0;
    bool wasGreater = false;
    ESP_LOGI(TAG, "Start Kalman Filter");
    while (1)
    {
        // wait for new Result from Pressure Sensor
        uint32_t notificationValue = ulTaskNotifyTakeIndexed(0, pdTRUE, portMAX_DELAY);
        if (notificationValue > 1)
        {
            ESP_LOGW(TAG, "Task Notification higher than 1, value: %lu", (unsigned long)notificationValue);
            continue;
        }
        // wait, till acc Reading has finished
        ulTaskNotifyTakeIndexed(1, pdTRUE, portMAX_DELAY);

        FlightLogger_t flight_state;
        memcpy(flight_state.acc, lastAcc, sizeof(lastAcc));
        memcpy(flight_state.gyr, lastGyr, sizeof(lastGyr));
        memcpy(flight_state.magn, lastMagn, sizeof(lastMagn));

        float heightDifferenz;
        xSemaphoreTake(pressure_temperature_Mutex, portMAX_DELAY);
        {
            // Temp. measurement not working... set to default
            startTemperature = 20;
            heightDifferenz = ((startTemperature + 273.15) / TEMPGRAD_L) * (pow(pressure / startPressure, -TEMPGRAD_L * R_SPECIFIC / GRAVITY_G) - 1);
            flight_state.pressure = pressure;
            flight_state.temperature = startTemperature;
        }
        xSemaphoreGive(pressure_temperature_Mutex);
        float acc = 0;

        ACCMeas_t tempAcc;
        float accSum = 0;
        int accSize = 0;
        while (xQueueReceive(accQueue, &tempAcc, (TickType_t)0) == pdTRUE)
        {
            accSum += tempAcc.accX;
            accSize++;
        }
        if (accSize == 0)
        {
            ESP_LOGW(TAG, "Cant recieve Acc");
            acc = 9.81f;
        }
        else
            acc = accSum / accSize;
        deltaT = (esp_timer_get_time() - lastTime) / 1000000;

        /* Set Q with new deltaT */
        matrix_set(Q, 0, 0, ((deltaT * deltaT * deltaT * deltaT) / 4) * bmi088_stnd_acc * bmi088_stnd_acc);
        matrix_set(Q, 0, 1, ((deltaT * deltaT * deltaT) / 2) * bmi088_stnd_acc * bmi088_stnd_acc);
        matrix_set(Q, 1, 0, ((deltaT * deltaT * deltaT) / 2) * bmi088_stnd_acc * bmi088_stnd_acc);
        matrix_set(Q, 1, 1, (deltaT * deltaT) * bmi088_stnd_acc * bmi088_stnd_acc);
        /* Set G with new deltaT */
        matrix_set(G, 0, 0, 0.5 * deltaT * deltaT);
        matrix_set(G, 1, 0, deltaT);
        /* Set F with new deltaT */
        matrix_set(F, 0, 0, 1);
        matrix_set(F, 0, 1, deltaT);
        matrix_set(F, 1, 0, 0);
        matrix_set(F, 1, 1, 1);
        /* Set z measurement */
        matrix_set(z, 0, 0, heightDifferenz);
        /* Set Input */
        matrix_set(u, 0, 0, acc - 9.81);
        kalman_update(&kf);
        if (fabs(matrix_get(x, 1, 0)) > 100.0)
        {
            ESP_LOGE(TAG, "Was greater 1");
        }
        kalman_predict(&kf);
        if (fabs(matrix_get(x, 1, 0)) > 100.0)
        {
            ESP_LOGE(TAG, "Was greater 2");
        }

        double vertVel = matrix_get(x, 1, 0);
        if (isnanf(vertVel))
        {
            // not working
            ESP_LOGI(TAG, "A: %.2f, P: %.2f", acc, pressure);
            ESP_LOGI(TAG, "Had to reset Kalman Filter\n");
            ResetKalman(&kf);
            continue;
        }
        vario_state_set_member(&vertVel, vario_state_vertVel);
        float accXCorr = acc - 9.81;
        vario_state_set_member(&accXCorr, vario_state_vertAcc);
        vario_state_set_member(&heightDifferenz, vario_state_altitudeBaro);
        if (countHeading % 10 == 0)
        {
            float roll, pitch, yaw;
            Quaternion_get_RPY(tempAcc.rotation[0], tempAcc.rotation[1], tempAcc.rotation[2], tempAcc.rotation[3], &roll, &pitch, &yaw);
            int heading = (int)yaw;
            while (heading > 360.0)
                heading -= 360.0;
            while (heading < 0.0)
                heading += 360.0;
            vario_state_set_member(&heading, vario_state_heading);
        }
        logVarioData(&flight_state);
        SoundManager_calculateNewBiep();
        // printf("Vel%.2f\n", vertVel);
        // // State Vector
        // static volatile float x_Buffer[2 * 1];
        // memcpy(x_Buffer, kf.x_Buffer, sizeof(x_Buffer));
        // // Output Vector
        // static volatile float z_Buffer[1 * 1];
        // memcpy(z_Buffer, kf.z_Buffer, sizeof(z_Buffer));
        // // State Transition Matrix
        // static volatile float F_Buffer[2 * 2];
        // memcpy(F_Buffer, kf.F_Buffer, sizeof(F_Buffer));
        // // Input Variable
        // static volatile float u_Buffer[1 * 1];
        // memcpy(u_Buffer, kf.u_Buffer, sizeof(u_Buffer));
        // // Control Matrix
        // static volatile float G_Buffer[2 * 1];
        // memcpy(G_Buffer, kf.G_Buffer, sizeof(G_Buffer));
        // // Estimate Uncertainty
        // static volatile float P_Buffer[2 * 2];
        // memcpy(P_Buffer, kf.P_Buffer, sizeof(P_Buffer));
        // // Process Noise Uncertainty
        // static volatile float Q_Buffer[2 * 2];
        // memcpy(Q_Buffer, kf.Q_Buffer, sizeof(Q_Buffer));
        // // Measurement Uncertainty
        // static volatile float R_Buffer[1 * 1];
        // memcpy(R_Buffer, kf.R_Buffer, sizeof(R_Buffer));
        // // Oberservation Matrix
        // static volatile float H_Buffer[1 * 2];
        // memcpy(H_Buffer, kf.H_Buffer, sizeof(H_Buffer));
        // // Kalman Gain
        // static volatile float K_Buffer[2 * 1];
        // memcpy(K_Buffer, kf.K_Buffer, sizeof(K_Buffer));
        // // Residual covariance matrix
        // static volatile float S_Buffer[1 * 1];
        // memcpy(S_Buffer, kf.S_Buffer, sizeof(S_Buffer));
        // // Innovativ vector
        // static volatile float y_Buffer[1 * 1];
        // memcpy(y_Buffer, kf.y_Buffer, sizeof(y_Buffer));
        printf("Kal: %.2fv, %.2fm, %.2fa, %ims\n", vertVel, heightDifferenz, acc, (int)(deltaT * 1000.0f));
        lastTime = esp_timer_get_time();
    }
    vTaskDelete(NULL);
}

void startMainTasks()
{
    pressure_temperature_Mutex = xSemaphoreCreateMutex();
    accQueue = xQueueCreate(10, sizeof(ACCMeas_t));
#ifdef ISSIMULATION
    xTaskCreate(TaskKalmanFilter, "Kalman Filter", 4 * 1024, NULL, TASK_KALMAN_PRIORITY, &TaskKalmanFilter_Handle);
    xTaskCreate(TaskMeasureHeight, "Measure Height", 4 * 1024, NULL, TASK_HEIGHT_PRIORITY, NULL);
    xTaskCreate(TaskMeasureAcc, "Measure Acceleration", 4 * 1024, NULL, TASK_ACC_PRIORITY, NULL);
#else
    xTaskCreatePinnedToCore(TaskKalmanFilter, "Kalman Filter", 4 * 1024, NULL, TASK_KALMAN_PRIORITY, &TaskKalmanFilter_Handle, 0);
    xTaskCreatePinnedToCore(TaskMeasureHeight, "Measure Height", 4 * 1024, NULL, TASK_HEIGHT_PRIORITY, NULL, 0);
    xTaskCreatePinnedToCore(TaskMeasureAcc, "Measure Acceleration", 4 * 1024, NULL, TASK_ACC_PRIORITY, NULL, 0);
#endif
}
