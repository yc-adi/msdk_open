/******************************************************************************
 * Copyright (C) 2023 Maxim Integrated Products, Inc., All Rights Reserved.
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

/*
 * @file    main.c
 * @brief   Demonstrates the various low power modes.
 *
 * @details Iterates through the various low power modes, using either the RTC
 *          alarm or a GPIO to wake from each.  #defines determine which wakeup
 *          source to use.  Once the code is running, you can measure the
 *          current used on the VCORE rail.
 *
 *          The power states shown are:
 *            1. Active mode power with all clocks on
 *            2. Active mode power with peripheral clocks disabled
 *            3. Active mode power with unused RAMs shut down
 *            4. SLEEP mode
 *            5. LPM mode
 *            6. UPM mode
 *            7. BACKUP mode
 *            8. STANDBY mode
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mxc_delay.h"
#include "mxc_device.h"
#include "mxc_errors.h"
#include "pb.h"
#include "led.h"
#include "lp.h"
#include "icc.h"
#include "rtc.h"
#include "spi.h"
#include "spi_regs.h"
#include "uart.h"
#include "nvic_table.h"

#define DELAY_IN_SEC 5
#define USE_CONSOLE 1

#define USE_BUTTON 0
#define USE_ALARM 1

#define DO_SLEEP 0
#define DO_LPM 0
#define DO_UPM 0
#define DO_BACKUP 0
#define DO_STANDBY 1

#if (!(USE_BUTTON || USE_ALARM))
#error "You must set either USE_BUTTON or USE_ALARM to 1."
#endif
#if (USE_BUTTON && USE_ALARM)
#error "You must select either USE_BUTTON or USE_ALARM, not both."
#endif

#if (DO_BACKUP && DO_STANDBY)
#error "You must select either DO_BACKUP or DO_STANDBY or neither, not both."
#endif

#if USE_CONSOLE
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

#define SPI_BUF_LEN     2
#define SPI_DATA_SIZE   16
#define SPI_SPEED       40000000 // Bit Rate
#define SPI             MXC_SPI0
#define SPI_IRQ         SPI0_IRQn

uint16_t spi_rx_data[SPI_BUF_LEN];
volatile mxc_spi_regs_t *spi_reg = SPI;
mxc_spi_req_t spi_req;
int spiRxReady = 0;

bool wakeupByTimer = false;

// *****************************************************************************

#if USE_ALARM
volatile int alarmed;
void alarmHandler(void)
{
    int flags = MXC_RTC->ctrl;
    alarmed = 1;

    if ((flags & MXC_F_RTC_CTRL_SSEC_ALARM) >> MXC_F_RTC_CTRL_SSEC_ALARM_POS) {
        MXC_RTC->ctrl &= ~(MXC_F_RTC_CTRL_SSEC_ALARM);
    }

    if ((flags & MXC_F_RTC_CTRL_TOD_ALARM) >> MXC_F_RTC_CTRL_TOD_ALARM_POS) {
        MXC_RTC->ctrl &= ~(MXC_F_RTC_CTRL_TOD_ALARM);
    }
}

void setTrigger(int waitForTrigger)
{
    alarmed = 0;

    while (MXC_RTC_Init(0, 0) == E_BUSY) {}

    while (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}

    while (MXC_RTC_SetTimeofdayAlarm(DELAY_IN_SEC) == E_BUSY) {}

    while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}

    while (MXC_RTC_Start() == E_BUSY) {}

    if (waitForTrigger) {
        while (!alarmed) {}
    }

    // Wait for serial transactions to complete.
#if USE_CONSOLE

    while (MXC_UART_ReadyForSleep(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {}

#endif // USE_CONSOLE
}
#endif // USE_ALARM

#if USE_BUTTON
volatile int buttonPressed;
void buttonHandler(void *pb)
{
    buttonPressed = 1;
}

void setTrigger(int waitForTrigger)
{
    volatile int tmp;

    buttonPressed = 0;

    if (waitForTrigger) {
        while (!buttonPressed) {}
    }

    // Debounce the button press.
    for (tmp = 0; tmp < 0x100000; tmp++) {
        __NOP();
    }

    // Wait for serial transactions to complete.
#if USE_CONSOLE

    while (MXC_UART_ReadyForSleep(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {}

#endif // USE_CONSOLE
}
#endif // USE_BUTTON

void button_wakeup(void)
{
    PRINT("\n******\nButton wake up example. VER 1.\n******\n\n");
    PRINT("Wait 5 secs.\n");
    LED_On(LED_GREEN);
    MXC_Delay(5000000);  // avoid brick
    LED_Off(LED_GREEN);

    // Configure and enable interrupt
    unsigned int pb = 0;
    MXC_GPIO_IntConfig(&pb_pin[pb], MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(pb_pin[pb].port, pb_pin[pb].mask);
    NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(pb_pin[pb].port)));

    /// enable button wakeup
    MXC_LP_EnableGPIOWakeup((mxc_gpio_cfg_t *)&pb_pin[0]);

    MXC_GPIO_SetWakeEn(pb_pin[0].port, pb_pin[0].mask);

    while (1)
    {
        PRINT("Running in ACTIVE mode for 5 secs.\n");
        MXC_Delay(5000000);

        PRINT("Entering STANDBY mode.\n");
        MXC_Delay(1000);

        MXC_LP_EnterStandbyMode();
        
        PRINT("Wake up from STANDBY mode.\n\n");
        MXC_Delay(1000);
    }
}

void tmrHandler(void)
{
    int flags = MXC_RTC->ctrl;

    if ((flags & MXC_F_RTC_CTRL_SSEC_ALARM) >> MXC_F_RTC_CTRL_SSEC_ALARM_POS) {
        MXC_RTC->ctrl &= ~(MXC_F_RTC_CTRL_SSEC_ALARM);
    }

    if ((flags & MXC_F_RTC_CTRL_TOD_ALARM) >> MXC_F_RTC_CTRL_TOD_ALARM_POS) {
        MXC_RTC->ctrl &= ~(MXC_F_RTC_CTRL_TOD_ALARM);
    }

    wakeupByTimer = true;
}

void timer_wakeup(void)
{
    PRINT("\n******\nTimer wake up example. VER 1.\n******\n\n");
    PRINT("Wait 5 secs.\n");
    LED_On(LED_GREEN);
    MXC_Delay(5000000);  // avoid brick
    LED_Off(LED_GREEN);

    MXC_NVIC_SetVector(RTC_IRQn, tmrHandler);
    /* MXC_NVIC_SetVector()
    int index = irqn + 16; // offset for externals

    // If not copied, do copy
    if (SCB->VTOR != (uint32_t)&ramVectorTable) {
        NVIC_SetRAM();
    }

    ramVectorTable[index] = irq_handler;
    NVIC_EnableIRQ(irqn);
    */

    // enable RTC wakeup
    MXC_GCR->pm |= MXC_F_GCR_PM_RTC_WE;
    
    MXC_RTC_SetTimeofdayAlarm(DELAY_IN_SEC);
    
    while (1)
    {
        PRINT("Running in ACTIVE mode for 5 secs.\n");
        MXC_Delay(5000000);

        PRINT("Set the timer then enter STANDBY mode for %d secs.\n", DELAY_IN_SEC);
        MXC_Delay(1000);

        // set the timer
        MXC_RTC_Init(0, 0);
        MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE);
        MXC_RTC_Start();

        MXC_LP_EnterStandbyMode();

        PRINT("Wake up from STANDBY mode.\n\n");
        MXC_Delay(1000);
    }
}

