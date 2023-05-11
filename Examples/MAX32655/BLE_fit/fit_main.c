/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Fitness sample application for the following profiles:
 *            Heart Rate profile
 *
 *  Copyright (c) 2011-2019 Arm Ltd. All Rights Reserved.
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
#include "util/bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "app_db.h"
#include "app_ui.h"
#include "app_hw.h"
#include "app_main.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "svc_hrs.h"
#include "svc_dis.h"
#include "svc_batt.h"
#include "svc_rscs.h"
#include "gatt/gatt_api.h"
#include "bas/bas_api.h"
#include "hrps/hrps_api.h"
#include "rscp/rscp_api.h"
#include "fit_api.h"
#include "pal_btn.h"
#include "tmr.h"

/* Bluetooth Cordio library */
#include "pal_timer.h"
#include "pal_uart.h"
#include "pal_bb.h"
#include "pal_rtc.h"

#include "mxc_device.h"
#include "board.h"
#include "uart.h"
#include "mxc_assert.h"
#include "lp.h"
#include "pwrseq_regs.h"
#include "wut.h"
#include "mcr_regs.h"
#include "icc.h"
#include "simo.h"

#define RTC_TICK_RATE_HZ (32768)
#define WAKEUP_US 1500
#define BB_CLK_RATE_HZ 1000000
#define MIN_WUT_TICKS 100
#define MAX_WUT_TICKS (RTC_TICK_RATE_HZ)

/* Timer for testing frequent advert changes */
wsfHandlerId_t advTimerHandlerId;
wsfTimer_t advTimer;

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! WSF message event starting value */
#define FIT_MSG_START 0xA0

/* Default Running Speed and Cadence Measurement period (seconds) */
#define FIT_DEFAULT_RSCM_PERIOD 1

/*! WSF message event enumeration */
enum {
    FIT_HR_TIMER_IND = FIT_MSG_START, /*! Heart rate measurement timer expired */
    FIT_BATT_TIMER_IND, /*! Battery measurement timer expired */
    FIT_RUNNING_TIMER_IND /*! Running speed and cadence measurement timer expired */
};

/*! Button press handling constants */
#define BTN_SHORT_MS 200
#define BTN_MED_MS 500
#define BTN_LONG_MS 1000

#define BTN_1_TMR MXC_TMR2
#define BTN_2_TMR MXC_TMR3

typedef struct {
    uint8_t goingToSleep;
    uint8_t wokeUp;
    uint32_t sleepStart;
    uint32_t sleepEnd;
    uint32_t sleepDelta;
    bool_t readyToAdvertise;
} stateCounter_t;
stateCounter_t state = { 0, 0, 0, 0, 0, FALSE };

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Application message type */
typedef union {
    wsfMsgHdr_t hdr;
    dmEvt_t dm;
    attsCccEvt_t ccc;
    attEvt_t att;
} fitMsg_t;

/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! configurable parameters for advertising */
static const appAdvCfg_t fitAdvCfg = {
    { 65535, 0, 0 }, /*! Advertising durations in ms, 60000*/
    { 1280, 0, 0 } /*! Advertising intervals in 0.625 ms units, 800 */
};

/*! configurable parameters for slave */
static const appSlaveCfg_t fitSlaveCfg = {
    FIT_CONN_MAX, /*! Maximum connections */
};

/*! configurable parameters for security */
static const appSecCfg_t fitSecCfg = {
    DM_AUTH_BOND_FLAG | DM_AUTH_SC_FLAG, /*! Authentication and bonding flags */
    0, /*! Initiator key distribution flags */
    DM_KEY_DIST_LTK, /*! Responder key distribution flags */
    FALSE, /*! TRUE if Out-of-band pairing data is present */
    TRUE /*! TRUE to initiate security upon connection */
};

/*! configurable parameters for connection parameter update */
static const appUpdateCfg_t fitUpdateCfg = {
    6000,
    /*! ^ Connection idle period in ms before attempting
    connection parameter update; set to zero to disable */
    640, /*! Minimum connection interval in 1.25ms units */
    800, /*! Maximum connection interval in 1.25ms units */
    0, /*! Connection latency */
    900, /*! Supervision timeout in 10ms units */
    5 /*! Number of update attempts before giving up */
};

