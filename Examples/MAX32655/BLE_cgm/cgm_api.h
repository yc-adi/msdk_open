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
#ifndef EXAMPLES_MAX32655_BLE_CGM_CGM_API_H
#define EXAMPLES_MAX32655_BLE_CGM_CGM_API_H

#include "wsf_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define DBG_DS          (1)

#define DBG_BUF_SIZE    (600)
#define DBG_GROUP_SIZE  (6)

#ifndef CGM_CONN_MAX
#define CGM_CONN_MAX    (1)
#endif

/*! enumeration of CGM CCCD */
enum cgm_app_ccc_idx {
  GATT_SC_CCC_IDX,                          /*! GATT service, service changed characteristic */

  WDXS_DC_CH_CCC_IDX,                       /*! WDXS DC service, service changed characteristic */
  WDXS_FTC_CH_CCC_IDX,                      /*! WDXS FTC service, service changed characteristic */
  WDXS_FTD_CH_CCC_IDX,                      /*! WDXS FTD service, service changed characteristic */
  WDXS_AU_CH_CCC_IDX,                       /*! WDXS AU service, service changed characteristic */

  DATS_WP_DAT_CCC_IDX,                      /*! Arm Ltd. proprietary service, data transfer characteristic */

  CGM_MEAS_CCC_IDX,                         /*! CGM service, CGM measurement characteristic: UUID 0x2AA7 */
  CGM_RACP_CCC_IDX,                         /*! CGM service, record access control point (RACP) characteristic: UUID 0x2A52 */
  //CGM_SOPS_CCC_IDX,                         /*! CGM service, specific ops control point (SOPS) characteristic: UUID 0x2AAC */
  
  CGM_CCC_IDX_MAX
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
void CgmStart(void);

/*************************************************************************************************/
/*!
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID for App.
 *
 *  \return None.
 */
/*************************************************************************************************/
void CgmHandlerInit(wsfHandlerId_t handlerId);

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
void CgmHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

#ifdef __cplusplus
};
#endif

#endif // EXAMPLES_MAX32655_BLE_CGM_CGM_API_H
