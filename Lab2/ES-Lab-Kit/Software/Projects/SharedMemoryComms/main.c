/**
 * @file main.c
 * @author Harsh Roniyar (roniyar@kth.se)
 * 
 * @version 0.1
 * @date 2025-10-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "bsp.h"

#define PRODUCE_PERIOD_MS 500u
#define PRODUCE_PERIOD_TICKS pdMS_TO_TICKS(PRODUCE_PERIOD_MS)

static int sharedAddress = 0; /* protected by xMutex */

static SemaphoreHandle_t xMutex = NULL;
static SemaphoreHandle_t xSemaphoreA = NULL; /* B -> A: processed */
static SemaphoreHandle_t xSemaphoreB = NULL; /* A -> B: new data */

static TaskHandle_t xTaskAHandle = NULL;
static TaskHandle_t xTaskBHandle = NULL;

void vTaskA(void *pvParameters);
void vTaskB(void *pvParameters);

int main(void)
{
    BSP_Init();

    /* Create synchronization objects before tasks start */
    xMutex      = xSemaphoreCreateMutex();
    xSemaphoreA = xSemaphoreCreateBinary();
    xSemaphoreB = xSemaphoreCreateBinary();

    /* Initial state: semaphores should be empty (A will give semB when first data is ready) */
    /* Create tasks (store handles so we can inspect / control later) */
    xTaskCreate(vTaskA, "TaskA", 256, NULL, 1, &xTaskAHandle);
    xTaskCreate(vTaskB, "TaskB", 256, NULL, 1, &xTaskBHandle);

    vTaskStartScheduler();

    /* Should not reach here */
    for (;;) {}
}

/* Task A: Producer & printer */
void vTaskA(void *pvParameters)
{
    TickType_t xLastWake = xTaskGetTickCount();
    int number = 1;

    for (;;)
    {
        /* Keep to a precise period */
        vTaskDelayUntil(&xLastWake, PRODUCE_PERIOD_TICKS);

        /* Write the shared location (hold mutex only while writing) */
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            sharedAddress = number;
            printf("Sending   : %d\n", sharedAddress);
            xSemaphoreGive(xMutex);
        }

        /* Notify B that new data is ready */
        xSemaphoreGive(xSemaphoreB);

        /* Wait for B to process and notify back */
        xSemaphoreTake(xSemaphoreA, portMAX_DELAY);

        /* Read (peek) the processed result, hold mutex only for read */
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            printf("Receiving : %d\n", sharedAddress);
            xSemaphoreGive(xMutex);
        }

        number++;
    }
}

/* Task B: Consumer & processor */
void vTaskB(void *pvParameters)
{
    for (;;)
    {
        /* Wait for data from A */
        xSemaphoreTake(xSemaphoreB, portMAX_DELAY);

        /* Process only while holding the mutex for the minimal time */
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            /* in-place processing*/
            sharedAddress *= -1;
            xSemaphoreGive(xMutex);
        }

        /* Notify A processing is done */
        xSemaphoreGive(xSemaphoreA);
    }
}
