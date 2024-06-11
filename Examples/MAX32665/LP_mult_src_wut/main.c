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

#include "gpio.h"
#include "icc.h"
#include "lp.h"
#include "mxc_device.h"
#include "mxc_sys.h"
#include "mxc_delay.h"
#include "mxc_errors.h"
#include "pb.h"
#include "board.h"
#include "wut.h"
#include "lp.h"
#include "simo.h"
#include "uart.h"

#include "sharp_mip.h"

#define CUSTOM_DS           1

#define DS_IN_SEC           30
#define MILLISECONDS_WUT    (DS_IN_SEC * 1000)

#define DELAY_TIME_IN_SEC   30

extern sharp_mip_dev ls013b7dh03_controller;

volatile int triggered;

/*
 *  Switch the system clock to the HIRC / 4
 *
 *  Enable the HIRC, set the divide ration to /4, and disable the HIRC96 oscillator.
 */
void switchToHIRCD4(void)
{
    MXC_SETFIELD(MXC_GCR->clkcn, MXC_F_GCR_CLKCN_PSC, MXC_S_GCR_CLKCN_PSC_DIV4);
    MXC_GCR->clkcn |= MXC_F_GCR_CLKCN_HIRC_EN;
    MXC_SETFIELD(MXC_GCR->clkcn, MXC_F_GCR_CLKCN_CLKSEL, MXC_S_GCR_CLKCN_CLKSEL_HIRC);
    /* Disable unused clocks */
    while (!(MXC_GCR->clkcn & MXC_F_GCR_CLKCN_CKRDY)) {}
    /* Wait for the switch to occur */
    MXC_GCR->clkcn &= ~(MXC_F_GCR_CLKCN_HIRC96M_EN);
    SystemCoreClockUpdate();
}

/*
 *  Switch the system clock to the HIRC96
 *
 *  Enable the HIRC96, set the divide ration to /1, and disable the HIRC oscillator.
 */
void switchToHIRC(void)
{
    MXC_SETFIELD(MXC_GCR->clkcn, MXC_F_GCR_CLKCN_PSC, MXC_S_GCR_CLKCN_PSC_DIV1);
    MXC_GCR->clkcn |= MXC_F_GCR_CLKCN_HIRC96M_EN;
    MXC_SETFIELD(MXC_GCR->clkcn, MXC_F_GCR_CLKCN_CLKSEL, MXC_S_GCR_CLKCN_CLKSEL_HIRC96);
    /* Disable unused clocks */
    while (!(MXC_GCR->clkcn & MXC_F_GCR_CLKCN_CKRDY)) {}
    /* Wait for the switch to occur */
    MXC_GCR->clkcn &= ~(MXC_F_GCR_CLKCN_HIRC_EN);
    SystemCoreClockUpdate();
}

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

    //sharp_mip_init(&ls013b7dh03_controller);
    //sharp_mip_onoff(&ls013b7dh03_controller, 0);

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

    printf("Running in ACTIVE mode. Start the test after %d secs or button press\n\n", DS_IN_SEC);
    waitTrigger(1); // start the demo after 30 secs or button press

    // enable the wake-up sources
    MXC_LP_EnableGPIOWakeup((mxc_gpio_cfg_t *)&pb_pin[0]);
    MXC_LP_EnableWUTAlarmWakeup();

    MXC_LP_ROM0Shutdown();
    MXC_LP_USBFIFOShutdown();
    MXC_LP_CryptoShutdown();
    MXC_LP_SRCCShutdown();
    MXC_LP_ICacheXIPShutdown();
    MXC_LP_ICache1Shutdown();
    MXC_LP_SysRam5Shutdown();
    MXC_LP_SysRam4Shutdown();
    MXC_LP_SysRam3Shutdown();
    MXC_LP_SysRam2Shutdown();
    MXC_LP_SysRam1PowerUp();
    MXC_LP_SysRam0PowerUp(); // Global variables are in RAM0 and RAM1

    while (1) {
        printf("Entering DEEPSLEEP mode, wake up by timer (%d secs) or button press.\n", DS_IN_SEC);
        waitTrigger(0);

#if CUSTOM_DS == 1
        MXC_ICC_Disable();
        MXC_LP_ICache0Shutdown();

        /* Shutdown unused power domains */
        MXC_PWRSEQ->lpcn |= MXC_F_PWRSEQ_LPCN_BGOFF;

        /* Prevent SIMO soft start on wakeup */
        MXC_LP_FastWakeupDisable();

        /* Enable VDDCSWEN=1 prior to enter backup/deepsleep mode */
        MXC_MCR->ctrl |= MXC_F_MCR_CTRL_VDDCSWEN;

        /* Switch the system clock to a lower frequency to conserve power in deep sleep
        and reduce current inrush on wakeup */
        switchToHIRCD4();

        /* Reduce VCOREB to 0.81v */
        MXC_SIMO_SetVregO_B(810);
#endif

        MXC_LP_EnterDeepSleepMode();

#if CUSTOM_DS == 1
        /*  If VCOREA not ready and VCOREB ready, switch VCORE=VCOREB 
        (set VDDCSW=2’b01). Configure VCOREB=1.1V wait for VCOREB ready. */

        /* Check to see if VCOREA is ready on  */
        if (!(MXC_SIMO->buck_out_ready & MXC_F_SIMO_BUCK_OUT_READY_BUCKOUTRDYC)) {
            /* Wait for VCOREB to be ready */
            while (!(MXC_SIMO->buck_out_ready & MXC_F_SIMO_BUCK_OUT_READY_BUCKOUTRDYB)) {}

            /* Move VCORE switch back to VCOREB */
            MXC_MCR->ctrl = (MXC_MCR->ctrl & ~(MXC_F_MCR_CTRL_VDDCSW)) |
                            (0x1 << MXC_F_MCR_CTRL_VDDCSW_POS);

            /* Raise the VCORE_B voltage */
            while (!(MXC_SIMO->buck_out_ready & MXC_F_SIMO_BUCK_OUT_READY_BUCKOUTRDYB)) {}
            MXC_SIMO_SetVregO_B(1000);
            while (!(MXC_SIMO->buck_out_ready & MXC_F_SIMO_BUCK_OUT_READY_BUCKOUTRDYB)) {}
        }

        MXC_LP_ICache0PowerUp();
        MXC_ICC_Enable();

        /* Restore the system clock */
        switchToHIRC();
#endif

        printf("Waked up from DEEPSLEEP mode by %s. Then delay %d secs to enter DEEPSLEEP again.\n\n",
               triggered == 1? "button" : "timer", DELAY_TIME_IN_SEC);
        MXC_Delay(DELAY_TIME_IN_SEC * 1000000);
    }
}
