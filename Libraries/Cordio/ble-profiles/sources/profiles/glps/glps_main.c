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
#include "svc_gls.h"
#include "app_api.h"
#include "app_ui.h"
#include "glps_api.h"
#include "glps_main.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/
/*! \brief Connection control block */
typedef struct
{
  dmConnId_t    connId;               /*! \brief Connection ID */
  bool_t        glmToSend;            /*! \brief glucose measurement ready to be sent on this channel */
} glpsConn_t;

/* Control block */
static struct
{
  glpsConn_t    conn[DM_CONN_MAX];          /* \brief connection control block */
  wsfTimer_t    measTimer;                  /* \brief periodic measurement timer */
  uint16_t      cgmPeriodSec;               /* CGM period in second*/
  uint8_t       operand[GLPS_OPERAND_MAX];  /* Stored operand filter data */
  glpsRec_t     *pCurrRec;                  /* Pointer to current measurement record */
  bool_t        inProgress;                 /* TRUE if RACP procedure in progress */
  bool_t        txReady;                    /* TRUE if ready to send next notification or indication */
  bool_t        aborting;                   /* TRUE if abort procedure in progress */
  uint8_t       glmCccIdx;                  /* Glucose measurement CCCD index */
  uint8_t       glmcCccIdx;                 /* Glucose measurement context CCCD index */
  uint8_t       racpCccIdx;                 /* Record access control point CCCD index */
  uint8_t       oper;                       /* Stored operator */
} glpsCb;

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
static uint8_t glpsBuildGlm(uint8_t *pBuf, glpsGlm_t *pGlm)
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
static uint8_t glpsBuildGlmc(uint8_t *pBuf, glpsGlmc_t *pGlmc)
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
void glpsSendMeasContext(dmConnId_t connId, glpsRec_t *pRec)
{
  uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN];
  uint8_t len;

  /* build glucose measurement context characteristic */
  len = glpsBuildGlmc(buf, &glpsCb.pCurrRec->context);

  /* send notification */
  AttsHandleValueNtf(connId, GLS_GLMC_HDL, len, buf);
  glpsCb.txReady = FALSE;
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
void glpsSendMeas(dmConnId_t connId, glpsRec_t *pRec)
{
  uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN];
  uint8_t len;

  /* build glucose measurement characteristic */
  len = glpsBuildGlm(buf, &glpsCb.pCurrRec->meas);

  /* send notification */
  AttsHandleValueNtf(connId, GLS_GLM_HDL, len, buf);
  glpsCb.txReady = FALSE;
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
void glpsRacpSendRsp(dmConnId_t connId, uint8_t opcode, uint8_t status)
{
  uint8_t buf[GLPS_RACP_RSP_LEN];

  /* build response */
  buf[0] = CH_RACP_OPCODE_RSP;
  buf[1] = CH_RACP_OPERATOR_NULL;
  buf[2] = opcode;
  buf[3] = status;

  /* send indication */
  AttsHandleValueInd(connId, GLS_RACP_HDL, GLPS_RACP_RSP_LEN, buf);
  glpsCb.txReady = FALSE;
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
void glpsRacpSendNumRecRsp(dmConnId_t connId, uint16_t numRec)
{
  uint8_t buf[GLPS_RACP_NUM_REC_RSP_LEN];

  /* build response */
  buf[0] = CH_RACP_OPCODE_NUM_RSP;
  buf[1] = CH_RACP_OPERATOR_NULL;
  buf[2] = UINT16_TO_BYTE0(numRec);
  buf[3] = UINT16_TO_BYTE1(numRec);

  /* send indication */
  AttsHandleValueInd(connId, GLS_RACP_HDL, GLPS_RACP_RSP_LEN, buf);
  glpsCb.txReady = FALSE;
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
static void glpsConnOpen(dmEvt_t *pMsg)
{
  /* initialize */
  glpsCb.pCurrRec = NULL;
  glpsCb.aborting = FALSE;
  glpsCb.inProgress = FALSE;
  glpsCb.txReady = FALSE;
  glpsCb.oper = CH_RACP_OPERATOR_NULL;
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
static void glpsConnClose(dmEvt_t *pMsg)
{
  glpsCb.pCurrRec = NULL;
  glpsCb.aborting = FALSE;
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
static void glpsHandleValueCnf(attEvt_t *pMsg)
{
  dmConnId_t connId = (dmConnId_t) pMsg->hdr.param;
  glpsCb.txReady = TRUE;

  /* if aborting finish that up */
  if (glpsCb.aborting)
  {
    glpsCb.aborting = FALSE;
    glpsRacpSendRsp(connId, CH_RACP_OPCODE_ABORT, CH_RACP_RSP_SUCCESS);
  }

  /* if this is for RACP indication */
  if (pMsg->handle == GLS_RACP_HDL)
  {
    /* procedure no longer in progress */
    glpsCb.inProgress = FALSE;
  }
  /* if this is for measurement or context notification */
  else if (pMsg->handle == GLS_GLM_HDL || pMsg->handle == GLS_GLMC_HDL)
  {
    if (glpsCb.pCurrRec != NULL)
    {
      /* if measurement was sent and there is context to send */
      if (pMsg->handle == GLS_GLM_HDL && AttsCccEnabled(connId, glpsCb.glmcCccIdx) &&
          (glpsCb.pCurrRec->meas.flags & CH_GLM_FLAG_CONTEXT_INFO))
      {
        /* send context */
        glpsSendMeasContext(connId, glpsCb.pCurrRec);
      }
      /* else if there is another record */
      else if (glpsDbGetNextRecord(glpsCb.oper, glpsCb.operand,
                                   glpsCb.pCurrRec, &glpsCb.pCurrRec) == CH_RACP_RSP_SUCCESS)
      {
        /* send measurement */
        glpsSendMeas(connId, glpsCb.pCurrRec);
      }
      /* else all records sent; send RACP response */
      else
      {
        glpsRacpSendRsp(connId, CH_RACP_OPCODE_REPORT, CH_RACP_RSP_SUCCESS);
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
static uint8_t glpsRacpOperCheck(uint8_t oper, uint16_t len, uint8_t *pOperand)
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
    glpsCb.oper = oper;
    memcpy(glpsCb.operand, pOperand, len);
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
static void glpsRacpReport(dmConnId_t connId, uint8_t oper, uint8_t *pOperand)
{
  uint8_t status;

  /* if record found */
  if ((status = glpsDbGetNextRecord(oper, pOperand, NULL, &glpsCb.pCurrRec)) == CH_RACP_RSP_SUCCESS)
  {
    /* send measurement */
    glpsSendMeas(connId, glpsCb.pCurrRec);
  }
  /* if not successful send response */
  else
  {
    glpsRacpSendRsp(connId, CH_RACP_OPCODE_REPORT, status);
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
static void glpsRacpDelete(dmConnId_t connId, uint8_t oper, uint8_t *pOperand)
{
  uint8_t status;

  /* delete records */
  status = glpsDbDeleteRecords(oper, pOperand);

  /* send response */
  glpsRacpSendRsp(connId, CH_RACP_OPCODE_DELETE, status);
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
static void glpsRacpAbort(dmConnId_t connId)
{
  /* if operation in progress */
  if (glpsCb.inProgress)
  {
    /* abort operation and clean up */
    glpsCb.pCurrRec = NULL;
  }

  /* send response */
  if (glpsCb.txReady)
  {
    glpsRacpSendRsp(connId, CH_RACP_OPCODE_ABORT, CH_RACP_RSP_SUCCESS);
  }
  else
  {
    glpsCb.aborting = TRUE;
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
static void glpsRacpReportNum(dmConnId_t connId, uint8_t oper, uint8_t *pOperand)
{
  uint8_t status;
  uint8_t numRec = 0;

  /* get number of records */
  status = glpsDbGetNumRecords(oper, pOperand, &numRec);

  if (status == CH_RACP_RSP_SUCCESS)
  {
    /* send response */
    glpsRacpSendNumRecRsp(connId, numRec);
  }
  else
  {
    glpsRacpSendRsp(connId, CH_RACP_OPCODE_REPORT_NUM, status);
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
static void glpsToggleBondingFlag(void)
{
  uint8_t *pGlpsFlagsValue;
  uint16_t len;

  /* Get flags */
  if (AttsGetAttr(GLS_GLF_HDL, &len, &pGlpsFlagsValue) == ATT_SUCCESS)
  {
    uint16_t glpsFlags;

    BYTES_TO_UINT16(glpsFlags, pGlpsFlagsValue);

    if (glpsFlags & CH_GLF_MULTI_BOND)
    {
      glpsFlags &= ~CH_GLF_MULTI_BOND;
    }
    else
    {
      glpsFlags |= CH_GLF_MULTI_BOND;
    }

    GlpsSetFeature(glpsFlags);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     GlpsInit
 *
 *  \brief  Initialize the Glucose profile sensor.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GlpsInit(uint16_t cgm_period_sec)
{
  glpsCb.cgmPeriodSec = cgm_period_sec;
  glpsDbInit();
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
void GlpsProcMsg(wsfMsgHdr_t *pMsg)
{
  if (pMsg->event == DM_CONN_OPEN_IND) {
      glpsConnOpen((dmEvt_t *) pMsg);
  } else if (pMsg->event == DM_CONN_CLOSE_IND) {
      glpsConnClose((dmEvt_t *) pMsg);
  } else if (pMsg->event == ATTS_HANDLE_VALUE_CNF) {
      glpsHandleValueCnf((attEvt_t *) pMsg);
  } else if (pMsg->event == glpsCb.measTimer.msg.event) {
      GlpsMeasTimerExp(pMsg);
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
void GlpsBtn(dmConnId_t connId, uint8_t btn)
{
  /* button actions when connected */
  if (connId != DM_CONN_ID_NONE)
  {
    switch (btn)
    {
      case APP_UI_BTN_2_MED:
        /* Toggle medication quantity, for test purposes only */
        glpsDbToggleMedicationUnits();
        break;

      case APP_UI_BTN_2_LONG:
        /* generate a new record */
        glpsDbGenerateRecord();
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
        glpsToggleBondingFlag();
        break;

      case APP_UI_BTN_2_MED:
        /* Toggle medication quantity, for test purposes only */
        glpsDbToggleMedicationUnits();
        break;

      case APP_UI_BTN_2_LONG:
        /* generate a new record */
        glpsDbGenerateRecord();
        break;

      case APP_UI_BTN_2_EX_LONG:
        /* delete all records */
        glpsDbDeleteRecords(CH_RACP_OPERATOR_ALL, NULL);
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
uint8_t GlpsRacpWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
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
  if (!AttsCccEnabled(connId, glpsCb.racpCccIdx))
  {
    return GLS_ERR_CCCD;
  }

  /* parse opcode and operator and adjust remaining parameter length */
  BSTREAM_TO_UINT8(opcode, pValue);
  BSTREAM_TO_UINT8(oper, pValue);
  len -= 2;

  /* handle a procedure in progress */
  if (opcode != CH_RACP_OPCODE_ABORT && glpsCb.inProgress)
  {
    return GLS_ERR_IN_PROGRESS;
  }

  /* handle record request when notifications not enabled */
  if (opcode == CH_RACP_OPCODE_REPORT && !AttsCccEnabled(connId, glpsCb.glmCccIdx))
  {
    return GLS_ERR_CCCD;
  }

  /* verify operands */
  if ((status = glpsRacpOperCheck(oper, len, pValue)) == CH_RACP_RSP_SUCCESS)
  {
    switch (opcode)
    {
      /* report records */
      case CH_RACP_OPCODE_REPORT:
        glpsRacpReport(connId, oper, pValue);
        break;

      /* delete records */
      case CH_RACP_OPCODE_DELETE:
        glpsRacpDelete(connId, oper, pValue);
        break;

      /* abort current operation */
      case CH_RACP_OPCODE_ABORT:
        glpsRacpAbort(connId);
        break;

      /* report number of records */
      case CH_RACP_OPCODE_REPORT_NUM:
        glpsRacpReportNum(connId, oper, pValue);
        break;

      /* unsupported opcode */
      default:
        glpsRacpSendRsp(connId, opcode, CH_RACP_RSP_OPCODE_NOT_SUP);
        break;
    }
  }
  /* else invalid operands; send response with failure status */
  else
  {
    glpsRacpSendRsp(connId, opcode, status);
  }

  /* procedure now in progress */
  glpsCb.inProgress = TRUE;

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
void GlpsSetFeature(uint16_t feature)
{
  uint8_t buf[2] = {UINT16_TO_BYTES(feature)};

  AttsSetAttr(GLS_GLF_HDL, sizeof(buf), buf);
}

/*************************************************************************************************/
/*!
 *  \brief  Set the CCCD index used by the application for glucose service characteristics.
 *
 *  \param  glmCccIdx   Glucose measurement CCCD index.
 *  \param  glmcCccIdx  Glucose measurement context CCCD index.
 *  \param  racpCccIdx  Record access control point CCCD index.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GlpsSetCccIdx(uint8_t glmCccIdx, uint8_t glmcCccIdx, uint8_t racpCccIdx)
{
  glpsCb.glmCccIdx = glmCccIdx;
  glpsCb.glmcCccIdx = glmCccIdx;
  glpsCb.racpCccIdx = racpCccIdx;
}

/*************************************************************************************************/
/*!
 *  \brief  Return TRUE if no connections with active measurements.
 *
 *  \return TRUE if no connections active.
 */
/*************************************************************************************************/
static bool_t glpsNoConnActive(void)
{
  glpsConn_t *pConn = glpsCb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, pConn++)
  {
    if (pConn->connId != DM_CONN_ID_NONE)
    {
      return FALSE;
    }
  }
  return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Start periodic glucose measurement.  This function starts a timer to perform
 *          periodic measurements.
 *
 *  \param  connId      DM connection identifier.
 *  \param  timerEvt    WSF event designated by the application for the timer.
 *  \param  glsGlmCccIdx   Index of glucose measurement CCC descriptor in CCC descriptor handle table.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GlpsMeasStart(dmConnId_t connId, uint8_t timerEvt, uint8_t glsGlmCccIdx)
{
  /* if this is first connection */
  if (glpsNoConnActive())
  {
    /* initialize control block */
    glpsCb.measTimer.msg.event = timerEvt;
    glpsCb.measTimer.msg.status = glsGlmCccIdx;

    /* start timer */
    WsfTimerStartMs(&glpsCb.measTimer, glpsCb.cgmPeriodSec * 1000);
  }

  /* set conn id */
  glpsCb.conn[connId - 1].connId = connId;
}

/*************************************************************************************************/
/*!
 *  \brief  Stop periodic glucose measurement.
 *
 *  \param  connId      DM connection identifier.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GlpsMeasStop(dmConnId_t connId)
{
  /* clear connection */
  glpsCb.conn[connId - 1].connId = DM_CONN_ID_NONE;
  glpsCb.conn[connId - 1].glmToSend = FALSE;

  /* if no remaining connections */
  if (glpsNoConnActive())
  {
    /* stop timer */
    WsfTimerStop(&glpsCb.measTimer);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Find next connection with measurement to send.
 *
 *  \param  cccIdx  glucose measurement CCC descriptor index.
 *
 *  \return Connection control block.
 */
/*************************************************************************************************/
static glpsConn_t *glpsFindNextToSend(uint8_t cccIdx)
{
  glpsConn_t *pConn = glpsCb.conn;
  uint8_t i;

  for (i = 0; i < DM_CONN_MAX; i++, pConn++)
  {
    if (pConn->connId != DM_CONN_ID_NONE && pConn->glmToSend)
    {
      if (AttsCccEnabled(pConn->connId, cccIdx))
      {
        return pConn;
      }
    }
  }
  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief  This function is called by the application when the periodic measurement
 *          timer expires.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void GlpsMeasTimerExp(wsfMsgHdr_t *pMsg)
{
  glpsConn_t  *pConn;

  /* if there are active connections */
  if (glpsNoConnActive() == FALSE)
  {
    /* read heart rate measurement sensor data */
    APP_TRACE_INFO0("TODO: retrieve/setup glucose measurement data."); // TODO, @?@, remove me !!!

    /* if ready to send measurements */
    if (glpsCb.txReady)
    {
      /* find next connection to send (note ccc idx is stored in timer status) */
      if ((pConn = glpsFindNextToSend(pMsg->status)) != NULL)
      {
        //glpsSendHrmNtf(pConn->connId);
        APP_TRACE_INFO0("TODO: send glucose measurement notification");
        glpsCb.txReady = FALSE;
        pConn->glmToSend = FALSE;
      }
    }

    /* restart timer */
    WsfTimerStartMs(&glpsCb.measTimer, glpsCb.cgmPeriodSec * 1000);
  }
}
