# ES-Lab-Kit-Examples


The repository contains introductory examples for the Embedded Systems Lab-Kit which is used at KTH for courses in embedded systems.

## Prerequisites

To use the examples you need to have access to the Embedded Systems Lab-Kit development board and need to have successfully installed the supporting software from this repository: [https://gits-15.sys.kth.se/mabecker/ES-Lab-Kit](https://gits-15.sys.kth.se/mabecker/ES-Lab-Kit).

## Installation

The installation instructions have been written for Linux and assume that you [created an SSH-key for your account](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account?platform=linux). The installation procedure should be easily adapted to Apple or Windows computers.

1. In the directory for the ES-Lab-Kit software enter the directory `Software`. The following example assumes that the ES-Lab-Kit software is installed below `~/Documents/ES-Lab-Kit`.
```
cd ~/Documents/ES-Lab-Kit/Software
```
2. Clone the repository.
```
git clone https://gits-15.sys.kth.se/ingo/ES-Lab-Kit-Examples
```
3. Now you can run the different examples (Blink, BlinkFreeRTOS, ...) by opening them as project in VSCode as discussed [here](https://gits-15.sys.kth.se/mabecker/ES-Lab-Kit).

## Available Examples 

- `Blink`: Minimal bare-metal program that toggles an LED
- `BlinkFreeRTOS`: Minimal FreeRTOS program that toggles an LED
- `GPIO_Timer_Interrupt`: Bare-metal program introducing GPIO- and repeating timer interrupts
- `MeasuringResponseTime`: Starting point for a FreeRTOS to simulate different task workloads and to measure response times
- `TwoTasksMsgQueueBlocking`: FreeRTOS program where two tasks communicate via a message queue with blocking write and blocking read
- `TwoTasksMsgQueueNonBlocking`: FreeRTOS program where two tasks communicate via a message queue with non-blocking write and blocking read
- `TwoTasksMutex`: FreeRTOS program that uses a mutex to create mutually exclusive access to a shared variable
- `TwoTasksSemaphore`: FreeRTOS program that demonstrates task synchronisation with a semaphore