/*! heart rate measurement configuration */
static const hrpsCfg_t fitHrpsCfg = {
    2000 /*! Measurement timer expiration period in ms */
};

/*! battery measurement configuration */
static const basCfg_t fitBasCfg = {
    30, /*! Battery measurement timer expiration period in seconds */
    1, /*! Perform battery measurement after this many timer periods */
    100 /*! Send battery level notification to peer when below this level. */
};

/*! SMP security parameter configuration */
static const smpCfg_t fitSmpCfg = {
    500, /*! 'Repeated attempts' timeout in msec */
    SMP_IO_NO_IN_NO_OUT, /*! I/O Capability */
    7, /*! Minimum encryption key length */
    16, /*! Maximum encryption key length */
    1, /*! Attempts to trigger 'repeated attempts' timeout */
    0, /*! Device authentication requirements */
    64000, /*! Maximum repeated attempts timeout in msec */
    64000, /*! Time msec before attemptExp decreases */
    2 /*! Repeated attempts multiplier exponent */
};

/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! advertising data, discoverable mode */
static uint8_t fitAdvDataDisc[] = {
    /*! flags */
    2, /*! length */
    DM_ADV_TYPE_FLAGS, /*! AD type */
    DM_FLAG_LE_GENERAL_DISC | /*! flags */
        DM_FLAG_LE_BREDR_NOT_SUP,

    /*! tx power */
    6, /*! length */
    DM_ADV_TYPE_MANUFACTURER, /*! AD type */
    UINT16_TO_BYTES(HCI_ID_ANALOG), /*! company ID */
    'c',
    'g',
    'm'

    /*! service UUID list */
    //9, /*! length */
    //DM_ADV_TYPE_16_UUID, /*! AD type */
    //UINT16_TO_BYTES(ATT_UUID_HEART_RATE_SERVICE), UINT16_TO_BYTES(ATT_UUID_RUNNING_SPEED_SERVICE),
    //UINT16_TO_BYTES(ATT_UUID_DEVICE_INFO_SERVICE), UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)
};

/*! scan data, discoverable mode */
static const uint8_t fitScanDataDisc[] = {
    /*! device name */
    4, /*! length */
    DM_ADV_TYPE_LOCAL_NAME, /*! AD type */
    'C', 'G', 'M'
};

/*when updating advertising data using AppAdvSetAdValue, 
  there is no need to pass all the flags */
uint8_t newData1[] = { UINT16_TO_BYTES(HCI_ID_ANALOG), /*! company ID */
                       'd', 'a', 't', 'a' };
uint8_t newData2[] = { UINT16_TO_BYTES(HCI_ID_ANALOG), /*! company ID */
                       't',
                       'e',
                       's',
                       't',
                       'i',
                       'n',
                       'g' };
/**************************************************************************************************
  Client Characteristic Configuration Descriptors
**************************************************************************************************/

/*! enumeration of client characteristic configuration descriptors */
enum {
    FIT_GATT_SC_CCC_IDX, /*! GATT service, service changed characteristic */
    FIT_HRS_HRM_CCC_IDX, /*! Heart rate service, heart rate monitor characteristic */
    FIT_BATT_LVL_CCC_IDX, /*! Battery service, battery level characteristic */
    FIT_RSCS_SM_CCC_IDX, /*! Runninc speed and cadence measurement characteristic */
    FIT_NUM_CCC_IDX
};

/*! client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t fitCccSet[FIT_NUM_CCC_IDX] = {
    /* cccd handle          value range               security level */
    { GATT_SC_CH_CCC_HDL, ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE }, /* FIT_GATT_SC_CCC_IDX */
    { HRS_HRM_CH_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE }, /* FIT_HRS_HRM_CCC_IDX */
    { BATT_LVL_CH_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE }, /* FIT_BATT_LVL_CCC_IDX */
    { RSCS_RSM_CH_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE } /* FIT_RSCS_SM_CCC_IDX */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t fitHandlerId;

/* WSF Timer to send running speed and cadence measurement data */
wsfTimer_t fitRscmTimer;

/* Running Speed and Cadence Measurement period - Can be changed at runtime to vary period */
static uint16_t fitRscmPeriod = FIT_DEFAULT_RSCM_PERIOD;

