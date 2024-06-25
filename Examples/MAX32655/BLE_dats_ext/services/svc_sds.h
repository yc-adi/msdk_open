/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. All Rights Reserved.
 * (now owned by Analog Devices, Inc.),
 * Copyright (C) 2023 Analog Devices, Inc. All Rights Reserved. This software
 * is proprietary to Analog Devices, Inc. and its licensors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/*************************************************************************************************/
/*! Secure Data Service 
*   Implementation of a characteristic with elevated security features.
*   The connection must be encrypted with an authenticated key to read/write
*   the attributes value.
*
 */
/*************************************************************************************************/

#ifndef EXAMPLES_MAX32655_BLUETOOTH_BLE_DATS_EXT_SERVICES_SVC_SDS_H_
#define EXAMPLES_MAX32655_BLUETOOTH_BLE_DATS_EXT_SERVICES_SVC_SDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Service and Characteristic UUIDs
**************************************************************************************************/
#define ATT_UUID_SEC_DATA_SERVICE \
    0xBE, 0x35, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x42, 0xD9, 0x32, 0x7E, 0x36, 0xFC, 0x42
/* MCS service GATT characteristic UUIDs*/
#define ATT_UUID_SEC_DATA \
    0xBE, 0x35, 0xD1, 0x24, 0x99, 0x33, 0xC6, 0x87, 0x85, 0x41, 0xD9, 0x31, 0x3E, 0x56, 0xFC, 0x42

/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/
/*! \brief Secured Data Service */
#define SEC_DATA_START_HDL 0x300 /*!< \brief Start handle. */
#define SEC_DATA_END_HDL (SEC_DAT_MAX_HDL - 1) /*!< \brief End handle. */

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! \brief Secured Service Handles */
enum {
    SEC_DATA_SVC_HDL = SEC_DATA_START_HDL, /*!< \brief Secured Data service declaration */
    SEC_DAT_CH_HDL, /*!< \brief Secured Data characteristic */
    SEC_DAT_HDL, /*!< \brief Secured Data  */
    SEC_DAT_CH_CCC_HDL, /*!< \brief Secured Data client characteristic configuration */
    SEC_DAT_MAX_HDL /*!< \brief Maximum handle. */
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
void SvcSecDataAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcSecDataRemoveGroup(void);

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
void SvcSecDataCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

/*! \} */ /* WP_SERVICE */

#ifdef __cplusplus
};
#endif

#endif // EXAMPLES_MAX32655_BLUETOOTH_BLE_DATS_EXT_SERVICES_SVC_SDS_H_
