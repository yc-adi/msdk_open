/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Glucose profile example record database and access functions.
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

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "att_api.h"
#include "svc_ch.h"
#include "svc_cgms.h"
#include "app_api.h"
#include "cgmps_api.h"
#include "cgmps_main.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Number of records in example database */
#define CGM_DB_NUM_RECORDS     3

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Control block */
cgmpsDbCb_t cgmpsDbCb;

/*! Example record database */
static cgmpsRec_t cgmpsDb[CGM_DB_NUM_RECORDS] =
{
  /* record 1 */
  {
    /* measurement */
    {
      CH_GLM_FLAG_TIME_OFFSET | CH_GLM_FLAG_CONC_TYPE_LOC |     /* Flags */
      CH_GLM_FLAG_UNITS_MOL_L | CH_GLM_FLAG_SENSOR_STATUS,
      0x0001,                                                   /* Sequence number */
      {2012, 6, 15, 20, 52, 36},                                /* Base time */
      60,                                                       /* Time offset */
      SFLT_TO_UINT16(50, -3),                                   /* Glucose concentration (SFLOAT) */
      CH_GLM_LOC_FINGER | (CH_GLM_TYPE_ART_BLOOD << 4),         /* Sample type and sample location */
      CH_GLM_STATUS_BATT_LOW                                    /* Sensor status annunciation */
    },
  },

  /* record 2 */
  {
    /* measurement */
    {
      CH_GLM_FLAG_CONC_TYPE_LOC | CH_GLM_FLAG_UNITS_KG_L |      /* Flags */
      CH_GLM_FLAG_CONTEXT_INFO,
      0x0002,                                                   /* Sequence number */
      {2012, 6, 16, 6, 45, 20},                                 /* Base time */
      0,                                                        /* Time offset */
      SFLT_TO_UINT16(20, -5),                                   /* Glucose concentration (SFLOAT) */
      CH_GLM_LOC_FINGER | (CH_GLM_TYPE_ART_BLOOD << 4),         /* Sample type and sample location */
      0                                                         /* Sensor status annunciation */
    },
  },

  /* record 3 */
  {
    /* measurement */
    {
      CH_GLM_FLAG_CONC_TYPE_LOC | CH_GLM_FLAG_UNITS_KG_L,       /* Flags */
      0x0003,                                                   /* Sequence number */
      {2012, 6, 16, 18, 45, 20},                                /* Base time */
      0,                                                        /* Time offset */
      SFLT_TO_UINT16(20, -5),                                   /* Glucose concentration (SFLOAT) */
      CH_GLM_LOC_FINGER | (CH_GLM_TYPE_ART_BLOOD << 4),         /* Sample type and sample location */
      0                                                         /* Sensor status annunciation */
    },
  }
};

/*************************************************************************************************/
/*!
 *  \brief  Get last database record.
 *
 *  \return Pointer to record or NULL if no record.
 */
/*************************************************************************************************/
static cgmpsRec_t *cgmpsDbGetEnd(void)
{
  if (cgmpsDbCb.numRec == 0)
  {
    return NULL;
  }

  return (cgmpsRec_t *) &cgmpsDb[cgmpsDbCb.numRec - 1];
}

/*************************************************************************************************/
/*!
 *  \brief  Get the next database record after the given current record.
 *
 *  \param  pCurrRec    Pointer to current record.
 *
 *  \return Pointer to record or NULL if no record.
 */