/* Heart Rate Monitor feature flags */
static uint8_t fitHrmFlags = CH_HRM_FLAGS_VALUE_8BIT | CH_HRM_FLAGS_ENERGY_EXP;

/******************* Deep sleep stuff *****************************/
int allow_sleep(void)
{
    /* Can not disable BLE DBB and 32 MHz clock while trim procedure is ongoing */
    if (MXC_WUT_TrimPending(MXC_WUT0) != E_NO_ERROR) {
        APP_TRACE_INFO0("Cant sleep Trim pending");
        return E_BUSY;
    }

    /* I disable checking uart because i need to see debug messages  */
    /* Figure out if the UART is active */
    while (PalUartGetState(PAL_UART_ID_TERMINAL) != PAL_UART_STATE_READY) {
        //APP_TRACE_INFO0("UART busy");
        // return E_BUSY;
    }

    /* Prevent characters from being corrupted if still transmitting,
      UART will shutdown in deep sleep  */
    if (MXC_UART_GetActive(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {
        // APP_TRACE_INFO0("UART active");
        //  return E_BUSY;
    }

    return E_NO_ERROR;
}

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

static void deepSleep(void)
{
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

    // DEBUG ----------------------------
    state.goingToSleep = 1;
    state.sleepStart = PalRtcCounterGet();
    //-----------------------------------

    MXC_LP_EnterDeepSleepMode();

    // DEBUG ----------------------------
    state.sleepEnd = PalRtcCounterGet();
    state.sleepDelta = state.sleepEnd - state.sleepStart;
    state.wokeUp = 1;
    //-----------------------------------

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
}

/*************************************************************************************************/
/*!
 *  \brief  Application DM callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitDmCback(dmEvt_t *pDmEvt)
{
    dmEvt_t *pMsg;
    uint16_t len;

    len = DmSizeOfEvt(pDmEvt);

    if ((pMsg = WsfMsgAlloc(len)) != NULL) {
        memcpy(pMsg, pDmEvt, len);
        WsfMsgSend(fitHandlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Application ATT callback.
 *
 *  \param  pEvt    ATT callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitAttCback(attEvt_t *pEvt)
{
    attEvt_t *pMsg;

    if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL) {
        memcpy(pMsg, pEvt, sizeof(attEvt_t));
        pMsg->pValue = (uint8_t *)(pMsg + 1);
        memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
        WsfMsgSend(fitHandlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Application ATTS client characteristic configuration callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitCccCback(attsCccEvt_t *pEvt)
{
    attsCccEvt_t *pMsg;
    appDbHdl_t dbHdl;

    /* If CCC not set from initialization and there's a device record and currently bonded */
    if ((pEvt->handle != ATT_HANDLE_NONE) &&
        ((dbHdl = AppDbGetHdl((dmConnId_t)pEvt->hdr.param)) != APP_DB_HDL_NONE) &&
        AppCheckBonded((dmConnId_t)pEvt->hdr.param)) {
        /* Store value in device database. */
        AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
    }

    if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL) {
        memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
        WsfMsgSend(fitHandlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
*  \brief  Send a Running Speed and Cadence Measurement Notification.
*
*  \param  connId    connection ID
*
*  \return None.
*/
/*************************************************************************************************/
static void fitSendRunningSpeedMeasurement(dmConnId_t connId)
{
    if (AttsCccEnabled(connId, FIT_RSCS_SM_CCC_IDX)) {
        static uint8_t walk_run = 1;

        /* TODO: Set Running Speed and Cadence Measurement Parameters */

        RscpsSetParameter(RSCP_SM_PARAM_SPEED, 1);
        RscpsSetParameter(RSCP_SM_PARAM_CADENCE, 2);
        RscpsSetParameter(RSCP_SM_PARAM_STRIDE_LENGTH, 3);
        RscpsSetParameter(RSCP_SM_PARAM_TOTAL_DISTANCE, 4);

        /* Toggle running/walking */
        walk_run = walk_run ? 0 : 1;
        RscpsSetParameter(RSCP_SM_PARAM_STATUS, walk_run);

        RscpsSendSpeedMeasurement(connId);
    }

    /* Configure and start timer to send the next measurement */
    fitRscmTimer.msg.event = FIT_RUNNING_TIMER_IND;
    fitRscmTimer.msg.status = FIT_RSCS_SM_CCC_IDX;
    fitRscmTimer.handlerId = fitHandlerId;
    fitRscmTimer.msg.param = connId;

    WsfTimerStartSec(&fitRscmTimer, fitRscmPeriod);
}

/*************************************************************************************************/
/*!
 *  \brief  Process CCC state change.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitProcCccState(fitMsg_t *pMsg)
{
    APP_TRACE_INFO3("ccc state ind value:%d handle:%d idx:%d", pMsg->ccc.value, pMsg->ccc.handle,
                    pMsg->ccc.idx);

    /* handle heart rate measurement CCC */
    if (pMsg->ccc.idx == FIT_HRS_HRM_CCC_IDX) {
        if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY) {
            HrpsMeasStart((dmConnId_t)pMsg->ccc.hdr.param, FIT_HR_TIMER_IND, FIT_HRS_HRM_CCC_IDX);
        } else {
            HrpsMeasStop((dmConnId_t)pMsg->ccc.hdr.param);
        }
        return;
    }

    /* handle running speed and cadence measurement CCC */
    if (pMsg->ccc.idx == FIT_RSCS_SM_CCC_IDX) {
        if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY) {
            fitSendRunningSpeedMeasurement((dmConnId_t)pMsg->ccc.hdr.param);
        } else {
            WsfTimerStop(&fitRscmTimer);
        }
        return;
    }

    /* handle battery level CCC */
    if (pMsg->ccc.idx == FIT_BATT_LVL_CCC_IDX) {
        if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY) {
            BasMeasBattStart((dmConnId_t)pMsg->ccc.hdr.param, FIT_BATT_TIMER_IND,
                             FIT_BATT_LVL_CCC_IDX);
        } else {
            BasMeasBattStop((dmConnId_t)pMsg->ccc.hdr.param);
        }
        return;
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Perform UI actions on connection close.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitClose(fitMsg_t *pMsg)
{
    /* stop heart rate measurement */
    HrpsMeasStop((dmConnId_t)pMsg->hdr.param);

    /* stop battery measurement */
    BasMeasBattStop((dmConnId_t)pMsg->hdr.param);

    /* Stop running speed and cadence timer */
    WsfTimerStop(&fitRscmTimer);
}

/*************************************************************************************************/
/*!
 *  \brief  Set up advertising and other procedures that need to be performed after
 *          device reset.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitSetup(fitMsg_t *pMsg)
{
    /* set advertising and scan response data for discoverable mode */
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(fitAdvDataDisc), (uint8_t *)fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(fitScanDataDisc), (uint8_t *)fitScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);

    /* start advertising; automatically set connectable/discoverable mode and bondable mode */
    AppAdvStart(APP_MODE_AUTO_INIT);
}

