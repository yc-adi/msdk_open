/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  CGM profile sensor.
 *
 *  Copyright (c) 2012-2018 Arm Ltd. All Rights Reserved.
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
#ifndef CGMPS_API_H
#define CGMPS_API_H

#include "cgmps_api.h"
#include "cgmps_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup CGM_PROFILE
 *  \{ */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief All supported features of the glucose profile */
#define GLP_ALL_SUPPORTED_FEATURES      0x000F

/*! \brief Connection control block */
typedef struct
{
  dmConnId_t    connId;               /*! \brief Connection ID */
  bool_t        cgmMeasToSend;        /*! \brief CGM measurement ready to be sent on this channel */
} cgmpsConn_t;

/* Control block */
typedef struct
{
  uint8_t       operand[CGMPS_OPERAND_MAX]; /* Stored operand filter data */
  cgmpsRec_t    *pCurrRec;                  /* Pointer to current measurement record */
  cgmpsConn_t   conn[DM_CONN_MAX];          /* connection control blcok */
  wsfTimer_t    measTimer;                  /* periodic measurement timer */
  wsfTimerTicks_t periodMs;                 /* Measurement timer expiration period in ms */
  bool_t        inProgress;                 /* TRUE if RACP procedure in progress */
  bool_t        txReady;                    /* TRUE if ready to send next notification or indication */
  bool_t        aborting;                   /* TRUE if abort procedure in progress */
  uint8_t       cgmMeasCccIdx;              /* CGM measurement CCCD index */
  uint8_t       racpCccIdx;                 /* Record access control point CCCD index */
  uint8_t       oper;                       /* Stored operator */
} cgmpsCb_t;


typedef struct
{
  uint8_t numRec;
} cgmpsDbCb_t;

void CgmpsMeasStart(dmConnId_t connId, uint8_t timerEvt, uint8_t cgmMeasCccIdx, uint32_t timerPeriodMs, uint8_t hndlrId);
void CgmpsMeasStop(dmConnId_t connId);
void cgmpsMeasTimerExp(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief  Initialize the CGM profile sensor.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmpsInit(void);

/*************************************************************************************************/
/*!
 *  \brief  This function is called by the application when a message that requires
 *          processing by the glucose profile sensor is received.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmpsProcMsg(wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
*  \brief  Handle a button press.
*
*  \param  connId    Connection identifier.
*  \param  btn       Button press.
*
*  \return None.
*/
/*************************************************************************************************/
void CgmpsBtn(dmConnId_t connId, uint8_t btn);

/*************************************************************************************************/
/*!
 *  \brief  ATTS write callback for glucose service record access control point.  Use this
 *          function as a parameter to SvcGlsCbackRegister().
 *
 *  \param  connId       DM connection identifier.
 *  \param  handle       ATT handle.
 *  \param  operation    ATT operation.
 *  \param  offset       Write offset.
 *  \param  len          Write length.
 *  \param  pValue       Value to write.
 *  \param  pAttr        Attribute to write.
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t CgmpsRacpWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                           uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr);

/*************************************************************************************************/
/*!
 *  \brief  Set the supported features of the glucose sensor.
 *
 *  \param  feature   Feature bitmask.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmpsSetFeature(uint8_t *feature);

/*************************************************************************************************/
/*!
 *  \brief  Set the CCCD index used by the application for CGM service characteristics.
 *
 *  \param  cgmCccIdx   CGM measurement CCCD index.
 *  \param  cgmStatusCccIdx CGM status CCCD index
 *  \param  racpCccIdx  Record access control point CCCD index.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmpsSetCccIdx(uint8_t cgmCccIdx, uint8_t cgmStatusCccIdx, uint8_t racpCccIdx);

/*! \} */    /* GLUCOSE_PROFILE */

#ifdef __cplusplus
};
#endif

#endif /* CGMPS_API_H */
