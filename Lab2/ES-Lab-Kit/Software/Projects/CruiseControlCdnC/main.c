/**
 * @file main.c
 * @author Harsh Roniyar (roniyar@kth.se)
 * 
 * @version 0.1
 * @date 2025-10-08
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
#include "hardware/clocks.h"

#define GAS_STEP 2  /* Defines how much the throttle is increased if GAS_STEP is asserted */

/* Definition of handles for tasks */
TaskHandle_t    xButton_handle; /* Handle for the Button task */
TaskHandle_t    xControl_handle; /* Handle for the Control task */
TaskHandle_t    xVehicle_handle; /* Handle for the Vehicle task */
TaskHandle_t    xDisplay_handle; /* Handle for the Display task */
TaskHandle_t    xWatchDog_handle; /* Handle for the Watchdog task */
TaskHandle_t    xOverload_handle; /* Handle for the OverloadDetection task */
TaskHandle_t    xExtraLoad_handle; /* Handle for the ExtraLoad task */

/* Definition of handles for queues */
QueueHandle_t xQueueVelocity;
QueueHandle_t xQueuePosition;
QueueHandle_t xQueueThrottle;
QueueHandle_t xQueueTargetVelocity;
QueueHandle_t xQueueCruiseControl;
QueueHandle_t xQueueGasPedal;
QueueHandle_t xQueueBrakePedal;

/* ---------------- Part 3 globals ------------------ */
/* Watchdog semaphore (OK token) */
SemaphoreHandle_t xSemaphoreWatchDogFood = NULL;

/* Part3 constants */
#define OVERLOAD_WATCHDOG_TIMEOUT_MS 1000u
#define OVERLOAD_DETECT_PERIOD_MS   100u
#define EXTRA_LOAD_PERIOD_MS        25u

/* Forward prototypes for new tasks */
void vWatchDogTask(void *arg);
void vOverloadDetectionTask(void *arg);
void vExtraLoadTask(void *arg);

/**
 * @brief The button task shall monitor the input buttons and send the values to the
 *        other tasks 
 * 
 * ==> MODIFY THIS TASK! 
 *     Currently the buttons are ignored. Use busy wait I/O to monitor the buttons
 * ==> MODIFIED: Now periodically reads button inputs and sends them via queues
 * @param args 
 */
void vButtonTask(void *args) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = (int)args;   /* Period in ticks */

    bool btnGas;
    bool btnBrake;
    bool prev_btnCruise = BSP_GetInput(SW_6); /* raw read, active-low: pressed -> 0 */
    bool value_cruise_control = false;

    uint16_t now_velocity = 0;

    for (;;) {
        /* Busy-wait read of hardware button states */
        bool raw_sw7 = BSP_GetInput(SW_7); /* GAS, active-low */
        bool raw_sw5 = BSP_GetInput(SW_5); /* BRAKE, active-low */
        bool raw_sw6 = BSP_GetInput(SW_6); /* CRUISE, raw */

        /* Convert to logical pressed booleans (pressed == true) */
        btnGas = !raw_sw7;
        btnBrake = !raw_sw5;

        /* Edge detect negative-edge of cruise button (pressed -> raw goes from 1->0) */
        if ((raw_sw6 != prev_btnCruise) && (raw_sw6 == false)) {
            /* at negative edge: toggle cruise state */
            value_cruise_control = !value_cruise_control;

            /* Snapshot current velocity as target */
            xQueuePeek(xQueueVelocity, &now_velocity, (TickType_t)0);
            xQueueOverwrite(xQueueTargetVelocity, &now_velocity);
            
        }

        prev_btnCruise = raw_sw6;

        /* Brake cancels cruise immediately */
        if (btnBrake) {
            value_cruise_control = false;
        }

        /* Send values to respective queues */
        xQueueOverwrite(xQueueGasPedal,&btnGas);
        xQueueOverwrite(xQueueBrakePedal,&btnBrake);
        xQueueOverwrite(xQueueCruiseControl,&value_cruise_control);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Periodic execution */
    }
}

/**
 * @brief The control tasks calculates the new throttle using your 
 *        control algorithm and the current values.
 * 
 * ==> MODIFY THIS TASK!
 *     Currently the throttle has a fixed value of 80
 * ==> MODIFIED: Now periodic, and Proportional controller for throttle adjustment
 * @param args 
 */