/*************************************************************************************************/
static cgmpsRec_t *cgmpsDbGetNext(cgmpsRec_t *pCurrRec)
{
  if (cgmpsDbCb.numRec == 0 || pCurrRec == cgmpsDbGetEnd())
  {
    return NULL;
  }
  else if (pCurrRec == NULL)
  {
    return (cgmpsRec_t *) cgmpsDb;
  }
  else
  {
    return (pCurrRec + 1);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get the next record filtered for "all".
 *
 *  \param  pCurrRec    Pointer to current record.
 *  \param  pRec        Return pointer to next record, if found.
 *
 *  \return CH_RACP_RSP_SUCCESS if a record is found, otherwise an error status is returned.
 */
/*************************************************************************************************/
static uint8_t cgmpsDbOpAll(cgmpsRec_t *pCurrRec,  cgmpsRec_t **pRec)
{
  *pRec = cgmpsDbGetNext(pCurrRec);
  return (*pRec != NULL) ? CH_RACP_RSP_SUCCESS : CH_RACP_RSP_NO_RECORDS;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the next record filtered for "greater than or equal to sequence number".
 *
 *  \param  pFilter     Glucose service RACP filter parameters.
 *  \param  pCurrRec    Pointer to current record.
 *  \param  pRec        Return pointer to next record, if found.
 *
 *  \return CH_RACP_RSP_SUCCESS if a record is found, otherwise an error status is returned.
 */
/*************************************************************************************************/
static uint8_t cgmpsDbOpGteqSeqNum(uint8_t *pFilter, cgmpsRec_t *pCurrRec,  cgmpsRec_t **pRec)
{
  uint16_t seqNum;

  /* parse seq number */
  pFilter++;
  BYTES_TO_UINT16(seqNum, pFilter);

  /* find record */
  while ((*pRec = cgmpsDbGetNext(pCurrRec)) != NULL)
  {
    if ((*pRec)->meas.seqNum >= seqNum)
    {
      return CH_RACP_RSP_SUCCESS;
    }
    pCurrRec = *pRec;
  }

  return CH_RACP_RSP_NO_RECORDS;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the next record filtered for "last".
 *
 *  \param  pCurrRec    Pointer to current record.
 *  \param  pRec        Return pointer to next record, if found.
 *
 *  \return CH_RACP_RSP_SUCCESS if a record is found, otherwise an error status is returned.
 */
/*************************************************************************************************/
static uint8_t cgmpsDbOpLast(cgmpsRec_t *pCurrRec, cgmpsRec_t **pRec)
{
  /* if current record is already last return failure */
  *pRec = cgmpsDbGetEnd();
  if (*pRec != NULL && *pRec != pCurrRec)
  {
    return CH_RACP_RSP_SUCCESS;
  }
  else
  {
    return CH_RACP_RSP_NO_RECORDS;
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize the CGM record database.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmpsDbInit(void)
{
  cgmpsDbCb.numRec = CGM_DB_NUM_RECORDS;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the next record that matches the given filter parameters that follows
 *          the given current record.
 *
 *  \param  oper        Operator.
 *  \param  pFilter     Glucose service RACP filter parameters.
 *  \param  pCurrRec    Pointer to current record.
 *  \param  pRec        Return pointer to next record, if found.
 *
 *  \return CH_RACP_RSP_SUCCESS if a record is found, otherwise an error status is returned.
 */
/*************************************************************************************************/
uint8_t cgmpsDbGetNextRecord(uint8_t oper, uint8_t *pFilter, cgmpsRec_t *pCurrRec,  cgmpsRec_t **pRec)
{
  uint8_t status;
  
  // TODO: get the record according to filter

  status = cgmpsDbOpLast(pCurrRec, pRec);

  return status;
}

/*************************************************************************************************/
/*!
 *  \brief  Delete records that match the given filter parameters.
 *
 *  \param  oper        Operator.
 *  \param  pFilter     Glucose service RACP filter parameters.
 *
 *  \return CH_RACP_RSP_SUCCESS if records deleted, otherwise an error status is returned.
 */
/*************************************************************************************************/
uint8_t cgmpsDbDeleteRecords(uint8_t oper, uint8_t *pFilter)
{
    cgmpsDbCb.numRec = 0;

    return CH_RACP_RSP_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the number of records matching the filter parameters.
 *
 *  \param  oper        Operator.
 *  \param  pFilter     Glucose service RACP filter parameters.
 *  \param  pNumRec     Returns number of records which match filter parameters.

 *
 *  \return RACP status.
 */
/*************************************************************************************************/
uint8_t cgmpsDbGetNumRecords(uint8_t oper, uint8_t *pFilter, uint8_t *pNumRec)
{
  cgmpsRec_t *pCurrRec = NULL;
  uint8_t   status;

  *pNumRec = 0;
  while ((status = cgmpsDbGetNextRecord(oper, pFilter, pCurrRec, &pCurrRec)) == CH_RACP_RSP_SUCCESS)
  {
    (*pNumRec)++;
  }

  if (status == CH_RACP_RSP_NO_RECORDS)
  {
    status = CH_RACP_RSP_SUCCESS;
  }

  return status;
}

/*************************************************************************************************/
/*!
*  \brief  Generate a new record.
*
*  \return None.
*/
/*************************************************************************************************/
void cgmpsDbGenerateRecord(void)
{
  if (cgmpsDbCb.numRec < CGM_DB_NUM_RECORDS)
  {
    cgmpsDbCb.numRec++;
  }
}

/*************************************************************************************************/
/*!
*  \brief  For conformance testing only.  Toggle the sample data record number 2's medication
*          quantity unit flag between Kilograms and Liters.  Also modifies quantity.
*
*  \return None.
*/
 /*************************************************************************************************/
void cgmpsDbToggleMedicationUnits(void)
{
  //TODO
}
