/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Proprietary data transfer server sample application.
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
#ifndef EXAMPLES_MAX32655_BLE_OTAS_DATS_API_H_
#define EXAMPLES_MAX32655_BLE_OTAS_DATS_API_H_

#include "wsf_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! enumeration of client characteristic configuration descriptors */
enum
{
  GLUC_GATT_SC_CCC_IDX,                    /*! GATT service, service changed characteristic */
  GLUC_GLS_GLM_CCC_IDX,                    /*! Glucose service, glucose measurement characteristic */
  GLUC_GLS_GLMC_CCC_IDX,                   /*! Glucose service, glucose measurement context characteristic */
  GLUC_GLS_RACP_CCC_IDX,                   /*! Glucose service, record access control point characteristic */
  GLUC_NUM_CCC_IDX
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void DatsStart(void);

/*************************************************************************************************/
/*!
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID for App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void DatsHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief  WSF event handler for the application.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void DatsHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

#ifdef __cplusplus
};
#endif

#endif // EXAMPLES_MAX32655_BLE_OTAS_DATS_API_H_
