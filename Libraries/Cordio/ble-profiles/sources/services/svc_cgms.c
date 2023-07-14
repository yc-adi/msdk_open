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
static const uint8_t cgmsValSvc[] = {UINT16_TO_BYTES(ATT_UUID_CGM_SERVICE)};
static const uint16_t cgmsLenSvc = sizeof(cgmsValSvc);

/* CGM measurement characteristic */
static const uint8_t cgmsMeasChVal[] = {ATT_PROP_NOTIFY, UINT16_TO_BYTES(CGMS_MEAS_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_MEAS)};
static const uint16_t cgmsMeasChLen = sizeof(cgmsMeasChVal);

/* CGM measurement */
/* Note these are dummy values */
static const uint8_t cgmsMeasVal[] = {0};
static const uint16_t cgmsMeasLen = sizeof(cgmsMeasVal);

/* CGM measurement client characteristic configuration */
static uint8_t cgmsValCgmmChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t glsLenGlmChCcc = sizeof(cgmsValCgmmChCcc);

/* Glucose measurement context */
/* Note these are dummy values */
static const uint8_t glsValGlmc[] = {0};
static const uint16_t glsLenGlmc = sizeof(glsValGlmc);

/* Glucose measurement context client characteristic configuration */
static uint8_t glsValGlmcChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t glsLenGlmcChCcc = sizeof(glsValGlmcChCcc);

/* CGM feature characteristic */
static const uint8_t cgmsFeatCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(CGMS_FEAT_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_FEATURE)};
static const uint16_t cgmsFeatChLen = sizeof(cgmsFeatCh);
/* CGM feature characteristic value */
static uint8_t cgmsFeatVal[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t cgmsFeatLen = sizeof(cgmsFeatVal);

/* CGM status characteristic */
static const uint8_t cgmsStCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(CGMS_ST_HDL), UINT16_TO_BYTES(ATT_UUID_CGM_STATUS)};
static const uint16_t cgmsStChLen = sizeof(cgmsStCh);
/* CGM status characteristic value */
static uint8_t cgmsStVal[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t cgmsStValLen = sizeof(cgmsStVal);

/* Control point characteristic */
static const uint8_t glsValRacpCh[] = {(ATT_PROP_INDICATE | ATT_PROP_WRITE), UINT16_TO_BYTES(CGMS_RACP_HDL), UINT16_TO_BYTES(ATT_UUID_RACP)};
static const uint16_t glsLenRacpCh = sizeof(glsValRacpCh);

/* Record access control point */
/* Note these are dummy values */
static const uint8_t glsValRacp[] = {0};
static const uint16_t glsLenRacp = sizeof(glsValRacp);

/* Record access control point client characteristic configuration */
static uint8_t glsValRacpChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t glsLenRacpChCcc = sizeof(glsValRacpChCcc);

/* CGM SOPS characteristic */
static uint8_t cgmsSops[] = {0};
static const uint8_t cgmsSopsLen = sizeof(cgmsSops);

/* CGM SOPS value */
/* Note these are dummy values */
static const uint8_t cgmsSopsVal[] = {0};
static const uint16_t cgmsSopsValLen = sizeof(cgmsSopsVal);

/* CGM SOPS CCCD */
static uint8_t cgmsSopsCccd[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t cgmsSopsCccdLen = sizeof(cgmsSopsCccd);

/* Attribute list for GLS group, must match cgms_hdl */
static const attsAttr_t cgmsList[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid, // 0x2800, primary service. CGMS_v1.0.1, 2 Service Declaration
    (uint8_t *) cgmsValSvc, // 0x181F
    (uint16_t *) &cgmsLenSvc,
    sizeof(cgmsValSvc),
    0, // settings
    ATTS_PERMIT_READ
  },

  /* CGM measurement characteristic declaration */
  {
    attChUuid, // 0x2803, NOTE: in attsIsHashableAttr() the next item will be ignored
    (uint8_t *) cgmsMeasChVal, // notify, hdl, 0x2AA7
    (uint16_t *) &cgmsMeasChLen,
    sizeof(cgmsMeasChVal),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    attCgmmChUuid, // 0x2AA7
    (uint8_t *) cgmsMeasVal,
    (uint16_t *) &cgmsMeasLen,
    cgmsMeasLen,
    0,
    0
  },
  /* Client characteristic configuration descriptor */
  {
    attCliChCfgUuid, // 0x2902
    (uint8_t *) cgmsValCgmmChCcc,
    (uint16_t *) &glsLenGlmChCcc,
    sizeof(cgmsValCgmmChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },

  // <--- CGM feature ATT_UUID_CGM_FEATURE
  // characteristic declaration
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsFeatCh, // READ, hdl, 0x2AA8
    (uint16_t *) &cgmsFeatChLen,
    sizeof(cgmsFeatCh),
    0,
    ATTS_PERMIT_READ
  },
  // characteristic value
  {
    attCgmfChUuid,
    cgmsFeatVal,
    (uint16_t *) &cgmsFeatLen,
    cgmsFeatLen,
    0,
    ATTS_PERMIT_READ
  },
  // --->

  // <--- CGM status ATT_UUID_CGM_STATUS
  // characteristic declaration
  {
    attChUuid, // 0x2803
    (uint8_t *) cgmsStCh, // READ, hdl, 0x2AA9
    (uint16_t *) &cgmsStChLen,
    sizeof(cgmsStChLen),
    0,
    ATTS_PERMIT_READ
  },
  // characteristic value
  {
    attCgmfChUuid,
    cgmsStVal,
    (uint16_t *) &cgmsStValLen,
    cgmsStValLen,
    0,
    ATTS_PERMIT_READ
  },
  // --->

  /* Record access control point characteristic delclaration */
  {
    attChUuid, // 0x2803
    (uint8_t *) glsValRacpCh,
    (uint16_t *) &glsLenRacpCh,
    sizeof(glsValRacpCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    attRacpChUuid,
    (uint8_t *) glsValRacp,
    (uint16_t *) &glsLenRacp,
    ATT_DEFAULT_PAYLOAD_LEN,
    (ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    CGMS_SEC_PERMIT_WRITE_AUTH
  },
  /* Client characteristic configuration descriptor */
  {
    attCliChCfgUuid,
    (uint8_t *) glsValRacpChCcc,
    (uint16_t *) &glsLenRacpChCcc,
    sizeof(glsValRacpChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
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

/* GLS group structure */
static attsGroup_t svcCgmsGroup =
{
  NULL,
  (attsAttr_t *)cgmsList,
  NULL,
  NULL,
  CGMS_START_HDL, // startHandle
  GLS_END_HDL // endHandle
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
  AttsAddGroup(&svcCgmsGroup);
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