void enterDeepSleep(void)
{
    bool_t schTimerActive;
    uint32_t preCapture, schUsec, dsTicks, dsWutTicks;
    uint64_t bleSleepTicks, idleTicks, schUsecElapsed;

    if (allow_sleep() != E_NO_ERROR) {
        // re enable interupts
        __asm volatile("cpsie i");
        APP_TRACE_INFO0("Allow sleep failed,");
        return;
    }

    /* Calculate the number of WUT ticks, but we need one to synchronize */
    // idleTicks = (uint64_t)(xExpectedIdleTime - 1) * (uint64_t)configRTC_TICK_RATE_HZ /
    //             (uint64_t)configTICK_RATE_HZ;
    idleTicks = MAX_WUT_TICKS;

    if (idleTicks > MAX_WUT_TICKS) {
        idleTicks = MAX_WUT_TICKS;
    }

    /* Check to see if we meet the minimum requirements for deep sleep */
    if (idleTicks < (MIN_WUT_TICKS + WAKEUP_US)) {
        return;
    }

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
       method as that will mask interrupts that should exit sleep mode. */
    __asm volatile("cpsid i");

    /* Determine if the Bluetooth scheduler is running */
    if (PalTimerGetState() == PAL_TIMER_STATE_BUSY) {
        schTimerActive = TRUE;
    } else {
        schTimerActive = FALSE;
    }
    if (!schTimerActive) {
        uint32_t ts;
        if (PalBbGetTimestamp(&ts)) {
            /*Determine if PalBb is active, return if we get a valid time stamp indicating 
            that the scheduler is waiting for a PalBb event */
            __asm volatile("cpsie i");
            APP_TRACE_INFO0("PalBb active");
            return;
        }
    }
    /* Disable SysTick */
    //@ PAUL this might be freetos related
    //SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);

    /* Enable wakeup from WUT */
    // WUT is the source for PAL_RTC
    NVIC_EnableIRQ(WUT_IRQn);
    MXC_LP_EnableWUTAlarmWakeup();

    if (schTimerActive) {
        /* Snapshot the current WUT value with the PalBb clock */
        MXC_WUT_Store();
        preCapture = MXC_WUT_GetCount();
        schUsec = PalTimerGetExpTime();

        // /* Adjust idleTicks for the time it takes to restart the BLE hardware */
        idleTicks -= ((WAKEUP_US)*RTC_TICK_RATE_HZ / 1000000);

        /* Calculate the time to the next BLE scheduler event */
        if (schUsec < WAKEUP_US) {
            bleSleepTicks = 0;
        } else {
            bleSleepTicks = ((uint64_t)schUsec - (uint64_t)WAKEUP_US) * (uint64_t)RTC_TICK_RATE_HZ /
                            (uint64_t)BB_CLK_RATE_HZ;
        }
    } else {
        /* Snapshot the current WUT value */
        MXC_WUT_Edge();
        preCapture = MXC_WUT_GetCount();
        bleSleepTicks = 0;
        schUsec = 0;
    }

    /* Sleep for the shortest tick duration */
    if ((schTimerActive) && (bleSleepTicks < idleTicks)) {
        dsTicks = bleSleepTicks;
    } else {
        dsTicks = idleTicks;
    }

    /* Bound the deep sleep time */
    if (dsTicks > MAX_WUT_TICKS) {
        dsTicks = MAX_WUT_TICKS;
    }

    /* Don't deep sleep if we don't have time */
    if (dsTicks >= MIN_WUT_TICKS) {
        /* Arm the WUT interrupt */
        MXC_WUT->cmp = preCapture + dsTicks;

        if (schTimerActive) {
            /* Stop the BLE scheduler timer */
            PalTimerStop();

            /* Shutdown BB hardware */
            PalBbDisable();
        }

        deepSleep();

        if (schTimerActive) {
            /* Enable and restore the BB hardware */
            PalBbEnable();

            PalBbRestore();

            /* Restore the BB counter */
            MXC_WUT_RestoreBBClock(BB_CLK_RATE_HZ);

            /* Restart the BLE scheduler timer */
            dsWutTicks = MXC_WUT->cnt - preCapture;
            schUsecElapsed = (uint64_t)dsWutTicks * (uint64_t)1000000 / (uint64_t)RTC_TICK_RATE_HZ;

            int palTimerStartTicks = schUsec - schUsecElapsed;
            if (palTimerStartTicks < 1) {
                palTimerStartTicks = 1;
            }
            PalTimerStart(palTimerStartTicks);
        }

    } else {
        APP_TRACE_INFO0("Not enough time to deep sleep");
    }
}
/*************************************************************************************************/
/*!
 *  \brief  Button press callback.
 *
 *  \param  btn    Button press.
 *
 *  \return None.
 */
