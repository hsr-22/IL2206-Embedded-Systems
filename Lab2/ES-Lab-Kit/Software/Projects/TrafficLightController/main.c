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
#include "bsp.h"

/*************************************************************/

/**
 * @brief Main function.
 * 
 * @return int 
 */
int main()
{
    BSP_Init();             /* Initialize all components on the lab-kit. */
    
    while (true) { 
        // Red for 3s
        BSP_SetLED(LED_RED, true);
        BSP_SetLED(LED_YELLOW, false);
        BSP_SetLED(LED_GREEN, false);
        sleep_ms(3000);

        // Red - Yellow for 1s
        BSP_SetLED(LED_RED, true);
        BSP_SetLED(LED_YELLOW, true);
        BSP_SetLED(LED_GREEN, false);
        sleep_ms(1000);

        // Green for 3s
        BSP_SetLED(LED_RED, false);
        BSP_SetLED(LED_YELLOW, false);
        BSP_SetLED(LED_GREEN, true);
        sleep_ms(3000);

        // Yellow for 1s
        BSP_SetLED(LED_RED, false);
        BSP_SetLED(LED_YELLOW, true);
        BSP_SetLED(LED_GREEN, false);
        sleep_ms(1000);
    }
}
/*-----------------------------------------------------------*/
