/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 * (now owned by Analog Devices, Inc.),
 * Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 * is proprietary to Analog Devices, Inc. and its licensors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/**
 * @file    main.c
 * @brief   BLE_FreeRTOS
 * @details This example demonstrates FreeRTOS with BLE capabilities.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"
#include "mxc_delay.h"
#include "mxc_device.h"
#include "wut.h"
#include "lp.h"
#include "led.h"
#include "board.h"
#include "pb.h"
#include "wsf_trace.h"
#include "flc.h"

#define BUILD_FOR_OTAS

/* Shadow register definitions */
#define MXC_R_SIR_SHR13 *((uint32_t *)(0x40005434))
#define MXC_R_SIR_SHR17 *((uint32_t *)(0x40005444))

/* Stringification macros */
#define STRING(x) STRING_(x)
#define STRING_(x) #x


//volatile int wutTrimComplete;
extern uint32_t _flash_update;
extern uint32_t _eflash_update;
extern uint32_t _text;
extern uint32_t _configlow;
extern uint32_t _confighigh;


extern void bleStartup(void);

/***** Functions *****/

/* =| vAssertCalled |==============================
 *
 *  Called when an assertion is detected. Use debugger to backtrace and 
 *  continue.
 *
 * =======================================================
 */
void vAssertCalled(const char *const pcFileName, uint32_t ulLine)
{
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    /* Parameters are not used. */
    (void)ulLine;
    (void)pcFileName;

    __asm volatile("cpsid i");
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
        value. */
        while (ulSetToNonZeroInDebuggerToContinue == 0) {}
    }
    __asm volatile("cpsie i");
}

/* =| vApplicationIdleHook |==============================
 *
 *  Call the user defined function from within the idle task.  This
 *  allows the application designer to add background functionality
 *  without the overhead of a separate task.
 *  NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
 *  CALL A FUNCTION THAT MIGHT BLOCK.
 *
 * =======================================================
 */
void vApplicationIdleHook(void)
{
    /* Sleep while idle */
    LED_Off(SLEEP_LED);

    MXC_LP_EnterSleepMode();

    LED_On(SLEEP_LED);
}

/* =| turnOffUnused |==========================
 *
 * Disable unused hardware to conserve power.
 *
 * =======================================================
 */
void turnOffUnused(void)
{
    /* Prevent SIMO leakage in DS by reducing the SIMO buck clock */
    if (MXC_GCR->revision == 0xA2) {
        MXC_R_SIR_SHR13 = 0x0;
        MXC_R_SIR_SHR17 &= ~(0xC0);
    } else if (MXC_GCR->revision == 0xA4) {
        MXC_R_SIR_SHR17 &= ~(0xC0);
    }

    MXC_LP_USBSWLPDisable();
}

#define HRM_REFRESH 121

void vTaskDataUpdater(void *pvParameters)
{
   TickType_t ticks = 0;
   TickType_t xLastWakeTime;

   /* Get task start time */
   xLastWakeTime = xTaskGetTickCount();

   while (1) {
       ticks = xTaskGetTickCount();
       printf("Uptime is 0x%08x (%u seconds) \n", ticks,
              ticks / configTICK_RATE_HZ);

       vTaskDelayUntil(&xLastWakeTime, HRM_REFRESH);
   }
}


#ifdef BUILD_FOR_OTAS

/**
 * @brief This function restores the boot configuration and returns a 32-bit unsigned integer value.
 *
 * The function restores the boot configuration, which involves reading data from non-volatile storage
 * such as EEPROM or flash memory, and configuring various hardware settings based on that data.
 *
 * @return 0 if the boot configuration was successfully restored, otherwise an error code.
 */
uint32_t restoreBootConfig(void)
{
    uint32_t err;

    portDISABLE_INTERRUPTS();

    /* if bootAddress are ok write it to the config file*/
    err = MXC_FLC_PageErase(((uint32_t)&_configlow));
    if (err != E_NO_ERROR)
    {
        printf("Flash page erase error at 0x%0x", ((uint32_t)&_configlow));
    }
    else
    {
        printf(">>> Initiating erase of %x pages of config flash <<<", ((uint32_t)&_configlow));
    }

    uint32_t bootAddress = ((uint32_t)&_flash_update);
    err += MXC_FLC_Write(((uint32_t)&_configlow), sizeof(bootAddress),
                         (uint32_t *)&bootAddress);
    uint32_t *temp2 = (uint32_t *)((uint32_t)&_configlow);
    /* verify data was written*/
    err += memcmp(temp2, &bootAddress, sizeof(bootAddress));
    if (err)
    {
    	printf("Error writing bootAddress to flash");
    }
    else
    {
    	printf("Restoring BootAddress of flash to OTAS %x or %x ", *temp2, bootAddress);
    }

    portENABLE_INTERRUPTS();

    return err;
}

