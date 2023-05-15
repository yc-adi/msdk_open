/*************************************************************************************************/
/*!
*  \file   main.c
*
*  \brief  Main file for cgm application.
*
*  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
*
*  Copyright (c) 2019 Packetcraft, Inc.
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
*/
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_bufio.h"
#include "wsf_msg.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_cs.h"
#include "wsf_timer.h"
#include "wsf_os.h"

#include "sec_api.h"
#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "hci_core.h"
#include "app_terminal.h"
#include "wut.h"
#include "rtc.h"
#include "trimsir_regs.h"

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
#include "ll_init_api.h"
#endif

#include "pal_bb.h"
#include "pal_cfg.h"

#include "cgm_api.h"
#include "app_ui.h"

#include "board.h"
#include "max32655.h"
#include "wakeup.h"
#include "pal_timer.h"
#include "led.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief UART TX buffer size */
#define PLATFORM_UART_TERMINAL_BUFFER_SIZE 2048U
#define DEFAULT_TX_POWER 0 /* dBm */

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief  Pool runtime configuration. */
static wsfBufPoolDesc_t mainPoolDesc[] = { { 16, 8 }, { 32, 4 }, { 192, 8 }, { 256, 8 } };

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
static LlRtCfg_t mainLlRtCfg;
#endif

volatile int wutTrimComplete;

#ifdef DEEP_SLEEP
/**
 * @brief global variable to keep the minimal time for the next job (task). 
 * After each job, task, action, or response, need to update this variable.
 * If this time is long enough, the CPU will enter deep sleep.
 */
uint32_t gNextJobTimeInUs = 0;

#define MAX_WUT_TICKS (32768) /* Maximum deep sleep time, units of 32 kHz ticks */
#define MIN_WUT_TICKS 100 /* Minimum deep sleep time, units of 32 kHz ticks */
#define WAKEUP_US 700 /* Deep sleep recovery time, units of us */

#define WAKE_UP_TIME_IN_US              (750)  /// the time after wake up to backto normal operation
#define MIN_DEEP_SLEEP_TIME_IN_US       (WAKE_UP_TIME_IN_US)

#define WAKEUP_IN_WUT_TICK      ((uint64_t)WAKEUP_US / (uint64_t)1000000 * (uint64_t)32768)
#define RESTORE_OP_IN_WUT_TICK  65
#define RESTORE_OP_IN_US        ((uint64_t)RESTORE_OP_IN_WUT_TICK / (uint64_t)32768 * (uint64_t)1000000)

#define WAKEUP_32M_US       (3200)
#define WUT_FREQ            (32768)
#define US_TO_WUTTICKS(x)   (((x) * WUT_FREQ) / 1000000)
#define WUT_MIN_TICKS       (10)

#define SLEEP_LED           (1)
#define DEEPSLEEP_LED       (0)

/**
 * @brief How many WUT ticks to delay entering Deep Sleep after being given the go ahead by the SDMA. 
 */
#define DEEPSLEEP_DELAY (20)

typedef void SDMASleepState_t;

static volatile bool_t bHaveWUTEvent;


#define DBG_BUF_SIZE    (512)
uint32_t debugBuf[512];
uint32_t debugBufHead = 0;
uint32_t debugBufTail = 0;
#endif  // DEEP_SLEEP
/**************************************************************************************************
  Functions
**************************************************************************************************/
#ifdef DEEP_SLEEP
extern wsfTimerTicks_t wsfTimerNextExpiration(void);
extern uint32_t wsfTimerTicksToRtc(wsfTimerTicks_t wsfTicks);
extern void MXC_LP_EnableWUTAlarmWakeup(void);
#endif  

#ifdef DEEP_SLEEP
/*************************************************************************************************/
/* Get the time delay expected on powerup */
uint32_t get_powerup_delay(uint32_t wait_ticks)
{
    uint32_t ret;

    ret = US_TO_WUTTICKS(WAKEUP_32M_US + (wait_ticks / 32));

    return ret;
}
#endif  // DEEP_SLEEP

