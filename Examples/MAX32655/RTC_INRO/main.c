/******************************************************************************
 * Copyright (C) 2022 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/

/**
 * @file    main.c
 * @brief   RTC with the INRO instead of the 32 kHz crystal
 * @details Testing the utilization of the INRO to replace the 32 kHz crystal. 
            The idea is that customers could use this to save on BOM area and cost.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "led.h"
#include "pb.h"
#include "board.h"
#include "mxc_delay.h"
#include "rtc.h"
#include "mcr_regs.h"
#include "trimsir_regs.h"
#include "tmr.h"
#include "simo.h"
#include "wut.h"

/***** Definitions *****/

#define MSEC_TO_RSSA(x) \
    (0 - ((x * 4096) /  1000)) /* Converts a time in milleseconds to the equivalent RSSA register value. */

#define SECS_PER_MIN 60
#define SECS_PER_HR (60 * SECS_PER_MIN)
#define SECS_PER_DAY (24 * SECS_PER_HR)

/***** Globals *****/

/***** Functions *****/
volatile int buttonPressed = 0;
void buttonHandler()
{
    buttonPressed = 1;
}

void printTime()
{
    int day, hr, min, err;
    uint32_t sec, rtc_readout;
    double subsec;

    do {
        err = MXC_RTC_GetSubSeconds(&rtc_readout);
    } while (err != E_NO_ERROR);
    subsec = rtc_readout / 4096.0;

    do {
        err = MXC_RTC_GetSeconds(&rtc_readout);
    } while (err != E_NO_ERROR);
    sec = rtc_readout;

    day = sec / SECS_PER_DAY;
    sec -= day * SECS_PER_DAY;

    hr = sec / SECS_PER_HR;
    sec -= hr * SECS_PER_HR;

    min = sec / SECS_PER_MIN;
    sec -= min * SECS_PER_MIN;

    subsec += sec;

    printf("Current Time (DAY d hh:mm:ss): DAY %d %02d:%02d:%06.3f\n", day, hr, min, subsec);
}

void RTC_IRQHandler(void)
{
    int flags = MXC_RTC_GetFlags();

    /* Check sub-second alarm flag. */
    if (flags & MXC_F_RTC_CTRL_SSEC_ALARM) {
        LED_Toggle(1);
        MXC_RTC_ClearFlags(MXC_F_RTC_CTRL_SSEC_ALARM);
    }
}

uint32_t USEC_to_RSSA(uint64_t inroFreq, uint64_t usec)
{
    uint64_t retval;
    retval = ((uint64_t)0 - (((uint64_t)usec * (uint64_t)inroFreq/(uint64_t)4) / (uint64_t)1000000 ));

    return retval;
}

uint32_t MSEC_to_RSSA(uint32_t inroFreq, uint32_t msec)
{
    return (0 - ((msec * inroFreq/4) / 1000 ));
}

uint32_t measureINRO(void)
{
    uint32_t elapsedERFO, elapsedINRO, elapsedWUT;

    /* Init timer to count 32 MHz periods */
    mxc_tmr_cfg_t tmr_cfg;
    tmr_cfg.pres = TMR_PRES_1;
    tmr_cfg.mode = TMR_MODE_CONTINUOUS;
    tmr_cfg.bitMode = TMR_BIT_MODE_32;
    tmr_cfg.clock = MXC_TMR_APB_CLK; // System clock is 32 Mhz, APB is 1/2
    tmr_cfg.cmp_cnt = 0xFFFFFFFF;
    tmr_cfg.pol = 0;
    MXC_TMR_Init(MXC_TMR3, &tmr_cfg, FALSE);

    /* Use LPTMR0 with INRO as the source */
    mxc_tmr_cfg_t lptmrCfg;
    lptmrCfg.pres = TMR_PRES_1;
    lptmrCfg.mode = TMR_MODE_CONTINUOUS;
    lptmrCfg.bitMode = TMR_BIT_MODE_32;
    lptmrCfg.clock = MXC_TMR_8K_CLK;
    lptmrCfg.cmp_cnt = 0xFFFFFFFF;
    lptmrCfg.pol = 0;
    MXC_TMR_Init(MXC_TMR4, &lptmrCfg, FALSE /*init_pins*/);

    MXC_TMR_Stop(MXC_TMR3);
    MXC_TMR_SetCount(MXC_TMR3, 0);

    /* Start the timers */
    MXC_TMR_Start(MXC_TMR3);
    MXC_TMR_Start(MXC_TMR4);
    elapsedWUT = MXC_WUT_GetCount(MXC_WUT);

    MXC_Delay(200000);

    /* Capture the TMR count and adjust for processing delay */
    elapsedERFO = MXC_TMR_GetCount(MXC_TMR3);
    elapsedINRO = MXC_TMR_GetCount(MXC_TMR4);
    elapsedWUT = MXC_WUT_GetCount(MXC_WUT) - elapsedWUT;

    /* Stop the timers */
    MXC_TMR_Stop(MXC_TMR3);
    MXC_TMR_Shutdown(MXC_TMR3);

    MXC_TMR_Stop(MXC_TMR4);
    MXC_TMR_Shutdown(MXC_TMR4);

    printf("elapsedERFO = %d\n", elapsedERFO);
    printf("elapsedINRO = %d\n", elapsedINRO);

    /* TODO: Can WUT run from INRO? */
    printf("elapsedWUT  = %d\n", elapsedWUT);
    
    /* TODO Calculate the frequency */
    // System clock is 32 Mhz, APB is 1/2
    uint64_t freq = ((uint64_t)elapsedINRO * (uint64_t)16001450) / (uint64_t)elapsedERFO;
    uint32_t freq32 = (uint32_t)freq;

    return freq32;
}