void button_and_timer_wakeup(void)
{
    PRINT("\n******\nButtion and timer wake up example. VER 1.\n******\n\n");
    PRINT("Wait 5 secs.\n");
    LED_On(LED_GREEN);
    MXC_Delay(5000000);  // avoid brick
    LED_Off(LED_GREEN);

    // Configure and enable interrupt for button
    unsigned int pb = 0;
    MXC_GPIO_IntConfig(&pb_pin[pb], MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(pb_pin[pb].port, pb_pin[pb].mask);
    NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(pb_pin[pb].port)));

    /// enable button wakeup
    MXC_LP_EnableGPIOWakeup((mxc_gpio_cfg_t *)&pb_pin[0]);
    MXC_GPIO_SetWakeEn(pb_pin[0].port, pb_pin[0].mask);

    // Configure Timer
    MXC_NVIC_SetVector(RTC_IRQn, tmrHandler);
    
    // enable RTC wakeup
    MXC_GCR->pm |= MXC_F_GCR_PM_RTC_WE;
    
    MXC_RTC_SetTimeofdayAlarm(DELAY_IN_SEC * 4);
    
    while (1)
    {
        PRINT("Running in ACTIVE mode for 5 secs.\n");
        MXC_Delay(5000000);

        PRINT("Set the timer then enter STANDBY mode for %d secs if no button press.\n", DELAY_IN_SEC * 4);
        wakeupByTimer = false;
        MXC_Delay(1000);

        // set the timer
        MXC_RTC_Init(0, 0);
        MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE);
        MXC_RTC_Start();

        MXC_LP_EnterStandbyMode();

        PRINT("Wake up by %s from STANDBY mode.\n\n", wakeupByTimer ? "Timer" : "Button");
        MXC_Delay(1000);
    }
}

