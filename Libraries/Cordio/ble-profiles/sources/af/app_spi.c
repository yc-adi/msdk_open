/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      SPI handler.
 *
 *  Copyright (c) 2015-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
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

#include "util/terminal.h"
#include "app_ui.h"
#include "util/print.h"

#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_spi.h"
#include "util/bstream.h"

#include "dm_api.h"

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/

/**************************************************************************************************
  Extern Functions
**************************************************************************************************/
extern void SpiRx(uint8_t* rxBuf);
extern void SpiHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);
extern void SpiInit(wsfHandlerId_t handlerId);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize SPI.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppSpiInit(void)
{
  wsfHandlerId_t handlerSpiId;

  /* Initialize Serial Communication. */
  APP_TRACE_INFO0("register WsfSpiCb.rx.cback = SpiRx");
  WsfSpiRegister(SpiRx);  // low level

  handlerSpiId = WsfOsSetNextHandler(SpiHandler);
  APP_TRACE_INFO1("register WSF OS SpiHandler as id: %d, which will be triggered by SpiRx().", handlerSpiId);  // WSF OS level: process event and msg

  SpiInit(handlerSpiId);
}