// *****************************************************************************
int main(void)
{
    int error;

    /* Delay to prevent bricks */
    volatile int i;
    for(i = 0; i < 0xFFFFFF; i++) {}

    /* Setup callback to receive notification of when button is pressed. */
    PB_RegisterCallback(0, (pb_callback)buttonHandler);

    /* Set the system clock to the 32 MHz clock for the INRO measurement */
    /* Enable 32 MHz clock if not already enabled */
    if (!(MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_ERFO_RDY)) {
        /* Power VREGO_D */
        MXC_SIMO->vrego_d = (0x3c << MXC_F_SIMO_VREGO_D_VSETD_POS);
        while (!(MXC_SIMO->buck_out_ready & MXC_F_SIMO_BUCK_OUT_READY_BUCKOUTRDYD)) {}

        /* Restore btleldoctrl setting */
        MXC_GCR->btleldoctrl = 0x3055;
        while (!(MXC_SIMO->buck_out_ready & MXC_F_SIMO_BUCK_OUT_READY_BUCKOUTRDYD)) {}

        /* Enable 32Mhz oscillator */
        MXC_GCR->clkctrl |= MXC_F_GCR_CLKCTRL_ERFO_EN;
        while (!(MXC_GCR->clkctrl & MXC_F_GCR_CLKCTRL_ERFO_RDY)) {}
    }

    /* Switch the system clock to the 32 MHz oscillator */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_ERFO);
    MXC_SYS_SetClockDiv(MXC_SYS_CLOCK_DIV_1);
    SystemCoreClockUpdate();
    Console_Init();

    printf("RTC_INRO\n");
    printf("SystemCoreClock = %d\n", SystemCoreClock);

    error = MXC_RTC_Init(0/* sec */, 0 /* ssec */);
    if (error != E_NO_ERROR) {
        printf("Failed RTC Initialization: %d\n", error);
        while (1) {}
    }

    /* Disable the ERTCO */
    MXC_GCR->clkctrl &= ~(MXC_F_GCR_CLKCTRL_ERTCO_EN);
    MXC_GCR->clkctrl |= (MXC_F_GCR_CLKCTRL_ERTCO_EN);

    MXC_MCR->ctrl |= (0x1 << 2);

    #if 0
    error = MXC_RTC_SetClock(MXC_RTC_CLK_ALT);
    if (error != E_NO_ERROR) {
        printf("Failed RTC Set Clock: %d\n", error);
        while (1) {}
    }
    #endif

    printf("RTC running from INRO\n");

    printf("MXC_TRIMSIR->inro = 0x%08X\n", MXC_TRIMSIR->inro);

    /* Init WUT */
    mxc_wut_cfg_t cfg;
    cfg.mode = MXC_WUT_MODE_COMPARE;
    cfg.cmp_cnt = 0xFFFFFFFF;

    MXC_WUT_Init(MXC_WUT, MXC_WUT_PRES_1);
    MXC_WUT_Config(MXC_WUT, &cfg); 
    MXC_WUT_SetCount(MXC_WUT, 0);
    MXC_WUT_Enable(MXC_WUT);

    uint32_t inroFreq = measureINRO();
#if 0
    while(1) {
        inroFreq = measureINRO();
        printf("INRO = %d Hz\n", inroFreq);
        MXC_Delay(1000000);
    }
#endif
    // inroFreq = 32768;

    /* Use LPTMR0 to output the INRO */
    mxc_tmr_cfg_t lptmrCfg;
    lptmrCfg.pres = TMR_PRES_1;
    lptmrCfg.mode = TMR_MODE_CONTINUOUS;
    lptmrCfg.bitMode = TMR_BIT_MODE_32;
    lptmrCfg.clock = MXC_TMR_8K_CLK;
    lptmrCfg.cmp_cnt = 1;
    lptmrCfg.pol = 0;
    MXC_TMR_Init(MXC_TMR4, &lptmrCfg, TRUE /*init_pins*/);
    MXC_TMR_Start(MXC_TMR4);

    if (MXC_RTC_Start() != E_NO_ERROR) {
        printf("Failed RTC_Start\n");
        printf("Example Failed\n");

        while (1) {}
    }

    MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE | MXC_F_RTC_CTRL_SSEC_ALARM_IE |
                       MXC_F_RTC_CTRL_RDY_IE);
    MXC_RTC_ClearFlags(MXC_RTC_GetFlags());

    NVIC_EnableIRQ(RTC_IRQn);
    __enable_irq();

    /* RTC Sub second alarm will run at freqINRO / 8 */

    if (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {
        return E_BUSY;
    }

    // error = MXC_RTC_SetSubsecondAlarm(USEC_to_RSSA(inroFreq, 10000));
    error = MXC_RTC_SetSubsecondAlarm(MSEC_to_RSSA(inroFreq, 1));
    if (error != E_NO_ERROR) {
        printf("Failed to set sub second alarm: %d\n", error);
        while (1) {}
    }

    if (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_SSEC_ALARM_IE) == E_BUSY) {
        return E_BUSY;
    }

    if (MXC_RTC_Start() != E_NO_ERROR) {
        printf("Failed RTC_Start\n");
        printf("Example Failed\n");

        while (1) {}
    }
    printTime();
    
    while (1) {
        if (buttonPressed) {
            /* Show the time elapsed. */
            printTime();
            /* Delay for switch debouncing. */
            MXC_Delay(MXC_DELAY_MSEC(100));
            /* Re-arm switch detection. */
            buttonPressed = 0;
        }
    }
}