/*************************************************************************************************/
#ifdef DEEP_SLEEP
void DeepSleep(void)
{
    uint32_t preCaptureInWutCnt, postCaptureInWutCnt, schUsec;
    uint32_t dsInWutCnt, dsWutTicks;
    uint64_t bleSleepTicks, idleInWutCnt, schUsecElapsed; //dsSysTickPeriods, ;
    bool_t schTimerActive;

    uint32_t wsfTicksToNextExpiration = wsfTimerNextExpiration();
    if (wsfTicksToNextExpiration > 0)
    {
        idleInWutCnt = wsfTimerTicksToRtc(wsfTicksToNextExpiration);
    }
    else
    {
        idleInWutCnt = 0;
    }

    // remove me !!!
    debugBuf[debugBufHead++] = wsfTicksToNextExpiration;
    if (debugBufHead >= DBG_BUF_SIZE)
    {
        debugBufHead = 0;
    }

    if (idleInWutCnt > MAX_WUT_TICKS) {
        idleInWutCnt = MAX_WUT_TICKS;
    }

    /* Check to see if we meet the minimum requirements for deep sleep */
    if (idleInWutCnt < (MIN_WUT_TICKS + WAKEUP_US)) {
        return;
    }

    WsfTaskLock();
    /** Enter a critical section but don't use the taskENTER_CRITICAL()
     *  method as that will mask interrupts that should exit sleep mode. 
     */
    __asm volatile("cpsid i");

    if (!wsfOsReadyToSleep())
        //remove me !!! TODO: && UART_PrepForSleep(MXC_UART_GET_UART(CONSOLE_UART)) == E_NO_ERROR) {
    {
        goto EXIT_SLEEP_FUNC;
    }

    /* Determine if the Bluetooth scheduler is running */
    if (PalTimerGetState() == PAL_TIMER_STATE_BUSY)
    {
        schTimerActive = TRUE;
    }
    else
    {
        schTimerActive = FALSE;
    }

    if (!schTimerActive) {
        uint32_t ts;
        if (PalBbGetTimestamp(&ts)) {
            /*Determine if PalBb is active, return if we get a valid time stamp indicating 
             * that the scheduler is waiting for a PalBb event */

            goto EXIT_SLEEP_FUNC;
        }
    }

    /* Disable SysTick */
    SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);

    /* Enable wakeup from WUT */
    NVIC_EnableIRQ(WUT_IRQn);
    MXC_LP_EnableWUTAlarmWakeup();

    /* Determine if we need to snapshot the PalBb clock */
    if (schTimerActive) 
    {
        /* Snapshot the current WUT value with the PalBb clock */
        MXC_WUT_Store(MXC_WUT);
        preCaptureInWutCnt = MXC_WUT_GetCount(MXC_WUT);
        schUsec = PalTimerGetExpTime();  // old way: bool_t SchBleGetNextDueTime(uint32_t *pDueTime)

        /* Adjust idleTicks for the time it takes to restart the BLE hardware */
        idleInWutCnt-= (WAKEUP_IN_WUT_TICK + RESTORE_OP_IN_WUT_TICK);

        /* Calculate the time to the next BLE scheduler event */
        if (schUsec < WAKEUP_US)
        {
            bleSleepTicks = 0;
        }
        else
        {
            bleSleepTicks = ((uint64_t)schUsec - (uint64_t)WAKEUP_US - RESTORE_OP_IN_US) *
                            (uint64_t)32768 / (uint64_t)BB_CLK_RATE_HZ;
        }
    } 
    else
    {
        /* Snapshot the current WUT value */
        MXC_WUT_Edge(MXC_WUT);
        preCaptureInWutCnt = MXC_WUT_GetCount(MXC_WUT);
        bleSleepTicks = 0;
        schUsec = 0;
    }

    /* Sleep for the shortest tick duration */
    if ((schTimerActive) 
        && (bleSleepTicks < idleInWutCnt))
    {
        dsInWutCnt = bleSleepTicks;
    } 
    else
    {
        dsInWutCnt = idleInWutCnt;
    }

    /* Bound the deep sleep time */
    if (dsInWutCnt > MAX_WUT_TICKS) {
        dsInWutCnt = MAX_WUT_TICKS;
    }

    /* Only enter deep sleep if we have enough time */
    if (dsInWutCnt >= MIN_WUT_TICKS) {
        /* Arm the WUT interrupt */
        MXC_WUT->cmp = preCaptureInWutCnt + dsInWutCnt;

        if (schTimerActive)
        {
            /* Stop the BLE scheduler timer */
            PalTimerStop();

            /* Shutdown BB hardware */
            PalBbDisable();
        }

        LED_Off(SLEEP_LED);
        LED_Off(DEEPSLEEP_LED);

        //MXC_LP_EnterStandbyMode();    // remove me !!!

        LED_On(DEEPSLEEP_LED);  // remove me !!!
        LED_On(SLEEP_LED);

        if (schTimerActive)
        {
            /* Enable and restore the BB hardware */
            PalBbEnable();

            PalBbRestore();

            /* Restore the BB counter */
            MXC_WUT_RestoreBBClock(MXC_WUT, BB_CLK_RATE_HZ);

            /* Restart the BLE scheduler timer */
            dsWutTicks = MXC_WUT->cnt - preCaptureInWutCnt;
            schUsecElapsed = (uint64_t)dsWutTicks * (uint64_t)1000000 / (uint64_t)32768;

            int palTimerStartTicks = schUsec - schUsecElapsed;
            if (palTimerStartTicks < 1) {
                palTimerStartTicks = 1;
            }
            PalTimerStart(palTimerStartTicks);
        }
    }

    /* Recalculate dsWutTicks for the FreeRTOS tick counter update */
    MXC_WUT_Edge(MXC_WUT);
    postCaptureInWutCnt = MXC_WUT_GetCount(MXC_WUT);
    dsWutTicks = postCaptureInWutCnt - preCaptureInWutCnt;
    
    /*
    * Advance ticks by # actually elapsed
    */
    //dsSysTickPeriods = (uint64_t)dsWutTicks * (uint64_t)1000 / (uint64_t)32768;

    /* Re-enable SysTick */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;                               /* if(not busy) */

