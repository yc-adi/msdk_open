/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  CGM sample application.
 *
 *  Copyright (c) 2012-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
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
#include "app_db.h"
#include "led.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "wsf_types.h"
#include "util/bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "wsf_nvm.h"
#include "wsf_timer.h"
#include "hci_api.h"
#include "sec_api.h"
#include "dm_api.h"
#include "smp_api.h"
#include "att_api.h"
#include "app_api.h"
#include "app_main.h"
#include "app_db.h"
#include "app_ui.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "svc_wp.h"
#include "util/calc128.h"
#include "gatt/gatt_api.h"
#include "cgm_api.h"
#include "wut.h"
#include "trimsir_regs.h"
#include "pal_btn.h"
#include "pal_uart.h"
#include "tmr.h"
#include "wsf_efs.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "svc_cgms.h"
#include "svc_dis.h"
#include "svc_wdxs.h"
#include "wdxs/wdxs_api.h"
#include "wdxs/wdxs_main.h"
#include "wdxs/wdxs_stream.h"
#include "wdxs_file.h"
#include "board.h"
#include "flc.h"
#include "wsf_cs.h"
#include "Ext_Flash.h"
#include "cgmps/cgmps_api.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
#if (BT_VER > 8)

/* PHY Test Modes */
#define DATS_PHY_2M 2
#define DATS_PHY_1M 1
#define DATS_PHY_CODED 3

#endif /* BT_VER */

#define TRIM_TIMER_EVT 0x99
#define CUST_SPEC_TMR_EVT 0x9A
#define CGM_MEAS_TMR_EVT 0x9B

#define TRIM_TIMER_PERIOD_MS 100000
#define CUST_SPEC_TMR_PERIOD_MS 60000
#define CGM_MEAS_PERIOD_MS 5000

/*! Button press handling constants */
#define BTN_SHORT_MS 200
#define BTN_MED_MS 500
#define BTN_LONG_MS 1000

#define BTN_1_TMR MXC_TMR2
#define BTN_2_TMR MXC_TMR3

#ifndef OTA_INTERNAL
#define OTA_INTERNAL 0
#endif

/******************
 * DEBUG USE
*******************/
uint32_t u32DbgBuf[DBG_BUF_SIZE];
uint32_t u32DbgBufNdx = 0;
uint8_t  u8DbgSt = 0;

void print_dbg_buf(void)
{
    int GroupCnt = DBG_GROUP_SIZE;
    uint32_t printed, str_pos = 0, j = 0;
    char temp[100];
    for (printed = 0; printed < DBG_BUF_SIZE ; ++printed)
    {
        #if 0
        if ((printed % GroupCnt) < (GroupCnt - 1))
        {
            str_pos += sprintf(&temp[str_pos], "%d,", u32DbgBuf[printed]);
        }
        else
        {
            str_pos += sprintf(&temp[str_pos], "%d", u32DbgBuf[printed]);
            APP_TRACE_INFO1("%s", temp);
            str_pos = 0;
        }
        #endif
        if (++j <= GroupCnt && u32DbgBuf[printed + 1] != 66)
        {
            str_pos += sprintf(&temp[str_pos], "%u,", u32DbgBuf[printed]);
        }
        else
        {
            str_pos += sprintf(&temp[str_pos], "%u", u32DbgBuf[printed]);
            //PalUartWriteData(PAL_UART_ID_TERMINAL, (const uint8_t *)temp, str_pos);
            APP_TRACE_INFO1("%s", temp);
            str_pos = 0;
            j = 0;
        }
    }
    u32DbgBufNdx = 0;
    u8DbgSt = 0;
}

/**************************************************************************************************
  Client Characteristic Configuration Descriptors (CCCD)
**************************************************************************************************/

/*! client characteristic configuration descriptors settings, indexed by cgm_app_ccc_idx */
static const attsCccSet_t cgmCccSet[CGM_CCC_IDX_MAX] =
{
  /* cccd handle          value range               security level */
  {GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_ENC},    /* GATT_SC_CCC_IDX */
//{GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE },  /* GATT_SC_CCC_IDX */

  {WDXS_DC_CH_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE },  /* WDXS_DC_CH_CCC_IDX */
  {WDXS_FTC_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE },  /* WDXS_FTC_CH_CCC_IDX */
  {WDXS_FTD_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE },  /* WDXS_FTD_CH_CCC_IDX */
  {WDXS_AU_CH_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE },  /* WDXS_AU_CH_CCC_IDX */
  
  {WP_DAT_CH_CCC_HDL,     ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE },  /* DATS_WP_DAT_CCC_IDX */

  {CGMS_MEAS_CH_CCC_HDL,  ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_ENC},    /* CGM_MEAS_CCC_IDX */
  {CGMS_RACP_CH_CCC_HDL,  ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_ENC}     /* CGM_RACP_CCC_IDX */
  //{CGMS_SOPS_CCCD_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_ENC}     /* CGM_SOPS_CCC_IDX */
};

