#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bsp.h"

TaskHandle_t    xTask1_handle; /* Handle for task 1. */
TaskHandle_t    xTask2_handle; /* Handle for task 2. */
SemaphoreHandle_t xMutex;

/* Shared global variable */
uint32_t x = 0;

void vTask1(void *args) { /* Periodic Task */
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */

    for (;;) {
        xSemaphoreTake(xMutex, ( TickType_t ) portMAX_DELAY);
        x++;
        printf("Task 1: x = %d\n", x);
        xSemaphoreGive(xMutex);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}
/*-----------------------------------------------------------*/

void vTask2(void *args) { /* Periodic Task */
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    
    for (;;) {
        xSemaphoreTake(xMutex, ( TickType_t ) portMAX_DELAY);
        x *= 2;
        printf("  ==> Task 2: x = %d\n", x);
        xSemaphoreGive(xMutex);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */        
    }
}

/*-----------------------------------------------------------*/
int main()
{
    BSP_Init();             /* Initialize all components on the lab-kit. */
    
    /* Create the tasks. */
    xTaskCreate(vTask1, "Task 1", 512, (void*) 1000, 2, &xTask1_handle);
    xTaskCreate(vTask2, "Task 2", 512, (void*) 2000, 1, &xTask2_handle);
    
    /* Create Semaphore */
    xMutex=xSemaphoreCreateMutex();

    vTaskStartScheduler();  /* Start the scheduler. */
    
    return 0; /* Should not reach here... */
}
/*-----------------------------------------------------------*/
