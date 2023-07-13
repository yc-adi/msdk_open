/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Glucose profile sensor.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "att_api.h"
#include "svc_ch.h"
#include "svc_cgms.h"
#include "app_api.h"
#include "app_ui.h"
#include "cgmps_api.h"
#include "cgmps_main.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* Control block */
static struct
{
  uint8_t       operand[GLPS_OPERAND_MAX];  /* Stored operand filter data */
  cgmpsRec_t     *pCurrRec;                  /* Pointer to current measurement record */
  bool_t        inProgress;                 /* TRUE if RACP procedure in progress */
  bool_t        txReady;                    /* TRUE if ready to send next notification or indication */
  bool_t        aborting;                   /* TRUE if abort procedure in progress */
  uint8_t       glmCccIdx;                  /* Glucose measurement CCCD index */
  uint8_t       glmcCccIdx;                 /* Glucose measurement context CCCD index */
  uint8_t       racpCccIdx;                 /* Record access control point CCCD index */
  uint8_t       oper;                       /* Stored operator */
} cgmpsCb;

/*************************************************************************************************/
/*!
 *  \brief  Build a glucose measurement characteristic.
 *
 *  \param  pBuf     Pointer to buffer to hold the built glucose measurement.
 *  \param  pGlm     Glucose measurement values.
 *
 *  \return Length of pBuf in bytes.
 */
/*************************************************************************************************/
static uint8_t cgmpsBuildGlm(uint8_t *pBuf, cgmpsGlm_t *pGlm)
{
  uint8_t   *p = pBuf;
  uint8_t   flags = pGlm->flags;

  /* flags */
  UINT8_TO_BSTREAM(p, flags);

  /* sequence number */
  UINT16_TO_BSTREAM(p, pGlm->seqNum);

  /* base time */
  UINT16_TO_BSTREAM(p, pGlm->baseTime.year);
  UINT8_TO_BSTREAM(p, pGlm->baseTime.month);
  UINT8_TO_BSTREAM(p, pGlm->baseTime.day);
  UINT8_TO_BSTREAM(p, pGlm->baseTime.hour);
  UINT8_TO_BSTREAM(p, pGlm->baseTime.min);
  UINT8_TO_BSTREAM(p, pGlm->baseTime.sec);

  /* time offset */
  if (flags & CH_GLM_FLAG_TIME_OFFSET)
  {
    UINT16_TO_BSTREAM(p, pGlm->timeOffset);
  }

  /* glucose concentration, type, and sample location */
  if (flags & CH_GLM_FLAG_CONC_TYPE_LOC)
  {
    UINT16_TO_BSTREAM(p, pGlm->concentration);
    UINT8_TO_BSTREAM(p, pGlm->typeSampleLoc);
  }

  /* sensor status annunciation */
  if (flags & CH_GLM_FLAG_SENSOR_STATUS)
  {
    UINT16_TO_BSTREAM(p, pGlm->sensorStatus);
  }

  /* return length */
  return (uint8_t) (p - pBuf);
}

/*************************************************************************************************/
/*!
 *  \brief  Build a glucose measurement context characteristic.
 *
 *  \param  pBuf     Pointer to buffer to hold the built glucose measurement context.
 *  \param  pGlmc    Glucose measurement context values.
 *
 *  \return Length of pBuf in bytes.
 */
