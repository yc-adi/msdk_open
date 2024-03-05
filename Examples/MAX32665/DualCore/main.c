/*************************************************************************************************
 *  Copyright (c) 2022-2024 Analog Devices, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *************************************************************************************************/

/**
 * @file    main.c
 * @brief   dual-core demo
 * @details This example demonstrates how to use the semaphore to synchronize 
 *          between the two cores.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "led.h"
#include "board.h"
#include "mxc_delay.h"
#include "mcr_regs.h"
#include "sema.h"
#include "core1.h"

/***** Definitions *****/
extern uint32_t __isr_vector_core1;
extern uint32_t __StackTop_Core1;
extern uint32_t __StackLimit_Core1;
int count0 = 0;
int count1 = 0;
/***** Globals *****/

/***** Functions *****/

// Similar to Core 0, the entry point for Core 1
// is Core1Main()
// Execution begins when the CPU1 Clock is enabled
int Core1_Main(void) {
    while (1) {
        // Wait for Core 0 to release the semaphore
        while(MXC_SEMA_GetSema(0) == E_BUSY) {}

        // Hold the semaphore during these delays to stall Core 0
        LED_On(0);
        LED_Off(1);
        MXC_Delay(500000);

        // Print the updated value from Core 0
        printf("Core 1: Ping: %d\n", count1);
        LED_Off(0);
        LED_On(1);
        MXC_Delay(500000);

        // Update the count and release the semaphore
        // causing Core 0 to print the new count
        count0++;
        MXC_SEMA_FreeSema(0);
    }
}

// *****************************************************************************
int main(void)
{
    printf("***** MAX32665 Dual Core Example *****\n");
    printf("Runs the 'Hello World' exampled by splitting the\n");
    printf("lights and console uart between core 0 and core 1.\n");
    printf("halting this example with a debugger will not stop core 1.\n\n");

    MXC_SEMA_Init();
    Core1_Start();

    while(1) {
        // Wait for Core 1 to update count and release the semaphore
        while(MXC_SEMA_GetSema(0) == E_BUSY) {}
        MXC_SEMA_GetSema(0);
        printf("Core 0: Pong: %d\n", count0);
        count1++;
        MXC_SEMA_FreeSema(0);

        // Delay after releasing the semaphore to avoid a race condition
        MXC_Delay(500000);
    }
}