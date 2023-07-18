/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Example CGM service implementation.
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

#ifndef SVC_CGMS_H
#define SVC_CGMS_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup GLUCOSE_SERVICE
 *  \{ */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/** \name Error Codes
 *
 */
/**@{*/
#define CGMS_ERR_IN_PROGRESS      0x80    /*!< \brief Procedure already in progress */
#define CGMS_ERR_CCCD             0x81    /*!< \brief CCCD improperly configured */
/**@}*/

/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/** \name Glucose Service Handles
 *
 */
/**@{*/
#define CGMS_START_HDL               0xF0              /*!< \brief Start handle. */
#define CGMS_END_HDL                 (CGMS_MAX_HDL - 1) /*!< \brief End handle. */

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! \brief CGM Service Handles (must match cgmsList[]) */
enum cgms_hdl {
  CGMS_SVC_HDL = CGMS_START_HDL,    /*!< \brief CGM service declaration */

  CGMS_MEAS_CH_HDL,                 /*!< \brief CGM measurement characteristic */
  CGMS_MEAS_HDL,                    /*!< \brief CGM measurement value */
  CGMS_MEAS_CH_CCC_HDL,             /*!< \brief CGM measurement CCCD */
   
  CGMS_FEAT_CH_HDL,                 /*!< \brief CGM feature characteristic: ATT_UUID_CGM_FEATURE */
  CGMS_FEAT_HDL,                    /*!< \brief CGM feature, 245 */

  CGMS_ST_CH_HDL,                   /*!< \brief CGM status characteristic: ATT_UUID_CGM_STATUS */
  CGMS_ST_HDL,                      /*!< \brief CGM status, 247 */

  CGMS_SESS_START_T_CH_HDL,         /*!< \brief CGM session start time characteristic: ATT_UUID_CGM_SESS_START_T */
  CGMS_SESS_START_T_HDL,            /*!< \brief CGM session start time */

  CGMS_SESS_RUN_T_CH_HDL,           /*!< \brief CGM session run time characteristic: ATT_UUID_CGM_SESS_RUN_T */
  CGMS_SESS_RUN_T_HDL,              /*!< \brief CGM session run time */

  CGMS_RACP_CH_HDL,                 /*!< \brief Record access control point characteristic */
  CGMS_RACP_HDL,                    /*!< \brief Record access control point */
  CGMS_RACP_CH_CCC_HDL,             /*!< \brief RACP client characteristic configuration */

#if 0
  CGMS_SOPS_CH_HDL,                 /*!< \brief CGM Specific Ops Control Point characteristic */
  CGMS_SOPS_HDL,                    /*!< \brief CGM Specific Ops Control Point */
  CGMS_SOPS_CCCD_HDL,               /*!< \brief CGM Specific Ops Control Point CCCD */
#endif

  CGMS_MAX_HDL                       /*!< \brief Maximum handle. */
};

enum // @?@ remove me !!!
{
  GLS_GLMC_CH_HDL,                  /*!< \brief Glucose measurement context characteristic */
  GLS_GLMC_HDL,                     /*!< \brief Glucose measurement context */
  GLS_GLMC_CH_CCC_HDL               /*!< \brief Glucose measurement context client characteristic configuration */
};

/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCgmsAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCgmsRemoveGroup(void);

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
void SvcCgmsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

/*! \} */    /* GLUCOSE_SERVICE */

#ifdef __cplusplus
};
#endif

#endif /* SVC_CGMS_H */