/*! Application message type */
typedef union {
    wsfMsgHdr_t hdr;
    dmEvt_t dm;
    attsCccEvt_t ccc;
    attEvt_t att;
} cgmMsg_t;

/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! configurable parameters for advertising */
static const appAdvCfg_t cgmAdvCfg = {
    { 0, 0, 0 }, /*! Advertising durations in ms */
    { 1280, 1280, 0 } /*! Advertising intervals in 0.625 ms units (20 ms to 10.24 secs) */
                      /* random delay from 0ms to 10ms is automatically added.  */
};

/*! configurable parameters for slave */
static const appSlaveCfg_t cgmSlaveCfg = {
    CGM_CONN_MAX, /*! Maximum connections */
};

/*! configurable parameters for security */
static const appSecCfg_t cgmSecCfg = {
    //DM_AUTH_BOND_FLAG | DM_AUTH_MITM_FLAG | DM_AUTH_SC_FLAG, /*! auth: Authentication and bonding flags */
    DM_AUTH_BOND_FLAG | DM_AUTH_MITM_FLAG, /*! auth: Authentication and bonding flags */
    DM_KEY_DIST_IRK, /*! iKeyDist: Initiator key distribution flags */
    DM_KEY_DIST_LTK | DM_KEY_DIST_IRK, /*! rKeyDist: Responder key distribution flags */
    //DM_KEY_DIST_LTK, // TODO: check the diff
    FALSE, /*! oob: TRUE if Out-of-band pairing data is present */
    TRUE /*! initiateSec: TRUE to initiate security upon connection */
};

/*! TRUE if Out-of-band pairing data is to be sent */
static const bool_t cgmSendOobData = FALSE;

/*! SMP security parameter configuration 
*
*   I/O Capability Codes to be set for 
*   Pairing Request (SMP_CMD_PAIR_REQ) packets and Pairing Response (SMP_CMD_PAIR_RSP) packets
*   when the MITM flag is set in Configurable security parameters above.
*       -SMP_IO_DISP_ONLY         : Display only. 
*       -SMP_IO_DISP_YES_NO       : Display yes/no. 
*       -SMP_IO_KEY_ONLY          : Keyboard only.
*       -SMP_IO_NO_IN_NO_OUT      : No input, no output. 
*       -SMP_IO_KEY_DISP          : Keyboard display. 
*/
static const smpCfg_t cgmSmpCfg = {
    500, /*! 'Repeated attempts' timeout in msec */
    SMP_IO_DISP_ONLY, /*! I/O Capability */
    7, /*! Minimum encryption key length */
    16, /*! Maximum encryption key length */
    1, /*! Attempts to trigger 'repeated attempts' timeout */
    0, /*! Device authentication requirements */
    64000, /*! Maximum repeated attempts timeout in msec */
    64000, /*! Time msec before attemptExp decreases */
    2 /*! Repeated attempts multiplier exponent */
};

/* iOS connection parameter update requirements

 The connection parameter request may be rejected if it does not meet the following guidelines:
 * Peripheral Latency of up to 30 connection intervals.
 * Supervision Timeout from 2 seconds to 6 seconds.
 * Interval Min of at least 15 ms.
 * Interval Min is a multiple of 15 ms.
 * One of the following:
   * Interval Max at least 15 ms greater than Interval Min.
   * Interval Max and Interval Min both set to 15 ms.
   * Interval Max * (Peripheral Latency + 1) of 2 seconds or less.
   * Supervision Timeout greater than Interval Max * (Peripheral Latency + 1) * 3.
*/

/*! configurable parameters for connection parameter update */
static const appUpdateCfg_t cgmUpdateCfg = {
    6000,           /*! Connection idle period in ms before attempting
                      connection parameter update. set to zero to disable */
    (1000 / 1.25), /*! Minimum connection interval in 1.25ms units */
    (1000 / 1.25), /*! Maximum connection interval in 1.25ms units */
    0, /*! Connection latency */
    600, /*! Supervision timeout in 10ms units, 600*10=6000 ms */
    5 /*! Number of update attempts before giving up */
};

