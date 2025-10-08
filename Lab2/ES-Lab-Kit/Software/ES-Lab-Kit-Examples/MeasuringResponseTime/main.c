#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "bsp.h"
#include "hardware/clocks.h"

TaskHandle_t    xTask1_handle; /* Handle for the task 1. */

/** 
 * @brief Wait function in milliseconds (CPU clock).
 * 
 * @param args Number of milliseconds (uint32_t).
 */
void wait_cpu_ms(uint32_t n) {

    const int time_1ms = clock_get_hz(clk_sys) / 1e3;

    if (n > 0) {
        BSP_WaitClkCycles(time_1ms * n);
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Task 1
 * 
 * @param args Period in ticks
 */
void vTask1(void *args) {
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    TickType_t release, completion;
    uint32_t response;
    uint32_t wcrt = 0;

    for (;;) {
        release = xLastWakeTime;
        /* Simulate Computation with estimated WCET*/ 
        wait_cpu_ms(100);
        
        completion = xTaskGetTickCount();
        response = completion - release;
        if (response > wcrt) {
            wcrt = response;
        }

        printf("Task 1 - Release: %6.3f, Completion: %6.3f, Response Time: %4.3f, WCRT: %4.3f", 
            ((float) pdTICKS_TO_MS(release) / 1000), 
            ((float) pdTICKS_TO_MS(completion) / 1000),
            ((float) pdTICKS_TO_MS(response) / 1000),
            ((float) pdTICKS_TO_MS(wcrt) / 1000));
        if (response > xPeriod) {
            printf(" ==> Deadline violated!");
        }    
        printf("\n");    
        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}

/*************************************************************/

/**
 * @brief Main function.
 * 
 * @return int 
 */
int main()
{
    BSP_Init();             /* Initialize all components on the lab-kit. */
    
    /* Create the tasks. */
    xTaskCreate(vTask1,          /* Pointer to task function */ 
                "Task 1",        /* Name of the task */
                512,             /* Stack depth in words */
                (void*) 400,     /* Task parameter, here period */ 
                3,               /* Task Priority */
                &xTask1_handle); /* Task Handle */

    vTaskStartScheduler();  /* Start the scheduler. */
    
    return 0;
}

/*-----------------------------------------------------------*/

