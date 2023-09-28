/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example CGM service implementation.
 *
 *  Copyright (c) 2012-2019 Arm Ltd. All Rights Reserved.
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

#include "wsf_types.h"
#include "att_api.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "svc_ch.h"
#include "svc_cgms.h"
#include "svc_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef CGMS_SEC_PERMIT_READ
#define CGMS_SEC_PERMIT_READ (ATTS_PERMIT_READ | ATTS_PERMIT_READ_ENC)
#endif

/*! Characteristic write permissions */
#ifndef CGMS_SEC_PERMIT_WRITE
#define CGMS_SEC_PERMIT_WRITE (ATTS_PERMIT_WRITE | ATTS_PERMIT_WRITE_ENC)
#define CGMS_SEC_PERMIT_WRITE_AUTH (ATTS_PERMIT_WRITE | ATTS_PERMIT_WRITE_ENC | ATTS_PERMIT_WRITE_AUTH)
#endif

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* CGM service declaration */
static const uint8_t cgmsSvc[] = {UINT16_TO_BYTES(ATT_UUID_CGM_SERVICE)};
static const uint16_t cgmsSvcLen = sizeof(cgmsSvc);
static const uint8_t cgmsSvcSetting = 0;
static const uint8_t cgmsSvcPerm = ATTS_PERMIT_READ;

/* CGM measurement characteristic */
static const uint8_t cgmsMeasCh[] = {ATT_PROP_NOTIFY, UINT16_TO_BYTES(CGMS_MEAS_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_MEAS)};
static const uint16_t cgmsMeasChLen = sizeof(cgmsMeasCh);
static const uint8_t cgmsMeasChSetting = 0;
static const uint8_t cgmsMeasChPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/* CGM measurement dummy values */
static const uint8_t cgmsMeasVal[] = {0};
static const uint16_t cgmsMeasValLen = sizeof(cgmsMeasVal);

/* CGM measurement cccd */
static uint8_t cgmsMeasCccd[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t cgmsMeasCccdLen = sizeof(cgmsMeasCccd);
static const uint8_t cgmsMeasCccdSetting = ATTS_SET_CCC;
static const uint8_t cgmsMeasCccdPerm = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE | ATTS_PERMIT_WRITE_AUTH;

/* CGM feature characteristic */
static const uint8_t cgmsFeatCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(CGMS_FEAT_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_FEATURE)};
static const uint16_t cgmsFeatChLen = sizeof(cgmsFeatCh);
static const uint8_t cgmsFeatChPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/* CGM feature value */
static uint8_t cgmsFeatVal[CGMS_FEAT_LEN] = {0, 0, 0, 0, 0, 0};
static const uint16_t cgmFeatValLen = sizeof(cgmsFeatVal);
static const uint8_t cgmsFeatValPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/* CGM status characteristic */
static const uint8_t cgmsStCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(CGMS_ST_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_STATUS)};
static const uint16_t cgmsStChLen = sizeof(cgmsStCh);
static const uint8_t cgmsStChPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/* CGM status value */
static uint8_t cgmsStVal[CGMS_ST_LEN] = {1, 2, 3};
static const uint16_t cgmsStValLen = sizeof(cgmsStVal);
static const uint8_t cgmsStValPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/* CGM session start time characteristic */
static const uint8_t cgmsSessStartTCh[] = {ATT_PROP_READ | ATT_PROP_WRITE, UINT16_TO_BYTES(CGMS_SESS_START_T_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_SESS_START_T)};
static const uint16_t cgmsSessStartTChLen = sizeof(cgmsSessStartTCh);
static const uint8_t cgmsSessStartTChPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/** 
 * @brief CGM session start time value
 * The Session Start Time Field defines the time of the initial CGM measurement. The absolute time of the 
 * first CGM measurement taken is not known, so the Server stores each CGM measurement with a relative 
 * time stamp (Time Offset), starting with 0 for the first measurement (Session Start).
 * Upon initial connection, if the device supports an automatic start of the CGM session (e.g., at power on), 
 * or after the Start Session procedure, the Client shall write its current time to this characteristic and the 
 * Server shall calculate and store the Session Start Time using the time of the client and its own current 
 * relative time value.
 * Byte 0: Time Zone field
 * Byte 1: DST Offset field for daylight saving setting
 * Byte 2: days
 * Byte 3: hour
 * Byte 4: minutes
 * Byte 5: seconds
 * BYte 6-7: CRC
*/
static uint8_t cgmsSessStartTVal[CGMS_SESS_START_T_LEN] = {0x80, 255, 0, 1, 2, 3, 0, 0};
static const uint16_t cgmsSessStartTValLen = sizeof(cgmsSessStartTVal);
static const uint8_t cgmsSessStartTValPerm = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE | ATTS_PERMIT_READ_AUTH | ATTS_PERMIT_WRITE_AUTH;

