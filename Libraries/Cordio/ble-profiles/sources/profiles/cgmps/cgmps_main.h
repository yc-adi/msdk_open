/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Glucose profile sensor internal interfaces.
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
#ifndef GLPS_MAIN_H
#define GLPS_MAIN_H

#include "app_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup GLUCOSE_PROFILE
 *  \{ */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Minimum RACP write length */
#define GLPS_RACP_MIN_WRITE_LEN       2

/*! \brief RACP response length */
#define GLPS_RACP_RSP_LEN             4

/*! \brief Glucose RACP number of stored records response length */
#define GLPS_RACP_NUM_REC_RSP_LEN     4

/*! \brief RACP operand maximum length */
#define GLPS_OPERAND_MAX              ((CH_RACP_GLS_FILTER_TIME_LEN * 2) + 1)

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief Glucose measurement structure */
typedef struct
{
  uint8_t         flags;            /*!< \brief Flags */
  uint16_t        seqNum;           /*!< \brief Sequence number */
  appDateTime_t   baseTime;         /*!< \brief Base time */
  int16_t         timeOffset;       /*!< \brief Time offset */
  uint16_t        concentration;    /*!< \brief Glucose concentration (SFLOAT) */
  uint8_t         typeSampleLoc;    /*!< \brief Sample type and sample location */
  uint16_t        sensorStatus;     /*!< \brief Sensor status annunciation */
} cgmpsGlm_t;

/*! \brief Glucose measurement context structure */
typedef struct
{
  uint8_t         flags;            /*!< \brief Flags */
  uint16_t        seqNum;           /*!< \brief Sequence number */
  uint8_t         extFlags;         /*!< \brief Extended Flags */
  uint8_t         carbId;           /*!< \brief Carbohydrate ID */
  uint16_t        carb;             /*!< \brief Carbohydrate (SFLOAT) */
  uint8_t         meal;             /*!< \brief Meal */
  uint8_t         testerHealth;     /*!< \brief Tester and health */
  uint16_t        exerDuration;     /*!< \brief Exercise Duration */
  uint8_t         exerIntensity;    /*!< \brief Exercise Intensity */
  uint8_t         medicationId;     /*!< \brief Medication ID */
  uint16_t        medication;       /*!< \brief Medication (SFLOAT) */
  uint16_t        hba1c;            /*!< \brief HbA1c */
} cgmpsGlmc_t;

/*! \brief Glucose measurement record */
typedef struct
{
  cgmpsGlm_t     meas;               /*!< \brief Glucose measurement */
  cgmpsGlmc_t    context;            /*!< \brief Glucose measurement context */
} cgmpsRec_t;

/*************************************************************************************************/
/*!
 *  \brief  Initialize the CGM record database.
 *
 *  \return None.
 */
/*************************************************************************************************/
void cgmpsDbInit(void);

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
 *  \return \ref CH_RACP_RSP_SUCCESS if a record is found, otherwise an error status is returned.
 */
/*************************************************************************************************/
uint8_t cgmpsDbGetNextRecord(uint8_t oper, uint8_t *pFilter, cgmpsRec_t *pCurrRec,  cgmpsRec_t **pRec);

/*************************************************************************************************/
/*!
 *  \brief  Delete records that match the given filter parameters.
 *
 *  \param  oper        Operator.
 *  \param  pFilter     Glucose service RACP filter parameters.
 *
 *  \return \ref CH_RACP_RSP_SUCCESS if records deleted, otherwise an error status is returned.
 */
/*************************************************************************************************/
uint8_t cgmpsDbDeleteRecords(uint8_t oper, uint8_t *pFilter);

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
uint8_t cgmpsDbGetNumRecords(uint8_t oper, uint8_t *pFilter, uint8_t *pNumRec);

/*************************************************************************************************/
/*!
*  \brief  Generate a new record.
*
*  \return None.
*/
/*************************************************************************************************/
void cgmpsDbGenerateRecord(void);

/*************************************************************************************************/
/*!
*  \brief  For conformance testing only.  Toggle the sample data record number 2's medication
*          quantity unit flag between Kilograms and Liters.  Also modifies quantity.
*
*  \return None.
*/
 /*************************************************************************************************/
void cgmpsDbToggleMedicationUnits(void);

/*! \} */    /* GLUCOSE_PROFILE */

#ifdef __cplusplus
};
#endif

#endif /* GLPS_MAIN_H */
