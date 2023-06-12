/*************************************************************************************************/
/*!
 *  \file       spi.c
 *
 *  \brief      SPI utilities.
 *
 *  Copyright (c) 2015-2018 Arm Ltd. All Rights Reserved.
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wsf_types.h"

#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_os.h"
#include "wsf_spi.h"
#include "util/print.h"

#include "util/bstream.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief  SPI events. */
enum
{
  SPI_EVT_RX = (1 << 0)
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief   Control block for SPI. */
typedef struct
{
  wsfHandlerId_t handlerId;                              /*!< Handler ID for SpiHandler(). */
} spiCtrlBlk_t;

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief   Control block for SPI. */
static spiCtrlBlk_t spiCb;

/*************************************************************************************************/
/*!
 *  \brief  Initialize SPI.
 *
 *  \param  handlerId   Handler ID for SpiHandler().
 */
/*************************************************************************************************/
void SpiInit(wsfHandlerId_t handlerId)
{
  APP_TRACE_INFO0("SPI: init");

  spiCb.handlerId = handlerId;
}

/*************************************************************************************************/
/*!
 *  \brief  Handler for SPI messages.
 *
 *  \param  event       WSF event mask.
 *  \param  pMsg        WSF message.
 */
/*************************************************************************************************/
void SpiHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  /* Unused parameters */
  (void)pMsg;

  if ((event & SPI_EVT_RX) != 0)
  {
    APP_TRACE_INFO0("SpiHandler: process SPI_EVT_RX");
    APP_TRACE_INFO0("TODO: process the SPI received data");
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Called by application when data from SPI is received.
 *
 *  \param  dataByte  Received byte.
 */
/*************************************************************************************************/
void SpiRx(uint8_t* rxBuf)
{
  /// TODO: SpiRx()
  APP_TRACE_INFO0("SpiRx: set WSF event SPI_EVT_RX");
  WsfSetEvent(spiCb.handlerId, SPI_EVT_RX);
}
