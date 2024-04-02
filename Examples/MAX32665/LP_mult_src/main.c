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
 * @details using either the RTC alarm or a GPIO to wake up from DEEPSLEEP
 *          mode.
 */

#include <stdio.h>
#include <stdint.h>

#include "mxc_delay.h"
#include "mxc_device.h"
#include "mxc_errors.h"
#include "nvic_table.h"
#include "pb.h"
#include "led.h"
#include "lp.h"
#include "icc.h"
#include "rtc.h"
#include "uart.h"
#include "simo.h"

#define DELAY_IN_SEC 30

volatile int triggered;

void buttonHandler(void *pb)
{
    triggered = 1;
}

void alarmHandler(void)
{
    int flags = MXC_RTC->ctrl;

    triggered = 2;

    if ((flags & MXC_F_RTC_CTRL_ALSF) >> MXC_F_RTC_CTRL_ALSF_POS) {
        MXC_RTC->ctrl &= ~(MXC_F_RTC_CTRL_ALSF);
    }

    if ((flags & MXC_F_RTC_CTRL_ALDF) >> MXC_F_RTC_CTRL_ALDF_POS) {
        MXC_RTC->ctrl &= ~(MXC_F_RTC_CTRL_ALDF);
    }
}

void waitTrigger(int waitForTrigger)
{
    int tmp;

    triggered = 0;

    while (MXC_RTC_Init(0, 0) == E_BUSY) {}

    while (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_ADE) == E_BUSY) {}

    while (MXC_RTC_SetTimeofdayAlarm(DELAY_IN_SEC) == E_BUSY) {}

    while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_ADE) == E_BUSY) {}

    while (MXC_RTC_Start() == E_BUSY) {}

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
    PRINT("\n\n****Low Power DEEPSLEEP Mode Example****\n\n");

    PB_RegisterCallback(0, (pb_callback)buttonHandler);
    MXC_NVIC_SetVector(RTC_IRQn, alarmHandler);

    MXC_Delay(5000000);     // during development leave enought time to use emulator to erash flash

    PRINT("Running in ACTIVE mode. Start the test after %d secs or button press\n", DELAY_IN_SEC);

    waitTrigger(1); // start the demo after 30 secs or button press

    // enable the wake-up sources
    MXC_LP_EnableGPIOWakeup((mxc_gpio_cfg_t *)&pb_pin[0]);
    MXC_LP_EnableRTCAlarmWakeup();

    while (1) {
        PRINT("Entering DEEPSLEEP mode, wake up by timer (%d secs) or button press.\n", DELAY_IN_SEC);
        waitTrigger(0);

        MXC_LP_EnterDeepSleepMode();

        PRINT("Waked up from DEEPSLEEP mode by %s. Then delay 5 secs.\n", triggered == 1? "button" : "timer");
        MXC_Delay(5000000);
    }
}
