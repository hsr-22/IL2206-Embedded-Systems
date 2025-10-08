/* Consult the RP2350 Datasheet for more information on
 *    - GPIO Interrupt (Section 9.5 and 9.10)
 *    - System Timers (Section 12.8)
 */

#include <stdio.h>
#include "bsp.h"

void gpio_callback(uint gpio, uint32_t events) {
    uint64_t now;

    now = time_us_64(); /* Returns the current time value in us */    
    BSP_ToggleLED(LED_GREEN);
    printf("Button Interrupt on GPIO %d at %lld\n", gpio, now);
}

bool repeating_timer_callback(__unused struct repeating_timer *t) {
    uint64_t now;

    now = time_us_64(); /* Returns the current time value in us */
    printf("Repeating Timer Interrupt at %lld\n", now);
            
    BSP_ToggleLED(LED_RED);
    return true;
}

int main() {
    BSP_Init(); /* Initialises ES-Lab-Kit */

    printf("Demo: GPIO and Timer Interrupt\n");

    /* Demo: Button Interrupt */
    gpio_set_irq_enabled_with_callback(
        SW_5,               /* Enable SW_5 for Interrupt*/
        GPIO_IRQ_EDGE_FALL, /* IRQ on Falling Edge */
        true,               /* Interrupt enabled */
        &gpio_callback);    /* Callback Function */


    /* Demo: Repeating Timer Interrupt */
    struct repeating_timer timer;
    add_repeating_timer_ms(
        1000,                     /* Delay in ms */
        repeating_timer_callback, /* Callback function */
        NULL,        /* Pass data to callback function */
        &timer);     /* Pointer to timer structure */
    
    while (1) { ; }  
}

