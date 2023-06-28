#include "Simulation.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>

static TaskHandle_t taskHandleSimulation;

static void SimulationTask(void *args)
{
    
}

void start_Simulation()
{
    xTaskCreate(SimulationTask, "SimulationTask", 1024 * 2, NULL, 10, &taskHandleSimulation);
}

void end_Simulation()
{
    vTaskDelete(taskHandleSimulation);
}