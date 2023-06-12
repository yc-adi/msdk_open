/**
 * @file    main.c
 * @brief   slave SPI RX Demo
 * @details Shows how slave receives data from master by SPI
 */

/******************************************************************************
 * Copyright (C) 2023 Analog Devices, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/

/***** Includes *****/
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_pins.h"
#include "nvic_table.h"
#include "uart.h"
#include "spi.h"
#include "dma.h"


/***** Definitions *****/
#define DATA_LEN        2
#define DATA_SIZE       16
#define SPI_SPEED       100000 // Bit Rate

#define SPI             MXC_SPI0
#define SPI_IRQ         SPI0_IRQn


/***** Globals *****/
uint16_t rx_data[DATA_LEN];
//uint16_t tx_data[DATA_LEN];
volatile int SPI_FLAG;


/***** Functions *****/
void SPI_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(SPI);

    SPI_FLAG = 0; // let the main loop run
}


int main(void)
{
    int cnt = 0, retVal;
    mxc_spi_req_t req;
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

    printf("\n**************************** SPI SLAVE RX TEST *************************\n");
    
    retVal = MXC_SPI_Init(SPI, // spi regiseter
                            0, // master mode
                            0, // quad mode
                            1, // num slaves
                            1, // ss polarity
                            SPI_SPEED,
                            spi_pins);
    if (retVal != E_NO_ERROR) {
        printf("\nSPI INITIALIZATION ERROR\n");
        return retVal;
    }

    retVal = MXC_SPI_SetDataSize(SPI, DATA_SIZE);
    if (retVal != E_NO_ERROR) {
        printf("\nSPI SET DATASIZE ERROR: %d\n", retVal);
        return retVal;
    }

    retVal = MXC_SPI_SetWidth(SPI, SPI_WIDTH_STANDARD);
    if (retVal != E_NO_ERROR) {
        printf("\nSPI SET WIDTH ERROR: %d\n", retVal);
        return retVal;
    }

    memset(rx_data, 0x0, DATA_LEN * sizeof(uint16_t));

    // SPI Request
    req.spi = SPI;
    req.txData = NULL;
    req.rxData = (uint8_t *)rx_data;
    req.txLen = 0;
    req.rxLen = DATA_LEN;
    req.ssIdx = 0;
    req.ssDeassert = 1;
    req.txCnt = 0;
    req.rxCnt = 0;
    req.completeCB = NULL;

    MXC_NVIC_SetVector(SPI_IRQ, SPI_IRQHandler);
    NVIC_EnableIRQ(SPI_IRQ);

    while (1)
    {
        SPI_FLAG = 1;
        MXC_SPI_SlaveTransactionAsync(&req);
        while (SPI_FLAG) {}
        
        printf("\n%d rx: 0x%04X 0x%04X", ++cnt, rx_data[0], rx_data[1]);

        MXC_Delay(100000);
    }

    return E_NO_ERROR;
}