/*! ATT configurable parameters (increase MTU) */
static const attCfg_t cgmAttCfg = {
    15, /* ATT server service discovery connection idle timeout in seconds */
#if OTA_INTERNAL
    128, /* desired ATT MTU */
#else
    241, /* desired ATT MTU */
#endif
    ATT_MAX_TRANS_TIMEOUT, /* transcation timeout in seconds */
    4 /* number of queued prepare writes supported by server */
};

/*! local IRK */
static uint8_t localIrk[] = { 0x95, 0xC8, 0xEE, 0x6F, 0xC5, 0x0D, 0xEF, 0x93,
                              0x35, 0x4E, 0x7C, 0x57, 0x08, 0xE2, 0xA3, 0x85 };

/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! advertising data, discoverable mode */
static const uint8_t cgmAdvDataDisc[] = {
  /*! flags */
  2,                                      /*! length */
  DM_ADV_TYPE_FLAGS,                      /*! AD type */
  //DM_FLAG_LE_LIMITED_DISC | DM_FLAG_LE_BREDR_NOT_SUP, /*! flags */
  DM_FLAG_LE_GENERAL_DISC | DM_FLAG_LE_BREDR_NOT_SUP, /*! flags */ // Since adv durations are all 0s.

  /*! manufacturer specific data */
  //3,                                      /*! length */
  //DM_ADV_TYPE_MANUFACTURER,               /*! AD type */
  //UINT16_TO_BYTES(HCI_ID_ANALOG),         /*! company ID */

  /*! service UUID list */
  5,                                      /*! length */
  DM_ADV_TYPE_16_UUID,                    /*! AD type */
  UINT16_TO_BYTES(ATT_UUID_CGM_SERVICE),
  UINT16_TO_BYTES(ATT_UUID_DEVICE_INFO_SERVICE)
  // UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE) // TODO
};

/*! scan data, discoverable mode */
static const uint8_t cgmScanDataDisc[] = {
    /*! device name */
    4, /*! length */
    DM_ADV_TYPE_LOCAL_NAME, /*! AD type */
    'C','G','M'
};

/*! CGM Feature */
static uint8_t cgmFeature[CGMS_FEAT_LEN] = {
    1, 2, 3,    // Feature 3 bytes
    4,          // Type-Sample Location Field 1 byte,
    0, 0        // CRC 2 bytes
};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! application control block */
static struct {
    wsfHandlerId_t handlerId; /* WSF handler ID */
#if (BT_VER > 8)
    uint8_t phyMode; /*! PHY Test Mode */
#endif /* BT_VER */
    appDbHdl_t resListRestoreHdl; /*! Resolving List last restored handle */
    bool_t restoringResList; /*! Restoring resolving list from NVM */
} cgmCb;

/* LESC OOB configuration */
static dmSecLescOobCfg_t *datsOobCfg;

/* Timer for trimming of the 32 kHz crystal */
wsfTimer_t trimTimer;

/* Timer for customer specified app */
wsfTimer_t custSpecAppTimer;

/**************************************************************************************************
  global Variables
**************************************************************************************************/
uint8_t conn_opened = 0; /// 0: connection is not opened
extern appSlaveCb_t appSlaveCb;
extern appDb_t appDb;

extern char *GetDmEvtStr(uint8_t evt);
extern void setAdvTxPower(void);
extern void printTime(void);

/*************************************************************************************************/
/*!
 *  \brief  Send notification containing data.
 *
 *  \param  connId      DM connection ID.
 *  \param  size        Size of message to send.
 *  \param  msg         Message to send
 *  \return None.
 */