EXIT_SLEEP_FUNC:
    /* Re-enable interrupts - see comments above */
    __asm volatile("cpsie i");
    WsfTaskUnlock();
}
#endif /* DEEP_SLEEP */

/*! \brief  Stack initialization for app. */
extern void StackInitCgm(void);

/*************************************************************************************************/
/*!
 *  \brief  Initialize WSF.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void mainWsfInit(void)
{
#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
    /* +12 for message headroom, + 2 event header, +255 maximum parameter length. */
    const uint16_t maxRptBufSize = 12 + 2 + 255;

    /* +12 for message headroom, +4 for header. */
    const uint16_t aclBufSize = 12 + mainLlRtCfg.maxAclLen + 4 + BB_DATA_PDU_TAILROOM;

    /* Adjust buffer allocation based on platform configuration. */
    mainPoolDesc[2].len = maxRptBufSize;
    mainPoolDesc[2].num = mainLlRtCfg.maxAdvReports;
    mainPoolDesc[3].len = aclBufSize;
    mainPoolDesc[3].num = mainLlRtCfg.numTxBufs + mainLlRtCfg.numRxBufs;
#endif

    const uint8_t numPools = sizeof(mainPoolDesc) / sizeof(mainPoolDesc[0]);

    uint16_t memUsed;
    WsfCsEnter();
    memUsed = WsfBufInit(numPools, mainPoolDesc);
    WsfHeapAlloc(memUsed);
    WsfCsExit();

    WsfOsInit();
    WsfTimerInit();
#if (WSF_TOKEN_ENABLED == TRUE) || (WSF_TRACE_ENABLED == TRUE)
    WsfTraceRegisterHandler(WsfBufIoWrite);
    WsfTraceEnable(TRUE);
#endif
}

/*************************************************************************************************/
/*!
*  \fn     WUT_IRQHandler
*
*  \brief  WUY interrupt handler.
*
*  \return None.
*/
/*************************************************************************************************/
void WUT_IRQHandler(void)
{
    MXC_WUT_Handler(MXC_WUT0);
}

/*************************************************************************************************/
/*!
*  \fn     wutTrimCb
*
*  \brief  Callback function for the WUT 32 kHz crystal trim.
*
*  \param  err    Error code from the WUT driver.
*
*  \return None.
*/
/*************************************************************************************************/
void wutTrimCb(int err)
{
    if (err != E_NO_ERROR) {
        APP_TRACE_INFO1("32 kHz trim error %d\n", err);
    } else {
        APP_TRACE_INFO1("32kHz trimmed to 0x%x", (MXC_TRIMSIR->rtc & MXC_F_TRIMSIR_RTC_X1TRIM) >>
                                                     MXC_F_TRIMSIR_RTC_X1TRIM_POS);
    }
    wutTrimComplete = 1;
}

