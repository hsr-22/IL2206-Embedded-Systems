#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"

TaskHandle_t    blinkTsk; /* Handle for the LED task. */

/**
 * @brief Blink task.
 * 
 * @param args Task period (uint32_t).
 */
void blink_task(void *args);

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
    xTaskCreate(blink_task,   /* Pointer to task function */
                "Blink Task", /* Name of the task */
                512,          /* Stack depth in words */
                (void*) 1000, /* Task parameter, here period */ 
                2,            /* Task Priority */
                &blinkTsk);   /* Task Handle */
    
    vTaskStartScheduler();  /* Start the scheduler. */
    
    while (true) { 
        sleep_ms(1000); /* Should not reach here... */
    }
}
/*-----------------------------------------------------------*/

void blink_task(void *args) {
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    for (;;) {
        BSP_ToggleLED(LED_GREEN);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}
/*-----------------------------------------------------------*/