/*************************************************************************************************/
static void datsSendData(dmConnId_t connId, uint8_t size, uint8_t *msg)
{
    if (AttsCccEnabled(connId, DATS_WP_DAT_CCC_IDX)) {
        /* send notification */
        AttsHandleValueNtf(connId, WP_DAT_HDL, size, msg);
    }
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
static void cgmDmCback(dmEvt_t *pDmEvt)
{
    dmEvt_t *pMsg;
    uint16_t len;

    if (pDmEvt->hdr.event == DM_SEC_ECC_KEY_IND) {
        DmSecSetEccKey(&pDmEvt->eccMsg.data.key);

        /* If the local device sends OOB data. */
        if (cgmSendOobData) {
            uint8_t oobLocalRandom[SMP_RAND_LEN];
            SecRand(oobLocalRandom, SMP_RAND_LEN);
            DmSecCalcOobReq(oobLocalRandom, pDmEvt->eccMsg.data.key.pubKey_x);
        }
    } else if (pDmEvt->hdr.event == DM_SEC_CALC_OOB_IND) {
        if (datsOobCfg == NULL) {
            datsOobCfg = WsfBufAlloc(sizeof(dmSecLescOobCfg_t));
        }

        if (datsOobCfg) {
            Calc128Cpy(datsOobCfg->localConfirm, pDmEvt->oobCalcInd.confirm);
            Calc128Cpy(datsOobCfg->localRandom, pDmEvt->oobCalcInd.random);
        }
    } else {
        len = DmSizeOfEvt(pDmEvt);

        if ((pMsg = WsfMsgAlloc(len)) != NULL) {
            memcpy(pMsg, pDmEvt, len);
            WsfMsgSend(cgmCb.handlerId, pMsg);
        }
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
static void datsAttCback(attEvt_t *pEvt)
{
    attEvt_t *pMsg;

    if (WdxsAttCback(pEvt)) {
        return;
    }

    if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL) {
        memcpy(pMsg, pEvt, sizeof(attEvt_t));
        pMsg->pValue = (uint8_t *) (pMsg + 1);
        memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
        WsfMsgSend(cgmCb.handlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief   Start the trim procedure for the 32 kHz crystal.
 *  \details 32 kHz crystal will drift over temperature, execute this trim procedure to align with 32 MHz clock.
 *  System will not be able to enter standby mode while this procedure is in progress (200-600 ms).
 *  Larger period values will save power. Do not execute this procedure while BLE hardware is disabled.
 *  Disable this periodic trim if under constant temperature. Refer to 32 kHz crystal data sheet for temperature variance.
 *
 *  \return  None.
 */
/*************************************************************************************************/
static void trimStart(void)
{
    int err;
    extern void wutTrimCb(int err);

    /* Start the 32 kHz crystal trim procedure */
    err = MXC_WUT_TrimCrystalAsync(MXC_WUT0, wutTrimCb);
    if (err != E_NO_ERROR) {
        APP_TRACE_INFO1("Error starting 32kHz crystal trim %d", err);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  ATTS write callback for proprietary data service.
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t cgmWpWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                         uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
    if (len == sizeof(fileHeader_t)) {
        uint16_t version = WdxsFileGetFirmwareVersion();

        fileHeader_t *tmpHeader;
        tmpHeader = (fileHeader_t *)pValue;
        initHeader(tmpHeader);

        datsSendData(connId, sizeof(version), (uint8_t *)&version);
    }

    return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
*
*  \brief  Add device to resolving list.
*
*  \param  dbHdl   Device DB record handle.
*
*  \return None.
*/
/*************************************************************************************************/
static void cgmPrivAddDevToResList(appDbHdl_t dbHdl)
{
    dmSecKey_t *pPeerKey;

    /* if peer IRK present */
    if ((pPeerKey = AppDbGetKey(dbHdl, DM_KEY_IRK, NULL)) != NULL) {
        /* set advertising peer address */
        AppSetAdvPeerAddr(pPeerKey->irk.addrType, pPeerKey->irk.bdAddr);
    }
}

/*************************************************************************************************/
/*!
*
*  \brief  Handle remove device from resolving list indication.
*
*  \param  pMsg    Pointer to DM callback event message.
*
*  \return None.
*/
/*************************************************************************************************/
static void cgmPrivRemDevFromResListInd(dmEvt_t *pMsg)
{
    if (pMsg->hdr.status == HCI_SUCCESS) {
        if (AppDbGetHdl((dmConnId_t)pMsg->hdr.param) != APP_DB_HDL_NONE) {
            uint8_t addrZeros[BDA_ADDR_LEN] = { 0 };

            /* clear advertising peer address and its type */
            AppSetAdvPeerAddr(HCI_ADDR_TYPE_PUBLIC, addrZeros);
        }
    }
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Display stack version.
 *
 *  \param  version    version number.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmDisplayStackVersion(const char *pVersion)
{
    APP_TRACE_INFO1("Stack Version: %s", pVersion);
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
static void cgmSetup(dmEvt_t *pMsg)
{
    /* Initialize control information */
    cgmCb.restoringResList = FALSE;

    /* set advertising and scan response data for discoverable mode */
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE,  sizeof(cgmAdvDataDisc),   (uint8_t *)cgmAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(cgmScanDataDisc), (uint8_t *)cgmScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE,  sizeof(cgmAdvDataDisc),  (uint8_t *)cgmAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(cgmScanDataDisc), (uint8_t *)cgmScanDataDisc);

    /* start advertising; automatically set connectable/discoverable mode and bondable mode */
    AppAdvStart(APP_MODE_AUTO_INIT);
}

/*************************************************************************************************/
/*!
 *  \brief  Begin restoring the resolving list.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmRestoreResolvingList(dmEvt_t *pMsg)
{
    /* Restore first device to resolving list in Controller. */
    cgmCb.resListRestoreHdl = AppAddNextDevToResList(APP_DB_HDL_NONE);

    if (cgmCb.resListRestoreHdl == APP_DB_HDL_NONE) {
        /* No device to restore.  Setup application. */
        cgmSetup(pMsg);
    } else {
        cgmCb.restoringResList = TRUE;
    }
}

/*************************************************************************************************/
/*!
*  \brief  Handle add device to resolving list indication.
 *
 *  \param  pMsg    Pointer to DM callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmPrivAddDevToResListInd(dmEvt_t *pMsg)
{
    /* Check if in the process of restoring the Device List from NV */
    if (cgmCb.restoringResList) {
        /* Set the advertising peer address. */
        cgmPrivAddDevToResList(cgmCb.resListRestoreHdl);

        /* Retore next device to resolving list in Controller. */
        cgmCb.resListRestoreHdl = AppAddNextDevToResList(cgmCb.resListRestoreHdl);

        if (cgmCb.resListRestoreHdl == APP_DB_HDL_NONE) {
            /* No additional device to restore. Setup application. */
            cgmSetup(pMsg);
        }
    } else {
        cgmPrivAddDevToResList(AppDbGetHdl((dmConnId_t)pMsg->hdr.param));
    }
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
static void cgmProcCccState(cgmMsg_t *pMsg)
{
    APP_TRACE_INFO3("ccc state ind value:%d handle:%d idx:%d", pMsg->ccc.value, pMsg->ccc.handle,
                    pMsg->ccc.idx);

    /* handle CGM measurement CCC */
    if (pMsg->ccc.idx == CGM_MEAS_CCC_IDX) {
        if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY) {
            CgmpsMeasStart((dmConnId_t)pMsg->ccc.hdr.param, CGM_MEAS_TMR_EVT, CGM_MEAS_CCC_IDX, CGM_MEAS_PERIOD_MS, cgmCb.handlerId);
        } else {
            CgmpsMeasStop((dmConnId_t)pMsg->ccc.hdr.param);
        }
    }

    // TODO: other changes
}

/*************************************************************************************************/
/*!
 *  \brief  Process messages from the event handler.
 *          Refer to Libraries/Cordio/ble-apps/sources/gluc/gluc_main.c/glucProcMsg()
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmProcMsg(dmEvt_t *pMsg)
{
    uint8_t uiEvent = APP_UI_NONE;

    switch (pMsg->hdr.event) {
    case ATTS_HANDLE_VALUE_CNF:
        CgmpsProcMsg(&pMsg->hdr);
        break;

    case ATTS_CCC_STATE_IND:
        cgmProcCccState((cgmMsg_t *)pMsg);
        break;

    case DM_RESET_CMPL_IND:
        AttsCalculateDbHash();
        DmSecGenerateEccKeyReq();
        AppDbNvmReadAll();
        cgmRestoreResolvingList(pMsg);
        setAdvTxPower();

        cgmSetup(pMsg);

        uiEvent = APP_UI_RESET_CMPL;
        break;

    case DM_ADV_START_IND:
        WsfTimerStartMs(&trimTimer, TRIM_TIMER_PERIOD_MS);

        uiEvent = APP_UI_ADV_START;
        break;

    case DM_ADV_STOP_IND:
        WsfTimerStop(&trimTimer);

        uiEvent = APP_UI_ADV_STOP;
        break;

    case DM_CONN_OPEN_IND:
        //@?@ conn_opened = 1;
        
        CgmpsProcMsg(&pMsg->hdr);

        uiEvent = APP_UI_CONN_OPEN;
        break;

    case DM_CONN_CLOSE_IND:
        WsfTimerStop(&trimTimer);
        
        APP_TRACE_INFO2("Connection closed status 0x%x, reason 0x%x", pMsg->connClose.status,
                        pMsg->connClose.reason);

        switch (pMsg->connClose.reason) {
            case HCI_ERR_CONN_TIMEOUT:
                APP_TRACE_INFO0(" TIMEOUT");
            break;
            
            case HCI_ERR_LOCAL_TERMINATED:
                APP_TRACE_INFO0(" LOCAL TERM");
            break;
        
            case HCI_ERR_REMOTE_TERMINATED:
                APP_TRACE_INFO0(" REMOTE TERM");
            break;

            case HCI_ERR_CONN_FAIL:
                APP_TRACE_INFO0(" FAIL ESTABLISH");
            break;

            case HCI_ERR_MIC_FAILURE:
                APP_TRACE_INFO0(" MIC FAILURE");
            break;
        }

        CgmpsProcMsg(&pMsg->hdr);
        conn_opened = 0;

        APP_TRACE_INFO1("dbg ndx=%d", u32DbgBufNdx);
        if ( u32DbgBufNdx > 0) print_dbg_buf();

        uiEvent = APP_UI_CONN_CLOSE;

        break;

    case DM_SEC_PAIR_CMPL_IND:
        DmSecGenerateEccKeyReq();
        AppDbNvmStoreBond(AppDbGetHdl((dmConnId_t)pMsg->hdr.param));
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
        if (pMsg->authReq.oob) {
            dmConnId_t connId = (dmConnId_t)pMsg->hdr.param;

            /* TODO: Perform OOB Exchange with the peer. */

            /* TODO: Fill datsOobCfg peerConfirm and peerRandom with value passed out of band */

            if (datsOobCfg != NULL) {
                DmSecSetOob(connId, datsOobCfg);
            }

            DmSecAuthRsp(connId, 0, NULL);
        } else {
            AppHandlePasskey(&pMsg->authReq);
        }
        break;

    case DM_SEC_COMPARE_IND:
        AppHandleNumericComparison(&pMsg->cnfInd);
        break;

    case DM_PRIV_ADD_DEV_TO_RES_LIST_IND:
        cgmPrivAddDevToResListInd(pMsg);
        break;

    case DM_PRIV_REM_DEV_FROM_RES_LIST_IND:
        cgmPrivRemDevFromResListInd(pMsg);
        break;

    case DM_ADV_NEW_ADDR_IND:
        break;

    case DM_PRIV_CLEAR_RES_LIST_IND:
        APP_TRACE_INFO1("Clear resolving list status 0x%02x", pMsg->hdr.status);
        break;

    case DM_PHY_UPDATE_IND:
        
        APP_TRACE_INFO3("RX=%d TX=%d dbg=%d", pMsg->phyUpdate.rxPhy, pMsg->phyUpdate.txPhy, u8DbgSt);
        break;

    case TRIM_TIMER_EVT:
        trimStart();
        WsfTimerStartMs(&trimTimer, TRIM_TIMER_PERIOD_MS);
        break;

    case CUST_SPEC_TMR_EVT:
        LED_Off(LED_RED);

        WsfTimerStartMs(&custSpecAppTimer, CUST_SPEC_TMR_PERIOD_MS); // start next

        if (conn_opened == 0) {
            APP_TRACE_INFO0("TODO: CUSTOMER SPECIFIED APP");
            printTime();

            MXC_Delay(1000); // workload
        }

        LED_On(LED_RED);
        break;

    case CGM_MEAS_TMR_EVT:
        cgmpsMeasTimerExp((wsfMsgHdr_t *)pMsg);
        break;

    default:
        break;
    }

    if (uiEvent != APP_UI_NONE) {
        AppUiAction(uiEvent);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Application handler init function called during system initialization.
 *          Refer to Libraries/Cordio/ble-apps/sources/gluc/gluc_main.c/GlucHandlerInit()
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmHandlerInit(wsfHandlerId_t handlerId)
{
    APP_TRACE_INFO0("CgmHandlerInit");

    /* store handler ID */
    cgmCb.handlerId = handlerId;

    /* Set configuration pointers */
    pAppSlaveCfg = (appSlaveCfg_t *)&cgmSlaveCfg;
    pAppAdvCfg = (appAdvCfg_t *)&cgmAdvCfg;
    pAppSecCfg = (appSecCfg_t *)&cgmSecCfg;
    pAppUpdateCfg = (appUpdateCfg_t *)&cgmUpdateCfg;
    pSmpCfg = (smpCfg_t *)&cgmSmpCfg;
    pAttCfg = (attCfg_t *)&cgmAttCfg;

    /* Initialize application framework */
    AppSlaveInit();
    AppServerInit();

    /* Set IRK for the local device */
    DmSecSetLocalIrk(localIrk);

    /* Setup 32 kHz crystal trim timer */
    trimTimer.handlerId = handlerId;
    trimTimer.msg.event = TRIM_TIMER_EVT;

    /* Set customer specified application timer */
    custSpecAppTimer.handlerId = handlerId;
    custSpecAppTimer.msg.event = CUST_SPEC_TMR_EVT;

    APP_TRACE_INFO0("start customer specified app timer");
    MXC_Delay(1000);
    //WsfTimerStartMs(&custSpecAppTimer, CUST_SPEC_TMR_PERIOD_MS); // first start

    /* initialize CGM profile sensor */
    CgmpsInit();
    CgmpsSetCccIdx(CGM_MEAS_CCC_IDX, 0, CGM_RACP_CCC_IDX);
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
static void cgmBtnCback(uint8_t btn)
{
#if (BT_VER > 8)
    dmConnId_t connId;
    if ((connId = AppConnIsOpen()) != DM_CONN_ID_NONE)
#else
    if (AppConnIsOpen() != DM_CONN_ID_NONE)
#endif /* BT_VER */
    {
        switch (btn) {
#if (BT_VER > 8)
        case APP_UI_BTN_2_SHORT:

        {
            static uint32_t coded_phy_cnt = 0;
            /* Toggle PHY Test Mode */
            coded_phy_cnt++;
            switch (coded_phy_cnt & 0x3) {
            case 0:
                /* 1M PHY */
                APP_TRACE_INFO0("1 MBit TX and RX PHY Requested");
                DmSetPhy(connId, HCI_ALL_PHY_ALL_PREFERENCES, HCI_PHY_LE_1M_BIT, HCI_PHY_LE_1M_BIT,
                         HCI_PHY_OPTIONS_NONE);
                break;
            case 1:
                /* 2M PHY */
                APP_TRACE_INFO0("2 MBit TX and RX PHY Requested");
                DmSetPhy(connId, HCI_ALL_PHY_ALL_PREFERENCES, HCI_PHY_LE_2M_BIT, HCI_PHY_LE_2M_BIT,
                         HCI_PHY_OPTIONS_NONE);
                break;
            case 2:
                /* Coded S2 PHY */
                APP_TRACE_INFO0("LE Coded S2 TX and RX PHY Requested");
                DmSetPhy(connId, HCI_ALL_PHY_ALL_PREFERENCES, HCI_PHY_LE_CODED_BIT,
                         HCI_PHY_LE_CODED_BIT, HCI_PHY_OPTIONS_S2_PREFERRED);
                break;
            case 3:
                /* Coded S8 PHY */
                APP_TRACE_INFO0("LE Coded S8 TX and RX PHY Requested");
                DmSetPhy(connId, HCI_ALL_PHY_ALL_PREFERENCES, HCI_PHY_LE_CODED_BIT,
                         HCI_PHY_LE_CODED_BIT, HCI_PHY_OPTIONS_S8_PREFERRED);
                break;
            }
            break;
        }

#endif /* BT_VER */
        case APP_UI_BTN_2_MED: {
            uint16_t version = WdxsFileGetFirmwareVersion();
            (void)version;
            APP_TRACE_INFO2("FW_VERSION: %d.%d", ((version & 0xFF00) >> 8), version & 0xFF);
            break;
        }

        default:
            APP_TRACE_INFO0(" - No action assigned");
            break;
        }
    } else {
        switch (btn) {
        case APP_UI_BTN_1_SHORT:
            /* start advertising */
            AppAdvStart(APP_MODE_AUTO_INIT);
            break;

        case APP_UI_BTN_1_MED:
            /* Enter bondable mode */
            AppSetBondable(TRUE);
            break;

        case APP_UI_BTN_1_LONG:
            /* clear all bonding info */
            AppSlaveClearAllBondingInfo();
            AppDbNvmDeleteAll();
            break;

        case APP_UI_BTN_1_EX_LONG: {
            const char *pVersion;
            StackGetVersionNumber(&pVersion);
            cgmDisplayStackVersion(pVersion);
        } break;

        case APP_UI_BTN_2_SHORT:
            /* stop advertising */
            AppAdvStop();
            break;
        case APP_UI_BTN_2_MED: {
            uint16_t version = WdxsFileGetFirmwareVersion();
            (void)version;
            APP_TRACE_INFO2("FW_VERSION: %d.%d", ((version & 0xFF00) >> 8), version & 0xFF);
            break;
        }
        default:
            APP_TRACE_INFO0(" - No action assigned");
            break;
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Callback for WSF buffer diagnostic messages.
 *
 *  \param  pInfo     Diagnostics message
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmWsfBufDiagnostics(WsfBufDiag_t *pInfo)
{
    if (pInfo->type == WSF_BUF_ALLOC_FAILED) {
        APP_TRACE_INFO2("CGM got WSF Buffer Allocation Failure - Task: %d Len: %d",
                        pInfo->param.alloc.taskId, pInfo->param.alloc.len);
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
 *  \brief  WSF event handler for application. 
 *          Refer to Libraries/Cordio/ble-apps/sources/gluc/gluc_main.c/GlucHandler()
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    if (pMsg != NULL) {
        APP_TRACE_INFO2("\nCGM got evt %d (%s)", pMsg->event, GetDmEvtStr(pMsg->event));

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

            /* process WDXS-related messages */
            WdxsProcDmMsg((dmEvt_t *)pMsg);
        }

        /* perform profile and user interface-related operations */
        cgmProcMsg((dmEvt_t *)pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Resets the system.
 *
 *  \return None.
 */
/*************************************************************************************************/
void WdxsResetSystem(void)
{
    APP_TRACE_INFO0("Reseting!");
    /* Wait for the console to finish printing */
    volatile int i;
    for (i = 0; i < 0xFFFFF; i++) {}
    NVIC_SystemReset();
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
static void cgmCccCback(attsCccEvt_t *pEvt)
{
    attsCccEvt_t *pMsg;
    appDbHdl_t dbHdl = AppDbGetHdl((dmConnId_t) pEvt->hdr.param);
    int isBonded = AppCheckBonded((dmConnId_t)pEvt->hdr.param);

    APP_TRACE_INFO4("cgmCccCback, hdl %d, dbHdl %d, idx %d, val %d", pEvt->handle, dbHdl, pEvt->idx, pEvt->value);

    /* If CCC not set from initialization and there's a device record and currently bonded */
    if ((pEvt->handle != ATT_HANDLE_NONE) &&
        (dbHdl != APP_DB_HDL_NONE) && isBonded)
    {
        APP_TRACE_INFO0("Store value in device database.");
        AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
        AppDbNvmStoreCccTbl(dbHdl); // work with iKeyDist = 0
    }
    else
    {
        APP_TRACE_INFO2("Do nothing. bonded %d, bondable %d", isBonded, appSlaveCb.bondable);
    }
    
    // in order to trigger characteristic notification
    if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL) {
        memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
        WsfMsgSend(cgmCb.handlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *          Refer to Libraries/Cordio/ble-apps/sources/gluc/gluc_main.c/GlucStart()
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmStart(void)
{
    /* Register for stack callbacks */
    DmRegister(cgmDmCback);
    DmConnRegister(DM_CLIENT_ID_APP, cgmDmCback);

    AttRegister(datsAttCback);
    AttConnRegister(AppServerConnCback);
    AttsCccRegister(CGM_CCC_IDX_MAX, (attsCccSet_t *)cgmCccSet, cgmCccCback);

    /* Initialize attribute server database */
    SvcCoreGattCbackRegister(GattReadCback, GattWriteCback);
    SvcCoreAddGroup();
    SvcWpCbackRegister(NULL, cgmWpWriteCback);
    SvcWpAddGroup();

    SvcCgmsAddGroup();
    SvcCgmsCbackRegister(NULL, CgmpsRacpWriteCback);

    SvcDisAddGroup();

    /* Set Service Changed CCCD index. */
    GattSetSvcChangedIdx(GATT_SC_CCC_IDX);

    /* Set supported features after starting database */
    CgmpsSetFeature(cgmFeature);

    /* Register for app framework button callbacks */
    AppUiBtnRegister(cgmBtnCback);

    /* Initialize the WDXS File */
    WdxsFileInit();

    /* Set the WDXS CCC Identifiers */
    WdxsSetCccIdx(WDXS_DC_CH_CCC_IDX, WDXS_AU_CH_CCC_IDX, WDXS_FTC_CH_CCC_IDX, WDXS_FTD_CH_CCC_IDX);

#if (BT_VER > 8)
    WdxsPhyInit();
#endif /* BT_VER */

#if (BT_VER > 8)
    DmPhyInit();
#endif /* BT_VER */

    WsfNvmInit();

    WsfBufDiagRegister(cgmWsfBufDiagnostics);

    /* Initialize with button press handler */
    PalBtnInit(btnPressHandler);

    /* Reset the device */
    DmDevReset();
}
