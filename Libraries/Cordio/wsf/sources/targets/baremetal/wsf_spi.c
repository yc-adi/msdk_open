/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  WSF SPI.
 *
 *  Copyright (c) 2013-2018 Arm Ltd. All Rights Reserved.
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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "wsf_cs.h"
#include "wsf_os.h"
#include "wsf_spi.h"
#include "wsf_trace.h"
#include "wsf_types.h"

#include "pal_uart.h"

#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_pins.h"
#include "nvic_table.h"
#include "spi.h"
#include "uart.h"

/**************************************************************************************************
  Defines
**************************************************************************************************/
#define DATA_LEN        2
#define DATA_SIZE       16
#define SPI_SPEED       100000 // Bit Rate

#define SPI             MXC_SPI0
#define SPI_IRQ         SPI0_IRQn

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief    TX structure */
typedef struct
{
  uint8_t      *pBuf;                              /*!< SPI TX buffer pointer */
  uint16_t     size;                               /*!< SPI TX buffer size */
  uint16_t     in;                                 /*!< SPI TX buffer in index */
  uint16_t     out;                                /*!< SPI TX buffer out index */
  uint16_t     crt;                                /*!< SPI TX current number of bytes sent */
} WsfSpiTx_t;

/*! \brief    RX structure */
typedef struct
{
  WsfSpiRxCback_t cback;                          /*!< SPI RX callback. */
  uint8_t         *pBuf;                          /*!< SPI RX buffer pointer */
  uint16_t        size;                           /*!< SPI RX buffer size */
} WsfSpiRx_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief  SPI Control block. */
static struct
{
  WsfSpiTx_t         tx;             /*!< target SPI TX structure */
  WsfSpiRx_t         rx;             /*!< target SPI RX structure */
  bool_t             initialized;    /*!< SPI RX is initialized */
} WsfSpiCb = {0};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

mxc_spi_req_t spi_req;
volatile int spi_flag = 0;  // 0: no data received, 1: RX data ready

/**************************************************************************************************
  Local Functions
**************************************************************************************************/
void wsfSpiRxHandler(void);

/*************************************************************************************************/
/*!
 *  \brief  SPI ISR.
 *
 */
/*************************************************************************************************/
__attribute__((weak)) void SPI_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(SPI);

    wsfSpiRxHandler();

    spi_flag = 1;
}


/*************************************************************************************************/
/*!
 *  \brief  SPI Rx handler
 */
/*************************************************************************************************/
void wsfSpiRxHandler(void)
{
  if (WsfSpiCb.rx.cback)
  {
    WsfSpiCb.rx.cback(WsfSpiCb.rx.pBuf);  // SpiRx
  }
}


/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the SPI.
 *
 *  \param  pBuf    Tx Buffer pointer.
 *  \param  size    Length of buffer.
 *
 *  \return memory used.
 */
/*************************************************************************************************/
int32_t WsfSpiInit(void *pBuf, uint32_t size)
{
  /* Skip initialization if it is already done. */
  if (WsfSpiCb.initialized)
  {
    return 0;
  }

  WsfSpiCb.tx.pBuf = (uint8_t *)pBuf;
  WsfSpiCb.tx.size = size / 2;

  WsfSpiCb.rx.pBuf = (uint8_t *)(pBuf + size / 2);
  WsfSpiCb.rx.size = size / 2;

  /// SPI init
  int retVal;
  mxc_spi_pins_t spi_pins;

  spi_pins.clock = true;
  spi_pins.miso = true;
  spi_pins.mosi = true;
  spi_pins.ss0 = true;
  spi_pins.ss1 = false;
  spi_pins.ss2 = false;
  spi_pins.sdio2 = false;
  spi_pins.sdio3 = false;
  spi_pins.vddioh = false;
  
  //MXC_Delay(100000);  // without this delay, MXC_SPI_SetDataSize will fail

  retVal = MXC_SPI_Init(SPI, // spi regiseter
                        0, // master mode
                        0, // quad mode
                        1, // num slaves
                        1, // ss polarity
                        SPI_SPEED,
                        spi_pins);
  if (retVal != E_NO_ERROR) {
    printf("\nSPI INITIALIZATION ERROR\n");
    return -1;
  }
  
  //MXC_Delay(100000);  // without this delay, MXC_SPI_SetDataSize will fail

  retVal = MXC_SPI_SetDataSize(SPI, DATA_SIZE);
  if (retVal != E_NO_ERROR) {
    printf("\nSPI SET DATASIZE ERROR: %d\n", retVal);
    return -2;
  }

  retVal = MXC_SPI_SetWidth(SPI, SPI_WIDTH_STANDARD);
  if (retVal != E_NO_ERROR) {
    printf("\nSPI SET WIDTH ERROR: %d\n", retVal);
    return -3;
  }

  memset(WsfSpiCb.rx.pBuf, 0x0, DATA_LEN * sizeof(uint16_t));

  // SPI Request
  spi_req.spi = SPI;
  spi_req.txData = NULL;
  spi_req.rxData = (uint8_t *)WsfSpiCb.rx.pBuf;
  spi_req.txLen = 0;
  spi_req.rxLen = DATA_LEN;
  spi_req.ssIdx = 0;
  spi_req.ssDeassert = 1;
  spi_req.txCnt = 0;
  spi_req.rxCnt = 0;
  spi_req.completeCB = NULL;

  MXC_NVIC_SetVector(SPI_IRQ, SPI_IRQHandler);
  NVIC_EnableIRQ(SPI_IRQ);

  /* Start SPI RX. */
  spi_flag = 0;
  MXC_SPI_SlaveTransactionAsync(&spi_req);

  WsfSpiCb.initialized = TRUE;
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief  Register the SPI RX callback.
 *
 *  \param  rxCback  Callback function for SPI RX.
 */
/*************************************************************************************************/
void WsfSpiRegister(WsfSpiRxCback_t rxCback)
{
  if (rxCback != NULL)
  {
    WsfSpiCb.rx.cback = rxCback;
  }
}