#endif
/* =| main |==============================================
 *
 * This program demonstrates FreeRTOS tasks, mutexes.
 *
 * =======================================================
 */
int main(void)
{

#ifdef BUILD_FOR_OTAS
    MXC_Delay(100000);

    APP_TRACE_INFO0("********************************************************************************************\r");

    APP_TRACE_INFO0("  SSSSS  EEEEEEE NN   NN  SSSSS  IIIII  OOOOO      OOOOO  RRRRRR  BBBBB   YY   YY TTTTTTT R \r");
    APP_TRACE_INFO0(" SS      EE      NNN  NN SS       III  OO   OO    OO   OO RR   RR BB   B  YY   YY   TTT     \r");
    APP_TRACE_INFO0("  SSSSS  EEEEE   NN N NN  SSSSS   III  OO   OO    OO   OO RRRRRR  BBBBBB   YYYYY    TTT     \r");
    APP_TRACE_INFO0("      SS EE      NN  NNN      SS  III  OO   OO    OO   OO RR  RR  BB   BB   YYY     TTT     \r");
    APP_TRACE_INFO0("  SSSSS  EEEEEEE NN   NN  SSSSS  IIIII  OOOO0      OOOO0  RR   RR BBBBBB    YYY     TTT     \r\n");

    APP_TRACE_INFO1("**Image Booting from Internal Flash address 0x%x\r\n",((uint32_t) &_text));
    APP_TRACE_INFO1("**Image Booting from Internal Flash address 0x%x\r\n",((uint32_t) &_text));
    APP_TRACE_INFO1("**Image Booting from Internal Flash address 0x%x\r\n",((uint32_t) &_text));
    APP_TRACE_INFO0("*****************************************************************************************\r\n");


    MXC_Delay(100000);

    restoreBootConfig();
#endif
    /* Print banner (RTOS scheduler not running) */
    printf("\n-=- %s BLE FreeRTOS (%s) Demo -=-\n", STRING(TARGET), tskKERNEL_VERSION_NUMBER);
#if configUSE_TICKLESS_IDLE
    printf("Tickless idle is enabled\n");
    /* Initialize CPU Active LED */
    LED_On(SLEEP_LED);
    LED_On(DEEPSLEEP_LED);
#endif
    printf("SystemCoreClock = %d\n", SystemCoreClock);

    /* Delay to prevent bricks */
    volatile int i;
    for (i = 0; i < 0x3FFFFF; i++) {}

    /* Turn off unused hardware to conserve power */
    turnOffUnused();

    /* Start the BLE application */
    bleStartup();

   ////if ((xTaskCreate(vTaskDataUpdater, (const char *)"vTaskDataUpdater", 2 * configMINIMAL_STACK_SIZE,
   //                   NULL, tskIDLE_PRIORITY + 2, NULL) != pdPASS)) {
   //      printf("xTaskCreate() failed to create a task.\n");
   //  } else {
   //      printf("Starting scheduler.\n");
   //      /* Start scheduler */
   //      vTaskStartScheduler();
   //  }

    /* Start scheduler */
    vTaskStartScheduler();

    /* This code is only reached if the scheduler failed to start */
    printf("ERROR: FreeRTOS did not start due to above error!\n");
    while (1) {
        __NOP();
    }

    /* Quiet GCC warnings */
    return -1;
}

typedef struct __attribute__((packed)) ContextStateFrame {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t return_address;
    uint32_t xpsr;
} sContextStateFrame;

/*****************************************************************/
void HardFault_Handler(void)
{
    __asm(" TST LR, #4\n"
          " ITE EQ \n"
          " MRSEQ R0, MSP \n"
          " MRSNE R0, PSP \n"
          " B HardFault_Decoder \n");
}

/*****************************************************************/
/* Disable optimizations for this function so "frame" argument */
/* does not get optimized away */
__attribute__((optimize("O0"))) void HardFault_Decoder(sContextStateFrame *frame)
{
    /* Hang here */
    while (1) {}
}