/* CGM session run time characteristic */
static const uint8_t cgmsSessRunTCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(CGMS_SESS_RUN_T_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_SESS_RUN_T)};
static const uint16_t cgmsSessRunTChLen = sizeof(cgmsSessRunTCh);
static const uint8_t cgmsSessRunTChPerm = ATTS_PERMIT_READ;
/**
 * @brief CGM session run time characteristic value
 * Sensors have a limited run time which they are approved for. However, this characteristic 
 * should enable a prediction of the run time depending on physiological effects in future devices.
 * The CGM Session Run Time is a relative time, based on the CGM Session Start Time. Here the offset
 * is a uint32_t count in seconds based on the session start time.
 */
static uint8_t cgmsSessRunTVal[CGMS_SESS_RUN_T_LEN] = {UINT32_TO_BYTES(0x11223344)};
static const uint16_t cgmsSessRunTValLen = sizeof(cgmsSessRunTVal);
static const uint8_t cgmsSessRunTValPerm = ATTS_PERMIT_READ | ATTS_PERMIT_WRITE | ATTS_PERMIT_READ_AUTH | ATTS_PERMIT_WRITE_AUTH;

/* Control point characteristic */
static const uint8_t cgmsRacpCh[] = {(ATT_PROP_INDICATE | ATT_PROP_WRITE), UINT16_TO_BYTES(CGMS_RACP_HDL), UINT16_TO_BYTES(ATT_UUID_RACP)};
static const uint16_t cgmsRacpChLen = sizeof(cgmsRacpCh);
static const uint8_t cgmsRacpChPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;

/* Record access control point */
/* Note these are dummy values */
static const uint8_t cgmsRacpVal[] = {0};
static const uint16_t cgmsRacpValLen = sizeof(cgmsRacpVal);
static const uint8_t cgmsRacpValPerm = ATTS_PERMIT_WRITE | ATTS_PERMIT_WRITE_ENC | ATTS_PERMIT_WRITE_AUTH;