void SPI_IRQHandler(void)
{
    MXC_SPI_AsyncHandler(SPI);

    spiRxReady = 2;
}

void spi_wakeup(void)
{
    PRINT("\n******\nSPI wake up example. VER 2.\n******\n");
    PRINT("Wait 5 secs.\n\n");
    LED_On(LED_GREEN);
    MXC_Delay(5000000);  // avoid brick
    LED_Off(LED_GREEN);

    // Configure SPI
    int spiInitRet;

    /// Init SPI Slave
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

    spiInitRet = MXC_SPI_Init(SPI, // spi regiseter
                              0, // master mode
                              0, // quad mode
                              1, // num slaves
                              1, // ss polarity
                              SPI_SPEED,
                              spi_pins);
    if (spiInitRet != E_NO_ERROR) {
        printf("SPI INITIALIZATION ERROR\n");
    }

    if (spiInitRet == E_NO_ERROR) {
        spiInitRet = MXC_SPI_SetDataSize(SPI, SPI_DATA_SIZE);
        if (spiInitRet != E_NO_ERROR) {
            MXC_SPI_Shutdown(SPI);            
            printf("SPI SET DATASIZE ERROR: %d\n", spiInitRet);
        }
    }

    if (spiInitRet == E_NO_ERROR) {
        spiInitRet = MXC_SPI_SetWidth(SPI, SPI_WIDTH_STANDARD);
        if (spiInitRet != E_NO_ERROR) {
            printf("\nSPI SET WIDTH ERROR: %d\n", spiInitRet);
        }
    }

    if (spiInitRet == E_NO_ERROR) {
        // SPI Request
        spi_req.spi = SPI;
        spi_req.txData = NULL;
        spi_req.rxData = (uint8_t *)spi_rx_data;
        spi_req.txLen = 0;
        spi_req.rxLen = SPI_BUF_LEN;
        spi_req.ssIdx = 0;
        spi_req.ssDeassert = 1;
        spi_req.txCnt = 0;
        spi_req.rxCnt = 0;
        spi_req.completeCB = NULL;

        //MXC_NVIC_SetVector(SPI_IRQ, SPI_IRQHandler);
        //NVIC_EnableIRQ(SPI_IRQ);

        memset(spi_rx_data, 0x0, SPI_BUF_LEN * sizeof(uint16_t));
    }

    printf("\nSPI SLAVE RX INIT: done\n");
    MXC_Delay(10000);

    /// enable SPI wakeup from both sides
    MXC_GCR->pm |= MXC_F_GCR_PM_GPIO_WE;
    spi_reg->wken |= MXC_F_SPI_WKEN_RX_FULL;

    while (1)
    {
        PRINT("Running in ACTIVE mode for 5 secs.\n");
        MXC_Delay(5000000);

        PRINT("Start SPI RX and enter STANDBY mode.\n");
        MXC_Delay(1000);

        spiRxReady = 0;
    
        spi_reg->wkfl = 0x000F;
        spi_reg->wken |= MXC_F_SPI_WKEN_RX_FULL;

        //MXC_SPI_SlaveTransactionAsync(&spi_req);
        //MXC_SPI_SlaveTransaction(&spi_req);
        MXC_SPI_SlaveRx(&spi_req);
        
        //MXC_LP_EnterLowPowerMode();
        //MXC_LP_EnterStandbyMode();
        
        MXC_Delay(5000000);

        PRINT("Waking up from standby mode, %d, %x, %04X %04X.\n\n", spiRxReady, spi_reg->wkfl, spi_rx_data[0], spi_rx_data[1]);
        MXC_Delay(1000);
    }
}