/*************************************************************************************************/
static uint8_t cgmpsBuildGlmc(uint8_t *pBuf, cgmpsGlmc_t *pGlmc)
{
  uint8_t   *p = pBuf;
  uint8_t   flags = pGlmc->flags;

  /* flags */
  UINT8_TO_BSTREAM(p, flags);

  /* sequence number */
  UINT16_TO_BSTREAM(p, pGlmc->seqNum);

  /* extended flags present */
  if (flags & CH_GLMC_FLAG_EXT)
  {
    UINT8_TO_BSTREAM(p, pGlmc->extFlags);
  }

  /* carbohydrate id and carbohydrate present */
  if (flags & CH_GLMC_FLAG_CARB)
  {
    UINT8_TO_BSTREAM(p, pGlmc->carbId);
    UINT16_TO_BSTREAM(p, pGlmc->carb);
  }

  /* meal present */
  if (flags & CH_GLMC_FLAG_MEAL)
  {
    UINT8_TO_BSTREAM(p, pGlmc->meal);
  }

  /* tester-health present */
  if (flags & CH_GLMC_FLAG_TESTER)
  {
    UINT8_TO_BSTREAM(p, pGlmc->testerHealth);
  }

  /* exercise duration and exercise intensity present */
  if (flags & CH_GLMC_FLAG_EXERCISE)
  {
    UINT16_TO_BSTREAM(p, pGlmc->exerDuration);
    UINT8_TO_BSTREAM(p, pGlmc->exerIntensity);
  }

  /* medication ID and medication present */
  if (flags & CH_GLMC_FLAG_MED)
  {
    UINT8_TO_BSTREAM(p, pGlmc->medicationId);
    UINT16_TO_BSTREAM(p, pGlmc->medication);
  }

  /* hba1c present */
  if (flags & CH_GLMC_FLAG_HBA1C)
  {
    UINT16_TO_BSTREAM(p, pGlmc->hba1c);
  }

  /* return length */
  return (uint8_t) (p - pBuf);
}