void vControlTask(void *args) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = (int)args;

    uint16_t throttle = 0;
    uint16_t velocity = 0;
    uint16_t target_velocity = 0;
    bool cruise_request = false;
    bool cruise_active = false;
    bool gas_pedal = false;
    bool brake_pedal = false;

    const uint16_t VELOCITY_CRUISE_THRESHOLD = 250; /* Minimum velocity for cruise control to be active */

    for (;;) {
        xQueuePeek(xQueueCruiseControl, &cruise_request, (TickType_t)0);
        xQueuePeek(xQueueGasPedal, &gas_pedal, (TickType_t)0);
        xQueuePeek(xQueueBrakePedal, &brake_pedal, (TickType_t)0);
        xQueuePeek(xQueueVelocity, &velocity, (TickType_t)0);
        xQueuePeek(xQueueTargetVelocity, &target_velocity, (TickType_t)0);

        /* Cruise control toggle logic */
        if (brake_pedal || gas_pedal) {
            /* brakes or gas deactivate cruise */
            cruise_active = false;
        } else {
            /* user requested cruise and velocity threshold satisfied */
            if (cruise_request && (velocity >= VELOCITY_CRUISE_THRESHOLD)) {
                if (!cruise_active) {
                    cruise_active = true;
                    /* snapshot current velocity as target */
                    target_velocity = velocity;
                    xQueueOverwrite(xQueueTargetVelocity, &target_velocity);
                }
            } else {
                cruise_active = false;
            }
        }
        
        if (brake_pedal) {
            throttle = 0;
        }
        else if (gas_pedal) {
            throttle += GAS_STEP;
            if (throttle > 80)
                throttle = 80;
        }
        else if (cruise_active) {
            int16_t err = (int16_t)target_velocity - (int16_t)velocity;
            /* simple proportional step adjust: */
            if (err > 40) throttle = (throttle + 3 > 80) ? 80 : throttle + 3;
            else if (err < -40) throttle = (throttle > 3) ? throttle - 3 : 0;
            /* else keep throttle */
        } else {
            throttle = 0;
        }

        /* Set yellow LED for cruise active */
        BSP_SetLED(LED_YELLOW, cruise_active);

        xQueueOverwrite(xQueueThrottle, &throttle);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/**
 * @brief The function returns the new position depending on the input parameters.
 * 
 * ==> DO NOT CHANGE THIS FUNCTION !!!
 * 
 * @param position 
 * @param velocity 
 * @param acceleration 
 * @param time_interval 
 * @return 
 */
uint16_t adjust_position(uint16_t position, int16_t velocity,
                         int8_t acceleration, uint16_t time_interval)
{
  int16_t new_position = position + velocity * time_interval / 1000
    + acceleration / 2  * (time_interval / 1000) * (time_interval / 1000);

  if (new_position > 24000) {
    new_position -= 24000;
  } else if (new_position < 0){
    new_position += 24000;
  }

  return new_position;
}


/**
 * @brief The function returns the new velocity depending on the input parameters.
 * 
 * ==> DO NOT CHANGE THIS FUNCTION !!! 
 *
 * @param velocity 
 * @param acceleration 
 * @param brake_pedal 
 * @param time_interval 
 * @return 
 */
int16_t adjust_velocity(int16_t velocity, int8_t acceleration,  
		       bool brake_pedal, uint16_t time_interval)
{
  int16_t new_velocity;
  uint8_t brake_retardation = 50;

  if (brake_pedal == false) {
    new_velocity = velocity  + (float) ((acceleration * time_interval) / 1000);
    if (new_velocity <= 0) {
        new_velocity = 0;
    }
  } 
  else { 
    if ((float) (brake_retardation * time_interval) / 1000 > velocity) {
       new_velocity = 0;
    }
    else {
      new_velocity = velocity - (float) brake_retardation * time_interval / 1000;
    }
  } 

  return new_velocity;
}

/**
 * @brief The vehicle task continuously calculates the velocity of the vehicle 
 *
 * ==> DO NOT CHANGE THIS TASK !!!  
 *
 * @param args 
 */
void vVehicleTask(void *args) {
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    uint16_t throttle;
    bool brake_pedal;
                           /* Approximate values*/
    uint8_t acceleration;  /* Value between 40 and -20 (4.0 m/s^2 and -2.0 m/s^2) */
    uint8_t retardation;   /* Value between 20 and -10 (2.0 m/s^2 and -1.0 m/s^2) */
    uint16_t position = 0; /* Value between 0 and 24000 (0.0 m and 2400.0 m)  */
    uint16_t velocity = 0; /* Value between -200 and 700 (-20.0 m/s amd 70.0 m/s) */
    uint16_t wind_factor;   /* Value between -10 and 20 (2.0 m/s^2 and -1.0 m/s^2) */

    for (;;) {
        xQueuePeek(xQueueThrottle, &throttle, ( TickType_t ) 0);
        xQueuePeek(xQueueBrakePedal, &brake_pedal, ( TickType_t ) 0);

        /* Retardation : Factor of Terrain and Wind Resistance */
        if (velocity > 0)
	        wind_factor = velocity * velocity / 10000 + 1;
        else 
	        wind_factor = (-1) * velocity * velocity / 10000 + 1;

        if (position < 4000) 
            retardation = wind_factor; // even ground
        else if (position < 8000)
            retardation = wind_factor + 8; // traveling uphill
        else if (position < 12000)
            retardation = wind_factor + 16; // traveling steep uphill
        else if (position < 16000)
            retardation = wind_factor; // even ground
        else if (position < 20000)
            retardation = wind_factor - 8; //traveling downhill
        else
            retardation = wind_factor - 16 ; // traveling steep downhill

        acceleration = throttle / 2 - retardation;	  
        position = adjust_position(position, velocity, acceleration, xPeriod); 
        velocity = adjust_velocity(velocity, acceleration, brake_pedal, xPeriod);         

 
        xQueueOverwrite(xQueueVelocity, &velocity);
        xQueueOverwrite(xQueuePosition, &position); 
        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}

/**
 * @brief Function to write the position on the 24 LEDs
 * 
 * @param position The position to display
 */
void write_position(uint16_t position) {
    uint32_t led_reg = 0u;
    int led_index = (position / 1000u); /* 0..23 */
    if (led_index < 0) led_index = 0;
    if (led_index > 23) led_index = 23;

    led_reg = (1u << led_index); /* a single bit set */

    /* LSB-first byte order as required by BSP_ShiftRegWriteAll */
    uint8_t bytes[3];
    bytes[2] = (uint8_t)((led_reg >> 16) & 0xFFu);
    bytes[1] = (uint8_t)((led_reg >> 8) & 0xFFu);
    bytes[0] = (uint8_t)((led_reg) & 0xFFu);

    BSP_ShiftRegWriteAll(bytes);
}

/**
 * @brief The display task shall show the information on 
 *          - the throttle and velocity on the seven segment display
 *          - the position on the 24 LEDs
 * 
 * ==> MODIFY THIS TASK!
 *     Currently the information is shown on the standard output (serial monitor in VSCode)
 * ==> MODIFIED: Uses LEDs and 7-segment display for output
 * @param args 
 */
void vDisplayTask(void *args) {
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */

    uint16_t velocity; 
    uint16_t throttle;  
    uint16_t position;
    bool gas_pedal;
    bool brake_pedal;
    bool cruise_control;
    char display_str[9];   /* enough for "TTVV\0" and extras */

    for (;;) {
        xQueuePeek(xQueueVelocity, &velocity, ( TickType_t ) 0);
        xQueuePeek(xQueuePosition, &position, ( TickType_t ) 0);
        xQueuePeek(xQueueThrottle, &throttle, ( TickType_t ) 0);
        xQueuePeek(xQueueGasPedal, &gas_pedal, ( TickType_t ) 0);
        xQueuePeek(xQueueBrakePedal, &brake_pedal, ( TickType_t ) 0);
        xQueuePeek(xQueueCruiseControl, &cruise_control, ( TickType_t ) 0);

        printf("Throttle: %d\n", throttle);
        printf("Velocity: %d\n", velocity);
        printf("Position: %d\n", position);

        sprintf(display_str, "%02d%02d", throttle, velocity/10);

        write_position(position);                /* 24 LEDs */
        BSP_SetLED(LED_GREEN, gas_pedal);
        BSP_SetLED(LED_YELLOW, cruise_control);
        BSP_SetLED(LED_RED, brake_pedal);

        BSP_7SegDispString(display_str);
        
        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}

/* -------------------- Part 3: Overload subsystem implementation ------------------- */

/* Helper busy-wait (ms) using tick counter */
static void busy_wait(uint32_t delay_ms)
{
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(delay_ms)) {
        /* spin */
    }
}

/* ExtraLoad task:
 * - period = 25 ms
 * - read SW10..SW17 (SW10 MSB), compute X (0..255)
 * - busy wait X/10 ms to impose load
 */
void vExtraLoadTask(void *arg)
{
    TickType_t xLastWake = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(EXTRA_LOAD_PERIOD_MS);

    for (;;) {
        bool b7 = BSP_GetInput(SW_10);
        bool b6 = BSP_GetInput(SW_11);
        bool b5 = BSP_GetInput(SW_12);
        bool b4 = BSP_GetInput(SW_13);
        bool b3 = BSP_GetInput(SW_14);
        bool b2 = BSP_GetInput(SW_15);
        bool b1 = BSP_GetInput(SW_16);
        bool b0 = BSP_GetInput(SW_17);

        uint8_t X = (uint8_t)((b7<<7) | (b6<<6) | (b5<<5) | (b4<<4) |
                              (b3<<3) | (b2<<2) | (b1<<1) | (b0<<0));

        uint32_t busy_ms = X / 10u; /* 0..25 ms */
        if (busy_ms > 0) busy_wait(busy_ms);

        vTaskDelayUntil(&xLastWake, xPeriod);
    }
}

/* OverloadDetection task:
 * - periodically gives watchdog semaphore (OK token)
 * - if it gets starved it will not give and watchdog will detect overload
 */
void vOverloadDetectionTask(void *arg)
{
    TickType_t xLastWake = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(OVERLOAD_DETECT_PERIOD_MS);

    for (;;) {
        if (xSemaphoreWatchDogFood != NULL) {
            xSemaphoreGive(xSemaphoreWatchDogFood);
        }
        vTaskDelayUntil(&xLastWake, xPeriod);
    }
}

/* Watchdog:
 * - waits for OK token with timeout 1000 ms
 * - if timeout => system overload: print and turn on all LEDs
 * - when OK returns, clear overload (turn off LEDs)
 */
void vWatchDogTask(void *arg)
{
    const TickType_t xTimeout = pdMS_TO_TICKS(OVERLOAD_WATCHDOG_TIMEOUT_MS);
    bool overloaded = false;

    for (;;) {
        BaseType_t got = xSemaphoreTake(xSemaphoreWatchDogFood, xTimeout);

        if (got == pdTRUE) {
            if (overloaded) {
                overloaded = false;
                /* clear all LEDs; main system will re-set them as appropriate */
                BSP_SetLED(LED_RED, false);
                BSP_SetLED(LED_GREEN, false);
                BSP_SetLED(LED_YELLOW, false);
                printf("Watchdog: system OK -> clearing overload.\n");
            }
        } else {
            /* timeout */
            if (!overloaded) {
                overloaded = true;
                printf("Watchdog: SYSTEM OVERLOAD DETECTED!\n");
                BSP_SetLED(LED_RED, true);
                BSP_SetLED(LED_GREEN, true);
                BSP_SetLED(LED_YELLOW, true);
            }
        }

        /* small sleep so this task does not loop too tightly */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Main program that starts all the tasks and the scheduler
 * 
 * ==> MODIFY THE MAIN PROGRAM!
 *        - Convert the button and control tasks to periodic tasks
 *        - Adjust the priorities of the task so that they correspond
 *          to the rate-monotonic algorithm.
 * ==> MODIFIED: Converted button and control tasks to periodic, adjusted priorities
 * @return 
 */
int main()
{
    BSP_Init();  /* Initialize all components on the ES Lab-Kit. */

    /* Create the message queues */
    xQueueCruiseControl = xQueueCreate( 1, sizeof(bool));
    xQueueGasPedal = xQueueCreate( 1, sizeof(bool));
    xQueueBrakePedal = xQueueCreate( 1, sizeof(bool));
    xQueueVelocity = xQueueCreate( 1, sizeof(uint16_t));
    xQueueTargetVelocity = xQueueCreate( 1, sizeof(uint16_t));
    xQueuePosition = xQueueCreate( 1, sizeof(uint16_t));
    xQueueThrottle = xQueueCreate( 1, sizeof(uint16_t));
    
    /* Create the tasks. */
    xTaskCreate(vButtonTask, "Button Task", 512, (void*) 50, 7, &xButton_handle);
    xTaskCreate(vVehicleTask, "Vehicle Task", 512, (void*) 100, 6, &xVehicle_handle); 
    xTaskCreate(vControlTask, "Control Task", 512, (void*) 200, 5, &xControl_handle);
    xTaskCreate(vDisplayTask, "Display Task", 512, (void*) 500, 4, &xDisplay_handle); 

    /* ----------------- Part 3 init: create watchdog semaphore and tasks ----------------- */
    xSemaphoreWatchDogFood = xSemaphoreCreateBinary();

    /* Create Watchdog (highest), ExtraLoad (high), OverloadDetection (low) */
    xTaskCreate(vWatchDogTask, "Watchdog", 512, (void*)1000, 10, &xWatchDog_handle);
    xTaskCreate(vExtraLoadTask, "ExtraLoad", 512, (void*)25, 9, &xExtraLoad_handle);
    xTaskCreate(vOverloadDetectionTask, "OverDetect", 512, NULL, 3, &xOverload_handle);

    vTaskStartScheduler();  /* Start the scheduler. */
    
    return 0;
}
/*-----------------------------------------------------------*/