int main(void)
{
    //button_wakeup();

    //timer_wakeup();

    button_and_timer_wakeup();

    //spi_wakeup();

    PRINT("****Low Power Mode Example****\n\n");

#if USE_ALARM
    PRINT("This code cycles through the MAX32655 power modes, using the RTC alarm to exit from "
          "each mode.  The modes will change every %d seconds.\n\n",
          DELAY_IN_SEC);
    MXC_NVIC_SetVector(RTC_IRQn, alarmHandler);
#endif // USE_ALARM

#if USE_BUTTON
    PRINT("This code cycles through the MAX32655 power modes, using a push button (PB1) to exit "
          "from each mode and enter the next.\n\n");
    PB_RegisterCallback(0, buttonHandler);
#endif // USE_BUTTON

    while (1) {
        PRINT("Running in ACTIVE mode.\n");
    #if !USE_CONSOLE
        MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_UART0);
    #endif // !USE_CONSOLE
        setTrigger(1);

    #if USE_BUTTON
        MXC_LP_EnableGPIOWakeup((mxc_gpio_cfg_t *)&pb_pin[0]);
        MXC_GPIO_SetWakeEn(pb_pin[0].port, pb_pin[0].mask);
    #endif // USE_BUTTON
    #if USE_ALARM
        MXC_LP_EnableRTCAlarmWakeup();
    #endif // USE_ALARM

        GPIO_PrepForSleep();

#if DO_SLEEP
        PRINT("Entering SLEEP mode.\n");
        setTrigger(0);
        MXC_LP_EnterSleepMode();
        PRINT("Waking up from SLEEP mode.\n");
#endif // DO_SLEEP

#if DO_LPM
        PRINT("Entering LPM mode.\n");
        setTrigger(0);
        MXC_LP_EnterLowPowerMode();
        PRINT("Waking up from LPM mode.\n");
#endif // DO_LPM

#if DO_UPM
        PRINT("Entering UPM mode.\n");
        setTrigger(0);
        MXC_LP_EnterMicroPowerMode();
        PRINT("Waking up from UPM mode.\n");
#endif // DO_UPM

#if DO_BACKUP
        PRINT("Entering BACKUP mode.\n");
        setTrigger(0);
        MXC_LP_EnterBackupMode();
#endif // DO_BACKUP

#if DO_STANDBY
        PRINT("Entering STANDBY mode.\n");
        setTrigger(0);
        MXC_LP_EnterStandbyMode();
#endif // DO_STANDBY
    }
}