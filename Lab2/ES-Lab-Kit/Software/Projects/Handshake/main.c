/**
 * @file main.c
 * @author Harsh Roniyar (roniyar@kth.se)
 * 
 * @version 0.1
 * @date 2025-10-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bsp.h"

#define STEP_TIME_MS 2000
#define STEP_TICKS   pdMS_TO_TICKS(STEP_TIME_MS)

SemaphoreHandle_t semRedDone;
SemaphoreHandle_t semGreenDone;

void vTaskGreen(void *pvParameters);
void vTaskRed(void *pvParameters);

int main(void)
{
    BSP_Init();             /* Initialize all components on the lab-kit. */

    semRedDone  = xSemaphoreCreateBinary();
    semGreenDone = xSemaphoreCreateBinary();

    /* Check semaphore creation */
    configASSERT(semRedDone  != NULL);
    configASSERT(semGreenDone != NULL);

    /* Initial state: both LEDs ON for the first visible step */
    BSP_SetLED(LED_RED, true);
    BSP_SetLED(LED_GREEN, true);

    xTaskCreate(vTaskRed, "RedTask",   256, NULL, 1, NULL);
    xTaskCreate(vTaskGreen, "GreenTask", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    /* Should not reach here */
    for (;;) {}
}

/*-----------------------------------------------------------*/

void vTaskRed(void *pvParameters)
{
    /* pattern: 4 steps, repeat
       Red:  On, On, Off, Off
    */
    const bool redState[4]   = { true,  true,  false, false };
    const bool greenState[4] = { true,  false, false, true  }; /* for documentation only */

    TickType_t xLastWakeTime = xTaskGetTickCount();
    size_t idx = 0;

    for (;;) {
        /* Set red LED according to pattern */
        BSP_SetLED(LED_RED, redState[idx]);

        /* Wait exactly STEP_TIME_MS since previous wake */
        vTaskDelayUntil(&xLastWakeTime, STEP_TICKS);

        /* Notify green task that red period finished */
        xSemaphoreGive(semRedDone);

        /* Wait for green to finish its period */
        xSemaphoreTake(semGreenDone, portMAX_DELAY);

        /* advance index */
        idx = (idx + 1) % 4;
    }
}

/*-----------------------------------------------------------*/

void vTaskGreen(void *pvParameters)
{
    /* pattern: 4 steps, repeat
       Green: On, Off, Off, On
    */
    const bool greenState[4] = { true,  false, false, true  };

    TickType_t xLastWakeTime = xTaskGetTickCount();
    size_t idx = 0;

    for (;;) {
        BSP_SetLED(LED_GREEN, greenState[idx]);

        /* Wait exactly STEP_TIME_MS since previous wake */
        vTaskDelayUntil(&xLastWakeTime, STEP_TICKS);

        /* Notify red task that green period finished */
        xSemaphoreGive(semGreenDone);

        /* Wait for red to finish its period */
        xSemaphoreTake(semRedDone, portMAX_DELAY);

        idx = (idx + 1) % 4;
    }
}
