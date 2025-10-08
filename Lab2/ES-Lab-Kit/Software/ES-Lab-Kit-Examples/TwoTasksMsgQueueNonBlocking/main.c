#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "bsp.h"

TaskHandle_t  xTask1_handle; /* Handle for task 1. */
TaskHandle_t  xTask2_handle; /* Handle for task 2. */
QueueHandle_t xQueue;

void vTask1(void *args) { /* Periodic Task */
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    uint32_t x = 0;

    for (;;) {
        xQueueOverwrite(xQueue, &x);
        printf("Task 1: x = %d\n", x);
        x++;

        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}
/*-----------------------------------------------------------*/

void vTask2(void *args) { /* Periodic Task */
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    uint32_t y;

    for (;;) {
        xQueueReceive(xQueue, &y, (TickType_t) portMAX_DELAY);
        printf("  ==> Task 2: y = %d\n", y);

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
    xQueue=xQueueCreate(1, sizeof(uint32_t));

    vTaskStartScheduler();  /* Start the scheduler. */
    
    return 0;
}
/*-----------------------------------------------------------*/