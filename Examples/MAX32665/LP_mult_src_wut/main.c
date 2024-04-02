/******************************************************************************
 *
 * Copyright 2024 Analog Devices, Inc.
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

/*
 * @file    main.c
 * @brief   Demonstrate multiple wake-up sources for DEEPSLEEP mode.
 *
 * @details using either the WUT timer or a GPIO to wake up from DEEPSLEEP
 *          mode.
 */

#include <stdio.h>
#include <stdint.h>

#include "mxc_device.h"
#include "mxc_sys.h"
#include "mxc_delay.h"
#include "mxc_errors.h"
#include "pb.h"
#include "board.h"
#include "wut.h"
#include "lp.h"
#include "uart.h"

#define DELAY_IN_SEC 15
#define MILLISECONDS_WUT (DELAY_IN_SEC * 1000)

#define SLEEP_TIME_IN_SEC   10

volatile int triggered;

void buttonHandler(void *pb)
{
    triggered = 1;
}

void WUT_IRQHandler(void)
{
    triggered = 2;

    MXC_WUT_IntClear();
}

void waitTrigger(int waitForTrigger)
{
    int tmp;

    triggered = 0;

    NVIC_EnableIRQ(WUT_IRQn);
    MXC_WUT_Enable();

    if (waitForTrigger) {
        while (!triggered) {}
    }

    // Debounce the button press.
    if (triggered == 1) {
        for (tmp = 0; tmp < 0x800000; tmp++) {
            __NOP();
        }
    }

    // Wait for serial transactions to complete.
    while (MXC_UART_ReadyForSleep(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {}
}

int main(void)
{
    mxc_wut_cfg_t cfg;
    uint32_t ticks;

    printf("\n\n****Low Power DEEPSLEEP Mode Example****\n\n");
    MXC_Delay(5000000);     // during development leave enought time to use emulator to erash flash

    PB_RegisterCallback(0, (pb_callback)buttonHandler);

    // init and config WUT
    MXC_WUT_GetTicks(MILLISECONDS_WUT, MXC_WUT_UNIT_MILLISEC, &ticks);  // Get ticks based off of milliseconds

    // config structure for one shot timer to trigger in a number of ticks
    cfg.mode = MXC_WUT_MODE_ONESHOT;
    cfg.cmp_cnt = ticks;

    // Init WUT
    MXC_WUT_Init(MXC_WUT_PRES_1);

    //Config WUT
    MXC_WUT_Config(&cfg);

    printf("Running in ACTIVE mode. Start the test after %d secs or button press\n\n", DELAY_IN_SEC);
    waitTrigger(1); // start the demo after 30 secs or button press

    // enable the wake-up sources
    MXC_LP_EnableGPIOWakeup((mxc_gpio_cfg_t *)&pb_pin[0]);
    MXC_LP_EnableWUTAlarmWakeup();

    while (1) {
        printf("Entering DEEPSLEEP mode, wake up by timer (%d secs) or button press.\n", DELAY_IN_SEC);
        waitTrigger(0);

        MXC_LP_EnterDeepSleepMode();

        printf("Waked up from DEEPSLEEP mode by %s. Then delay %d secs to enter DEEPSLEEP again.\n\n",
               triggered == 1? "button" : "timer", SLEEP_TIME_IN_SEC);
        MXC_Delay(SLEEP_TIME_IN_SEC * 1000000);
    }
}
