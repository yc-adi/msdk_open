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
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "mxc_device.h"
#include "led.h"
#include "board.h"
#include "mxc_delay.h"
#include "mcr_regs.h"
#include "sema.h"
#include "core1.h"
#include "tmr.h"

extern uint32_t SystemCoreClock; /*!< System Clock Frequency (Core Clock)  */
#ifndef PeripheralClock
#define PeripheralClock (SystemCoreClock / 2) /*!< Peripheral Clock Frequency */
#endif

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
    printf("Core 1: enter while loop.\n");
    while (1) {
        // Wait for Core 0 to release the semaphore
        while(MXC_SEMA_GetSema(1) == E_BUSY) {}

        LED_On(0);
        LED_Off(1);

        MXC_TMR_Delay(MXC_TMR1, MXC_DELAY_MSEC(500));

        // Print the updated value from Core 0
        printf("Core 1: Ping: %d\n", count1);

        LED_Off(0);
        LED_On(1);

        MXC_TMR_Delay(MXC_TMR1, MXC_DELAY_MSEC(500));

        // Update the count and release the semaphore
        // causing Core 0 to print the new count
        count0++;
        MXC_SEMA_FreeSema(0);
    }
}

// *****************************************************************************
int main(void)
{
    uint32_t n = 5;

    printf("\n\n\n***** MAX32665 Dual Core Example *****\n");
    printf("Similar to the 'Hello World' example but split the\n");
    printf("lights and console uart between core 0 and core 1.\n");
    printf("halting this example with a debugger will not stop core 1.\n\n");

    MXC_SEMA_Init();

    MXC_SEMA_GetSema(0);

    Core1_Start();

    MXC_SEMA_FreeSema(1);


    mxc_tmr_cfg_t tmr_cfg;
    tmr_cfg.pres = MXC_TMR_PRES_1;
    tmr_cfg.mode = MXC_TMR_MODE_CONTINUOUS;
    tmr_cfg.cmp_cnt = PeripheralClock / 2;
    tmr_cfg.pol = 0;
    MXC_TMR_Init(MXC_TMR2, &tmr_cfg);

    MXC_TMR_Start(MXC_TMR2);

    while(1) {
        // Wait for Core 1 to update count and release the semaphore
        while(MXC_SEMA_CheckSema(0) == E_BUSY) {}
        MXC_SEMA_GetSema(0);

        printf("Core 0: Pong: %d\n", count0);
        count1++;

        // do some jobs here
        n = (MXC_TMR_GetCount(MXC_TMR2) % 9) + 1;
        MXC_TMR_Delay(MXC_TMR0, MXC_DELAY_MSEC(n * 100));

        MXC_SEMA_FreeSema(1);
    }
}