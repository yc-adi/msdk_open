/*************************************************************************************************/
/*!
 *  \file   wsf_spi.h
 *
 *  \brief  SPI service.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *  
 *  Portions Copyright (c) 2023 Analog Devices, Inc.
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
#ifndef WSF_SPI_H
#define WSF_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup WSF_SPI_API
 *  \{ */


/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Callback Function Datatypes
**************************************************************************************************/

/*! \brief    SPI Rx callback. */
typedef void (*WsfSpiRxCback_t)(uint8_t* rxBuf);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the target SPI.
 *
 *  \param  pBuf    Tx Buffer pointer.
 *  \param  size    Length of buffer.
 *
 *  \return memory used.
 */
/*************************************************************************************************/
int32_t WsfSpiInit(void *pBuf, uint32_t size);

/*************************************************************************************************/
/*!
 *  \brief     Register the target SPI RX callback.
 *
 *  \param[in] Callback function for SPI RX.
 */
/*************************************************************************************************/
void WsfSpiRegister(WsfSpiRxCback_t rxCback);

/*************************************************************************************************/
/*!
 *  \brief  Transmit buffer on target SPI.
 *
 *  \param  pBuf    Buffer to transmit.
 *  \param  len     Length of buffer in octets.
 */
/*************************************************************************************************/
bool_t WsfSpiWrite(const uint8_t *pBuf, uint32_t len);

/*! \} */    /* WSF_SPI_API */

#ifdef __cplusplus
};
#endif

#endif /* WSF_SPI_H */