/*************************************************************************************************/
void advTimerHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    static bool_t swtich = FALSE;

    /* switching between two advertising packets of same length */
    if (swtich) {
        AppAdvSetAdValue(APP_ADV_DATA_DISCOVERABLE, DM_ADV_TYPE_MANUFACTURER, sizeof(newData1),
                         (uint8_t *)newData1);
        swtich = FALSE;
    } else {
        AppAdvSetAdValue(APP_ADV_DATA_DISCOVERABLE, DM_ADV_TYPE_MANUFACTURER, sizeof(newData2),
                         (uint8_t *)newData2);
        swtich = TRUE;
    }
    /* enter deep sleep */
    //enterDeepSleep();
    /* Continue next erase */
    WsfTimerStartMs(&advTimer, 1000);
}
static void fitBtnCback(uint8_t btn)
{
    static bool_t switchData = FALSE;
    dmConnId_t connId;
    static uint8_t heartRate = 78; /* for testing/demonstration */

    /* button actions when connected */
    if ((connId = AppConnIsOpen()) != DM_CONN_ID_NONE) {
        switch (btn) {
        case APP_UI_BTN_1_SHORT:
            /* increment the heart rate */
            AppHwHrmTest(++heartRate);
            break;

        case APP_UI_BTN_1_MED:
            break;

        case APP_UI_BTN_1_LONG:
            AppConnClose(connId);
            break;

        case APP_UI_BTN_2_SHORT:
            /* decrement the heart rate */
            AppHwHrmTest(--heartRate);
            break;

        case APP_UI_BTN_2_MED:
            /* Toggle HRM Sensor DET flags */
            if (!(fitHrmFlags & (CH_HRM_FLAGS_SENSOR_DET | CH_HRM_FLAGS_SENSOR_NOT_DET))) {
                fitHrmFlags |= CH_HRM_FLAGS_SENSOR_DET;
            } else if (fitHrmFlags & CH_HRM_FLAGS_SENSOR_DET) {
                fitHrmFlags &= ~CH_HRM_FLAGS_SENSOR_DET;
                fitHrmFlags |= CH_HRM_FLAGS_SENSOR_NOT_DET;
            } else {
                fitHrmFlags &= ~CH_HRM_FLAGS_SENSOR_NOT_DET;
            }

            HrpsSetFlags(fitHrmFlags);
            break;

        case APP_UI_BTN_2_LONG:
            /* Toggle HRM RR Interval feature flag */
            if (fitHrmFlags & CH_HRM_FLAGS_RR_INTERVAL) {
                fitHrmFlags &= ~CH_HRM_FLAGS_RR_INTERVAL;
            } else {
                fitHrmFlags |= CH_HRM_FLAGS_RR_INTERVAL;
            }

            HrpsSetFlags(fitHrmFlags);
            break;

        default:
            break;
        }
    } else { /* button actions when not connected */
        switch (btn) {
        case APP_UI_BTN_1_SHORT:
            AppAdvStart(APP_MODE_AUTO_INIT);
            break;

        case APP_UI_BTN_1_MED:
            /* enter discoverable and bondable mode */
            AppSetBondable(TRUE);
            AppAdvStart(APP_MODE_DISCOVERABLE);
            break;

        case APP_UI_BTN_1_LONG:
            /* clear all bonding info */
            AppSlaveClearAllBondingInfo();
            break;

        case APP_UI_BTN_2_SHORT:

            // AppAdvStop();

            if (switchData) {
                AppAdvSetAdValue(APP_ADV_DATA_DISCOVERABLE, DM_ADV_TYPE_MANUFACTURER,
                                 sizeof(newData2), (uint8_t *)newData2);
            } else {
                AppAdvSetAdValue(APP_ADV_DATA_DISCOVERABLE, DM_ADV_TYPE_MANUFACTURER,
                                 sizeof(newData1), (uint8_t *)newData1);
            }
            enterDeepSleep(); // to test if adv resumes after deep sleep

            if (state.sleepDelta) {
                APP_TRACE_INFO1("Slept for %d ticks", state.sleepDelta);
                //reset state struct
                memset(&state, 0, sizeof(stateCounter_t));
            }
            switchData = !switchData;
            // flag to restart advertising on next wsf loop
            //state.readyToAdvertise = TRUE;

            break;

        default:
            break;
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitProcMsg(fitMsg_t *pMsg)
{
    uint8_t uiEvent = APP_UI_NONE;

    switch (pMsg->hdr.event) {
    case FIT_RUNNING_TIMER_IND:
        fitSendRunningSpeedMeasurement((dmConnId_t)pMsg->ccc.hdr.param);
        break;

    case FIT_HR_TIMER_IND:
        HrpsProcMsg(&pMsg->hdr);
        break;

    case FIT_BATT_TIMER_IND:
        BasProcMsg(&pMsg->hdr);
        break;

    case ATTS_HANDLE_VALUE_CNF:
        HrpsProcMsg(&pMsg->hdr);
        BasProcMsg(&pMsg->hdr);
        break;

    case ATTS_CCC_STATE_IND:
        fitProcCccState(pMsg);
        break;

    case DM_RESET_CMPL_IND:
        AttsCalculateDbHash();
        DmSecGenerateEccKeyReq();
        fitSetup(pMsg);
        uiEvent = APP_UI_RESET_CMPL;
        break;

    case DM_ADV_SET_START_IND:
        uiEvent = APP_UI_ADV_SET_START_IND;
        break;

    case DM_ADV_SET_STOP_IND:
        uiEvent = APP_UI_ADV_SET_STOP_IND;
        break;

    case DM_ADV_START_IND:
        uiEvent = APP_UI_ADV_START;
        break;

    case DM_ADV_STOP_IND:
        uiEvent = APP_UI_ADV_STOP;
        break;

    case DM_CONN_OPEN_IND:
        HrpsProcMsg(&pMsg->hdr);
        BasProcMsg(&pMsg->hdr);
        uiEvent = APP_UI_CONN_OPEN;
        break;

    case DM_CONN_CLOSE_IND:
        fitClose(pMsg);
        uiEvent = APP_UI_CONN_CLOSE;
        break;

    case DM_SEC_PAIR_CMPL_IND:
        DmSecGenerateEccKeyReq();
        uiEvent = APP_UI_SEC_PAIR_CMPL;
        break;

    case DM_SEC_PAIR_FAIL_IND:
        DmSecGenerateEccKeyReq();
        uiEvent = APP_UI_SEC_PAIR_FAIL;
        break;

    case DM_SEC_ENCRYPT_IND:
        uiEvent = APP_UI_SEC_ENCRYPT;
        break;

    case DM_SEC_ENCRYPT_FAIL_IND:
        uiEvent = APP_UI_SEC_ENCRYPT_FAIL;
        break;

    case DM_SEC_AUTH_REQ_IND:
        AppHandlePasskey(&pMsg->dm.authReq);
        break;

    case DM_SEC_ECC_KEY_IND:
        DmSecSetEccKey(&pMsg->dm.eccMsg.data.key);
        break;

    case DM_SEC_COMPARE_IND:
        AppHandleNumericComparison(&pMsg->dm.cnfInd);
        break;

    case DM_PRIV_CLEAR_RES_LIST_IND:
        APP_TRACE_INFO1("Clear resolving list status 0x%02x", pMsg->hdr.status);
        break;

    case DM_HW_ERROR_IND:
        uiEvent = APP_UI_HW_ERROR;
        break;

    case DM_VENDOR_SPEC_IND:
        // restart advertising
        state.sleepEnd = PalRtcCounterGet();

        APP_TRACE_INFO1("TIME to restart advt: %d", state.sleepEnd - state.sleepStart);
        AppAdvStart(APP_MODE_AUTO_INIT);

    default:
        break;
    }

    if (state.readyToAdvertise) {
        AppAdvStart(APP_MODE_AUTO_INIT);
        state.readyToAdvertise = FALSE;
    }

    if (uiEvent != APP_UI_NONE) {
        AppUiAction(uiEvent);
    }
}

/*************************************************************************************************/
/*!
 *  \brief     Platform button press handler.
 *
 *  \param[in] btnId  button ID.
 *  \param[in] state  button state. See ::PalBtnPos_t.
 *
 *  \return    None.
 */
/*************************************************************************************************/
static void btnPressHandler(uint8_t btnId, PalBtnPos_t state)
{
    if (btnId == 1) {
        /* Start/stop button timer */
        if (state == PAL_BTN_POS_UP) {
            /* Button Up, stop the timer, call the action function */
            unsigned btnUs = MXC_TMR_SW_Stop(BTN_1_TMR);
            if ((btnUs > 0) && (btnUs < BTN_SHORT_MS * 1000)) {
                AppUiBtnTest(APP_UI_BTN_1_SHORT);
            } else if (btnUs < BTN_MED_MS * 1000) {
                AppUiBtnTest(APP_UI_BTN_1_MED);
            } else if (btnUs < BTN_LONG_MS * 1000) {
                AppUiBtnTest(APP_UI_BTN_1_LONG);
            } else {
                AppUiBtnTest(APP_UI_BTN_1_EX_LONG);
            }
        } else {
            /* Button down, start the timer */
            MXC_TMR_SW_Start(BTN_1_TMR);
        }
    } else if (btnId == 2) {
        /* Start/stop button timer */
        if (state == PAL_BTN_POS_UP) {
            /* Button Up, stop the timer, call the action function */
            unsigned btnUs = MXC_TMR_SW_Stop(BTN_2_TMR);
            if ((btnUs > 0) && (btnUs < BTN_SHORT_MS * 1000)) {
                AppUiBtnTest(APP_UI_BTN_2_SHORT);
            } else if (btnUs < BTN_MED_MS * 1000) {
                AppUiBtnTest(APP_UI_BTN_2_MED);
            } else if (btnUs < BTN_LONG_MS * 1000) {
                AppUiBtnTest(APP_UI_BTN_2_LONG);
            } else {
                AppUiBtnTest(APP_UI_BTN_2_EX_LONG);
            }
        } else {
            /* Button down, start the timer */
            MXC_TMR_SW_Start(BTN_2_TMR);
        }
    } else {
        APP_TRACE_ERR0("Undefined button");
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void FitHandlerInit(wsfHandlerId_t handlerId)
{
    APP_TRACE_INFO0("FitHandlerInit");

    /* store handler ID */
    fitHandlerId = handlerId;

    /* Set configuration pointers */
    pAppAdvCfg = (appAdvCfg_t *)&fitAdvCfg;
    pAppSlaveCfg = (appSlaveCfg_t *)&fitSlaveCfg;
    pAppSecCfg = (appSecCfg_t *)&fitSecCfg;
    pAppUpdateCfg = (appUpdateCfg_t *)&fitUpdateCfg;

    /* Initialize application framework */
    AppSlaveInit();
    AppServerInit();

    /* Set stack configuration pointers */
    pSmpCfg = (smpCfg_t *)&fitSmpCfg;

    /* initialize heart rate profile sensor */
    HrpsInit(handlerId, (hrpsCfg_t *)&fitHrpsCfg);
    HrpsSetFlags(fitHrmFlags);

    /* initialize battery service server */
    BasInit(handlerId, (basCfg_t *)&fitBasCfg);
}

/*************************************************************************************************/
/*!
 *  \brief  WSF event handler for application.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void FitHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    if (pMsg != NULL) {
        APP_TRACE_INFO1("Fit got evt %d", pMsg->event);

        /* process ATT messages */
        if (pMsg->event >= ATT_CBACK_START && pMsg->event <= ATT_CBACK_END) {
            /* process server-related ATT messages */
            AppServerProcAttMsg(pMsg);
        } else if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END) {
            /* process DM messages */
            /* process advertising and connection-related messages */
            AppSlaveProcDmMsg((dmEvt_t *)pMsg);

            /* process security-related messages */
            AppSlaveSecProcDmMsg((dmEvt_t *)pMsg);
        }

        /* perform profile and user interface-related operations */
        fitProcMsg((fitMsg_t *)pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void FitStart(void)
{
    /* Register for stack callbacks */
    DmRegister(fitDmCback);
    DmConnRegister(DM_CLIENT_ID_APP, fitDmCback);
    AttRegister(fitAttCback);
    AttConnRegister(AppServerConnCback);
    AttsCccRegister(FIT_NUM_CCC_IDX, (attsCccSet_t *)fitCccSet, fitCccCback);

    /* Register for app framework callbacks */
    AppUiBtnRegister(fitBtnCback);
    /* Initialize with button press handler */
    PalBtnInit(btnPressHandler);
    /* Initialize attribute server database */
    SvcCoreGattCbackRegister(GattReadCback, GattWriteCback);
    SvcCoreAddGroup();
    SvcHrsCbackRegister(NULL, HrpsWriteCback);
    SvcHrsAddGroup();
    SvcDisAddGroup();
    SvcBattCbackRegister(BasReadCback, NULL);
    SvcBattAddGroup();
    SvcRscsAddGroup();

    /* Set Service Changed CCCD index. */
    GattSetSvcChangedIdx(FIT_GATT_SC_CCC_IDX);

    /* Set running speed and cadence features */
    RscpsSetFeatures(RSCS_ALL_FEATURES);

    /* Reset the device */
    DmDevReset();
}