/*************************************************************************************************/
/*!
*  \fn     main
*
*  \brief  Entry point for demo software.
*
*  \param  None.
*
*  \return None.
*/
/*************************************************************************************************/
int main(void)
{
#ifdef DEEP_SLEEP
    printf("\n\n***** MAX32665 BLE GGM Profile, Deep Sleep Version *****");
    printf("SystemCoreClock = %d", SystemCoreClock);
#endif /* DEEP_SLEEP */

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
    /* Configurations must be persistent. */
    static BbRtCfg_t mainBbRtCfg;

    PalBbLoadCfg((PalBbCfg_t *)&mainBbRtCfg);
    LlGetDefaultRunTimeCfg(&mainLlRtCfg);
#if (BT_VER >= LL_VER_BT_CORE_SPEC_5_0)
    /* Set 5.0 requirements. */
    mainLlRtCfg.btVer = LL_VER_BT_CORE_SPEC_5_0;
#endif
    PalCfgLoadData(PAL_CFG_ID_LL_PARAM, &mainLlRtCfg.maxAdvSets, sizeof(LlRtCfg_t) - 9);
#if (BT_VER >= LL_VER_BT_CORE_SPEC_5_0)
    PalCfgLoadData(PAL_CFG_ID_BLE_PHY, &mainLlRtCfg.phy2mSup, 4);
#endif

    /* Set the 32k sleep clock accuracy into one of the following bins, default is 20
      HCI_CLOCK_500PPM
      HCI_CLOCK_250PPM
      HCI_CLOCK_150PPM
      HCI_CLOCK_100PPM
      HCI_CLOCK_75PPM
      HCI_CLOCK_50PPM
      HCI_CLOCK_30PPM
      HCI_CLOCK_20PPM
    */
    mainBbRtCfg.clkPpm = 20;

    /* Set the default connection power level */
    mainLlRtCfg.defTxPwrLvl = DEFAULT_TX_POWER;
#endif

    uint32_t memUsed;
    WsfCsEnter();
    memUsed = WsfBufIoUartInit(WsfHeapGetFreeStartAddress(), PLATFORM_UART_TERMINAL_BUFFER_SIZE);
    WsfHeapAlloc(memUsed);
    WsfCsExit();

    mainWsfInit();
    AppTerminalInit();

#if defined(HCI_TR_EXACTLE) && (HCI_TR_EXACTLE == 1)
    WsfCsEnter();
    LlInitRtCfg_t llCfg = { .pBbRtCfg = &mainBbRtCfg,
                            .wlSizeCfg = 4,
                            .rlSizeCfg = 4,
                            .plSizeCfg = 4,
                            .pLlRtCfg = &mainLlRtCfg,
                            .pFreeMem = WsfHeapGetFreeStartAddress(),
                            .freeMemAvail = WsfHeapCountAvailable() };

    memUsed = LlInit(&llCfg);
    WsfHeapAlloc(memUsed);
    WsfCsExit();

    bdAddr_t bdAddr;
    PalCfgLoadData(PAL_CFG_ID_BD_ADDR, bdAddr, sizeof(bdAddr_t));
    LlSetBdAddr((uint8_t *)&bdAddr);

    /* Start the 32 MHz crystal and the BLE DBB counter to trim the 32 kHz crystal */
    PalBbEnable();

    /* Output buffered square wave of 32 kHz clock to GPIO */
    // MXC_RTC_SquareWaveStart(MXC_RTC_F_32KHZ);

    /* Execute the trim procedure */
    wutTrimComplete = 0;
    MXC_WUT_TrimCrystalAsync(MXC_WUT0, wutTrimCb);
    while (!wutTrimComplete) {}

    /* Shutdown the 32 MHz crystal and the BLE DBB */
    PalBbDisable();
#endif

    StackInitCgm();

    CgmStart();

    // WsfOsEnterMainLoop();
    while(TRUE)
    {
        WsfTimerSleepUpdate();

        wsfOsDispatcher();

#ifdef DEEP_SLEEP
        DeepSleep();
#else
        if (!WsfOsActive())
        {
            WsfTimerSleep();
        }
#endif
    }

    /* Does not return. Should never be reached here. */
    return 0;
}