/*************************************************************************************************/
/*!
 *  \brief  Send a glucose measurement context notification.
 *
 *  \param  connId      Connection ID.
 *  \param  pRec        Return pointer to glucose record.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmpsSendMeasContext(dmConnId_t connId, cgmpsRec_t *pRec)
{
  uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN];
  uint8_t len;

  /* build glucose measurement context characteristic */
  len = cgmpsBuildGlmc(buf, &cgmpsCb.pCurrRec->context);

  /* send notification */
  AttsHandleValueNtf(connId, GLS_GLMC_HDL, len, buf);
  cgmpsCb.txReady = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Send a glucose measurement notification.
 *
 *  \param  connId      Connection ID.
 *  \param  pRec        Return pointer to glucose record.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmpsSendMeas(dmConnId_t connId, cgmpsRec_t *pRec)
{
  uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN];
  uint8_t len;

  /* build glucose measurement characteristic */
  len = cgmpsBuildGlm(buf, &cgmpsCb.pCurrRec->meas);

  /* send notification */
  AttsHandleValueNtf(connId, CGMS_MEAS_HDL, len, buf);
  cgmpsCb.txReady = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Send a RACP response indication.
 *
 *  \param  connId      Connection ID.
 *  \param  opcode      RACP opcode.
 *  \param  status      RACP status.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmpsRacpSendRsp(dmConnId_t connId, uint8_t opcode, uint8_t status)
{
  uint8_t buf[GLPS_RACP_RSP_LEN];

  /* build response */
  buf[0] = CH_RACP_OPCODE_RSP;
  buf[1] = CH_RACP_OPERATOR_NULL;
  buf[2] = opcode;
  buf[3] = status;

  /* send indication */
  AttsHandleValueInd(connId, GLS_RACP_HDL, GLPS_RACP_RSP_LEN, buf);
  cgmpsCb.txReady = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Send a RACP number of records response indication.
 *
 *  \param  connId      Connection ID.
 *  \param  numRec      Number of records.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmpsRacpSendNumRecRsp(dmConnId_t connId, uint16_t numRec)
{
  uint8_t buf[GLPS_RACP_NUM_REC_RSP_LEN];

  /* build response */
  buf[0] = CH_RACP_OPCODE_NUM_RSP;
  buf[1] = CH_RACP_OPERATOR_NULL;
  buf[2] = UINT16_TO_BYTE0(numRec);
  buf[3] = UINT16_TO_BYTE1(numRec);

  /* send indication */
  AttsHandleValueInd(connId, GLS_RACP_HDL, GLPS_RACP_RSP_LEN, buf);
  cgmpsCb.txReady = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Handle connection open.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsConnOpen(dmEvt_t *pMsg)
{
  /* initialize */
  cgmpsCb.pCurrRec = NULL;
  cgmpsCb.aborting = FALSE;
  cgmpsCb.inProgress = FALSE;
  cgmpsCb.txReady = FALSE;
  cgmpsCb.oper = CH_RACP_OPERATOR_NULL;
}

/*************************************************************************************************/
/*!
 *  \brief  Handle connection close.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsConnClose(dmEvt_t *pMsg)
{
  cgmpsCb.pCurrRec = NULL;
  cgmpsCb.aborting = FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Handle an ATT handle value confirm.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsHandleValueCnf(attEvt_t *pMsg)
{
  dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;
  cgmpsCb.txReady = TRUE;

  /* if aborting finish that up */
  if (cgmpsCb.aborting)
  {
    cgmpsCb.aborting = FALSE;
    cgmpsRacpSendRsp(connId, CH_RACP_OPCODE_ABORT, CH_RACP_RSP_SUCCESS);
  }

  /* if this is for RACP indication */
  if (pMsg->handle == GLS_RACP_HDL)
  {
    /* procedure no longer in progress */
    cgmpsCb.inProgress = FALSE;
  }
  /* if this is for measurement or context notification */
  else if (pMsg->handle == CGMS_MEAS_HDL || pMsg->handle == GLS_GLMC_HDL)
  {
    if (cgmpsCb.pCurrRec != NULL)
    {
      /* if measurement was sent and there is context to send */
      if (pMsg->handle == CGMS_MEAS_HDL && AttsCccEnabled(connId, cgmpsCb.glmcCccIdx) &&
          (cgmpsCb.pCurrRec->meas.flags & CH_GLM_FLAG_CONTEXT_INFO))
      {
        /* send context */
        cgmpsSendMeasContext(connId, cgmpsCb.pCurrRec);
      }
      /* else if there is another record */
      else if (cgmpsDbGetNextRecord(cgmpsCb.oper, cgmpsCb.operand,
                                   cgmpsCb.pCurrRec, &cgmpsCb.pCurrRec) == CH_RACP_RSP_SUCCESS)
      {
        /* send measurement */
        cgmpsSendMeas(connId, cgmpsCb.pCurrRec);
      }
      /* else all records sent; send RACP response */
      else
      {
        cgmpsRacpSendRsp(connId, CH_RACP_OPCODE_REPORT, CH_RACP_RSP_SUCCESS);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Verify validity of operand data.  If valid, store the data.
 *
 *  \param  oper        Operator.
 *  \param  len         Operand length.
 *  \param  pOperand    Operand data.

 *
 *  \return RACP status.
 */
/*************************************************************************************************/
static uint8_t cgmpsRacpOperCheck(uint8_t oper, uint16_t len, uint8_t *pOperand)
{
  uint8_t status = CH_RACP_RSP_SUCCESS;
  uint8_t filterType;
  uint8_t filterLen = 0;

  /* these operators have no operands */
  if (oper == CH_RACP_OPERATOR_ALL || oper == CH_RACP_OPERATOR_FIRST ||
      oper == CH_RACP_OPERATOR_LAST || oper == CH_RACP_OPERATOR_NULL)
  {
    if (len != 0)
    {
      status = CH_RACP_RSP_INV_OPERAND;
    }
  }
  /* remaining operators must have operands */
  else if (oper == CH_RACP_OPERATOR_LTEQ || oper == CH_RACP_OPERATOR_GTEQ ||
           oper == CH_RACP_OPERATOR_RANGE)
  {
    if (len == 0)
    {
      status = CH_RACP_RSP_INV_OPERAND;
    }
    else
    {
      filterType = *pOperand;
      len--;

      /* operand length depends on filter type */
      if (filterType == CH_RACP_GLS_FILTER_SEQ)
      {
        filterLen = CH_RACP_GLS_FILTER_SEQ_LEN;
      }
      else if (filterType == CH_RACP_GLS_FILTER_TIME)
      {
        filterLen = CH_RACP_GLS_FILTER_TIME_LEN;
      }
      else
      {
        status = CH_RACP_RSP_OPERAND_NOT_SUP;
      }

      if (status == CH_RACP_RSP_SUCCESS)
      {
        /* range operator has two filters, others have one */
        if (oper == CH_RACP_OPERATOR_RANGE)
        {
          filterLen *= 2;
        }

        /* verify length */
        if (len != filterLen)
        {
          status = CH_RACP_RSP_INV_OPERAND;
        }
      }
    }
  }
  /* unknown operator */
  else
  {
    status = CH_RACP_RSP_OPERATOR_NOT_SUP;
  }

  /* store operator and operand */
  if (status == CH_RACP_RSP_SUCCESS)
  {
    cgmpsCb.oper = oper;
    memcpy(cgmpsCb.operand, pOperand, len);
  }

  return status;
}

/*************************************************************************************************/
/*!
 *  \brief  Handle a RACP report stored records operation.
 *
 *  \param  connId      Connection ID.
 *  \param  oper        Operator.
 *  \param  pOperand    Operand data.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsRacpReport(dmConnId_t connId, uint8_t oper, uint8_t *pOperand)
{
  uint8_t status;

  /* if record found */
  if ((status = cgmpsDbGetNextRecord(oper, pOperand, NULL, &cgmpsCb.pCurrRec)) == CH_RACP_RSP_SUCCESS)
  {
    /* send measurement */
    cgmpsSendMeas(connId, cgmpsCb.pCurrRec);
  }
  /* if not successful send response */
  else
  {
    cgmpsRacpSendRsp(connId, CH_RACP_OPCODE_REPORT, status);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Handle a RACP delete records operation.
 *
 *  \param  connId      Connection ID.
 *  \param  oper        Operator.
 *  \param  pOperand    Operand data.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsRacpDelete(dmConnId_t connId, uint8_t oper, uint8_t *pOperand)
{
  uint8_t status;

  /* delete records */
  status = cgmpsDbDeleteRecords(oper, pOperand);

  /* send response */
  cgmpsRacpSendRsp(connId, CH_RACP_OPCODE_DELETE, status);
}

/*************************************************************************************************/
/*!
 *  \brief  Handle a RACP abort operation.
 *
 *  \param  connId      Connection ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsRacpAbort(dmConnId_t connId)
{
  /* if operation in progress */
  if (cgmpsCb.inProgress)
  {
    /* abort operation and clean up */
    cgmpsCb.pCurrRec = NULL;
  }

  /* send response */
  if (cgmpsCb.txReady)
  {
    cgmpsRacpSendRsp(connId, CH_RACP_OPCODE_ABORT, CH_RACP_RSP_SUCCESS);
  }
  else
  {
    cgmpsCb.aborting = TRUE;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Handle a RACP report number of stored records operation.
 *
 *  \param  connId      Connection ID.
 *  \param  oper        Operator.
 *  \param  pOperand    Operand data.

 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsRacpReportNum(dmConnId_t connId, uint8_t oper, uint8_t *pOperand)
{
  uint8_t status;
  uint8_t numRec = 0;

  /* get number of records */
  status = cgmpsDbGetNumRecords(oper, pOperand, &numRec);

  if (status == CH_RACP_RSP_SUCCESS)
  {
    /* send response */
    cgmpsRacpSendNumRecRsp(connId, numRec);
  }
  else
  {
    cgmpsRacpSendRsp(connId, CH_RACP_OPCODE_REPORT_NUM, status);
  }
}

/*************************************************************************************************/
/*!
 *
 *  \brief  Toggle bonding flag in Glucose Feature Characteristic.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void cgmpsToggleBondingFlag(void)
{
  uint8_t *pCgmpsFlagsValue;
  uint16_t len;

  /* Get flags */
  if (AttsGetAttr(GLS_GLF_HDL, &len, &pCgmpsFlagsValue) == ATT_SUCCESS)
  {
    uint16_t cgmpsFlags;

    BYTES_TO_UINT16(cgmpsFlags, pCgmpsFlagsValue);

    if (cgmpsFlags & CH_GLF_MULTI_BOND)
    {
      cgmpsFlags &= ~CH_GLF_MULTI_BOND;
    }
    else
    {
      cgmpsFlags |= CH_GLF_MULTI_BOND;
    }

    CgmpsSetFeature(cgmpsFlags);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     CgmpsInit
 *
 *  \brief  Initialize the CGM profile sensor.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmpsInit(void)
{
  cgmpsDbInit();
}

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
void CgmpsProcMsg(wsfMsgHdr_t *pMsg)
{
  switch(pMsg->event)
  {
    case DM_CONN_OPEN_IND:
      cgmpsConnOpen((dmEvt_t *) pMsg);
      break;

    case DM_CONN_CLOSE_IND:
      cgmpsConnClose((dmEvt_t *) pMsg);
      break;

    case ATTS_HANDLE_VALUE_CNF:
      cgmpsHandleValueCnf((attEvt_t *) pMsg);
      break;

    default:
      break;
  }
}

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
void CgmpsBtn(dmConnId_t connId, uint8_t btn)
{
  /* button actions when connected */
  if (connId != DM_CONN_ID_NONE)
  {
    switch (btn)
    {
      case APP_UI_BTN_2_MED:
        /* Toggle medication quantity, for test purposes only */
        cgmpsDbToggleMedicationUnits();
        break;

      case APP_UI_BTN_2_LONG:
        /* generate a new record */
        cgmpsDbGenerateRecord();
        break;

      default:
        break;
    }
  }
  /* button actions when not connected */
  else
  {
    switch (btn)
    {
      case APP_UI_BTN_2_SHORT:
        /* Toggle bonding flag */
        cgmpsToggleBondingFlag();
        break;

      case APP_UI_BTN_2_MED:
        /* Toggle medication quantity, for test purposes only */
        cgmpsDbToggleMedicationUnits();
        break;

      case APP_UI_BTN_2_LONG:
        /* generate a new record */
        cgmpsDbGenerateRecord();
        break;

      case APP_UI_BTN_2_EX_LONG:
        /* delete all records */
        cgmpsDbDeleteRecords(CH_RACP_OPERATOR_ALL, NULL);
        break;

      default:
        break;
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  ATTS write callback for glucose service record access control point.  Use this
 *          function as a parameter to SvcGlsCbackRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t CgmpsRacpWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                           uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
  uint8_t opcode;
  uint8_t oper;
  uint8_t status;

  /* sanity check on length */
  if (len < GLPS_RACP_MIN_WRITE_LEN)
  {
    return ATT_ERR_LENGTH;
  }

  /* if control point not configured for indication */
  if (!AttsCccEnabled(connId, cgmpsCb.racpCccIdx))
  {
    return GLS_ERR_CCCD;
  }

  /* parse opcode and operator and adjust remaining parameter length */
  BSTREAM_TO_UINT8(opcode, pValue);
  BSTREAM_TO_UINT8(oper, pValue);
  len -= 2;

  /* handle a procedure in progress */
  if (opcode != CH_RACP_OPCODE_ABORT && cgmpsCb.inProgress)
  {
    return GLS_ERR_IN_PROGRESS;
  }

  /* handle record request when notifications not enabled */
  if (opcode == CH_RACP_OPCODE_REPORT && !AttsCccEnabled(connId, cgmpsCb.glmCccIdx))
  {
    return GLS_ERR_CCCD;
  }

  /* verify operands */
  if ((status = cgmpsRacpOperCheck(oper, len, pValue)) == CH_RACP_RSP_SUCCESS)
  {
    switch (opcode)
    {
      /* report records */
      case CH_RACP_OPCODE_REPORT:
        cgmpsRacpReport(connId, oper, pValue);
        break;

      /* delete records */
      case CH_RACP_OPCODE_DELETE:
        cgmpsRacpDelete(connId, oper, pValue);
        break;

      /* abort current operation */
      case CH_RACP_OPCODE_ABORT:
        cgmpsRacpAbort(connId);
        break;

      /* report number of records */
      case CH_RACP_OPCODE_REPORT_NUM:
        cgmpsRacpReportNum(connId, oper, pValue);
        break;

      /* unsupported opcode */
      default:
        cgmpsRacpSendRsp(connId, opcode, CH_RACP_RSP_OPCODE_NOT_SUP);
        break;
    }
  }
  /* else invalid operands; send response with failure status */
  else
  {
    cgmpsRacpSendRsp(connId, opcode, status);
  }

  /* procedure now in progress */
  cgmpsCb.inProgress = TRUE;

  return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  Set the supported features of the glucose sensor.
 *
 *  \param  feature   Feature bitmask.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmpsSetFeature(uint16_t feature)
{
  uint8_t buf[2] = {UINT16_TO_BYTES(feature)};

  AttsSetAttr(GLS_GLF_HDL, sizeof(buf), buf);
}

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
void CgmpsSetCccIdx(uint8_t cgmCccIdx, uint8_t cgmStatusCccIdx, uint8_t racpCccIdx)
{
  cgmpsCb.glmCccIdx = cgmCccIdx;
  cgmpsCb.racpCccIdx = racpCccIdx;
}