/* Record access control point client characteristic configuration */
static uint8_t cgmsRacpChCccd[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t cgmsRacpChCccdLen = sizeof(cgmsRacpChCccd);
static const uint8_t cgmsRacpChCccdPerm = ATTS_PERMIT_WRITE | ATTS_PERMIT_WRITE_ENC | ATTS_PERMIT_WRITE_AUTH;

/* CGM SOPS characteristic */
//static uint8_t cgmsSops[] = {0};
//static const uint8_t cgmsSopsLen = sizeof(cgmsSops);
//static const uint8_t cgmsSopsPerm = ATTS_PERMIT_READ | ATTS_PERMIT_READ_AUTH;;

/* CGM SOPS value */
/* Note these are dummy values */
//static const uint8_t cgmsSopsVal[] = {0};
//static const uint16_t cgmsSopsValLen = sizeof(cgmsSopsVal);
//static const uint8_t cgmsSopsValPerm = CGMS_SEC_PERMIT_WRITE_AUTH;

/* CGM SOPS CCCD */
//static uint8_t cgmsSopsCccd[] = {UINT16_TO_BYTES(0x0000)};
//static const uint16_t cgmsSopsCccdLen = sizeof(cgmsSopsCccd);
//static const uint8_t cgmsSopsCccdPerm = ATTS_PERMIT_WRITE | ATTS_PERMIT_WRITE_ENC | ATTS_PERMIT_WRITE_AUTH;

/* Attribute list for CGM group, must match cgms_hdl */
static const attsAttr_t cgmsList[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid, // 0x2800, primary service. CGMS_v1.0.1, 2 Service Declaration
    (uint8_t *) cgmsSvc, // 0x181F
    (uint16_t *) &cgmsSvcLen,
    sizeof(cgmsSvc),
    cgmsSvcSetting,
    cgmsSvcPerm
  },

  // <--- CGM measurement ATT_UUID_CGM_MEAS
  /* CGM measurement characteristic declaration */
  {
    attChUuid, // 0x2803, NOTE: in attsIsHashableAttr() the next item will be ignored
    (uint8_t *) cgmsMeasCh,
    (uint16_t *) &cgmsMeasChLen,
    sizeof(cgmsMeasCh),
    cgmsMeasChSetting,
    cgmsMeasChPerm
  },
  /* Characteristic value */
  {
    attCgmmChUuid, // 0x2AA7
    (uint8_t *) cgmsMeasVal,
    (uint16_t *) &cgmsMeasValLen,
    sizeof(cgmsMeasVal),
    0,
    0
  },
  /* Client characteristic configuration descriptor */
  {
    attCliChCfgUuid, // 0x2902
    (uint8_t *) cgmsMeasCccd,
    (uint16_t *) &cgmsMeasCccdLen,
    sizeof(cgmsMeasCccd),
    cgmsMeasCccdSetting,
    cgmsMeasCccdPerm
  },
  // --->

  // <--- CGM feature ATT_UUID_CGM_FEATURE
  // characteristic declaration
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsFeatCh,
    (uint16_t *) &cgmsFeatChLen,
    sizeof(cgmsFeatCh),
    0,
    cgmsFeatChPerm
  },
  // characteristic value
  {
    attCgmfChUuid,
    cgmsFeatVal,
    (uint16_t *) &cgmFeatValLen,
    sizeof(cgmsFeatVal),
    0,
    cgmsFeatValPerm
  },
  // --->

  // <--- CGM status ATT_UUID_CGM_STATUS
  // characteristic declaration
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsStCh,
    (uint16_t *) &cgmsStChLen,
    sizeof(cgmsStCh),
    0,
    cgmsStChPerm
  },
  // characteristic value
  {
    attCgmfChUuid,
    cgmsStVal,
    (uint16_t *) &cgmsStValLen,
    sizeof(cgmsStVal),
    0,
    cgmsStValPerm
  },
  // --->

  // <--- CGM session start time ATT_UUID_CGM_SESS_START_T
  // characteristic declaration
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsSessStartTCh,
    (uint16_t *) &cgmsSessStartTChLen,
    sizeof(cgmsSessStartTCh),
    0,
    cgmsSessStartTChPerm
  },
  // characteristic value
  {
    attCgmSessStartTChUuid,
    cgmsSessStartTVal,
    (uint16_t *) &cgmsSessStartTValLen,
    sizeof(cgmsSessStartTVal),
    0,
    cgmsSessStartTValPerm
  },
  // --->

  // <--- CGM session run time ATT_UUID_CGM_SESS_RUN_T
  // characteristic declaration
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsSessRunTCh,
    (uint16_t *) &cgmsSessRunTChLen,
    sizeof(cgmsSessRunTCh),
    0,
    cgmsSessRunTChPerm
  },
  // characteristic value
  {
    attCgmSessRunTChUuid,
    cgmsSessRunTVal,
    (uint16_t *) &cgmsSessRunTValLen,
    cgmsSessRunTValLen,
    0,
    cgmsSessRunTValPerm
  },
  // --->

  /* Record access control point characteristic delclaration */
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsRacpCh,
    (uint16_t *) &cgmsRacpChLen,
    sizeof(cgmsRacpCh),
    0,
    cgmsRacpChPerm
  },
  /* Characteristic value */
  {
    attRacpChUuid,
    (uint8_t *) cgmsRacpVal,
    (uint16_t *) &cgmsRacpValLen,
    ATT_DEFAULT_PAYLOAD_LEN,
    (ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    cgmsRacpValPerm
  },
  /* Client characteristic configuration descriptor */
  {
    attCliChCfgUuid,
    (uint8_t *) cgmsRacpChCccd,
    (uint16_t *) &cgmsRacpChCccdLen,
    sizeof(cgmsRacpChCccd),
    ATTS_SET_CCC,
    cgmsRacpChCccdPerm
  },
#if 0
  /* CGM SOPS characteristic delclaration */
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsSops,
    (uint16_t *) &cgmsSopsLen,
    cgmsSopsLen,
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    attCgmSopsChUuid, // 0x2AAC
    (uint8_t *) cgmsSopsVal,
    (uint16_t *) &cgmsSopsValLen,
    ATT_DEFAULT_PAYLOAD_LEN,
    (ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    CGMS_SEC_PERMIT_WRITE_AUTH
  },
  /* CGM SOPS CCCD */
  {
    attCliChCfgUuid,
    (uint8_t *) cgmsSopsCccd,
    (uint16_t *) &cgmsSopsCccdLen,
    cgmsSopsCccdLen,
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  }
#endif
};

/* CGM group structure */
static attsGroup_t svcCgmsGroup =
{
  NULL,
  (attsAttr_t *)cgmsList,
  NULL,
  NULL,
  CGMS_START_HDL, // startHandle
  CGMS_END_HDL // endHandle
};

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCgmsAddGroup(void)
{
  AttsAddGroup(&svcCgmsGroup, "SvcCgms");
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCgmsRemoveGroup(void)
{
  AttsRemoveGroup(CGMS_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCgmsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcCgmsGroup.readCback = readCback;
  svcCgmsGroup.writeCback = writeCback;
}
