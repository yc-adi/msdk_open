/*******************************************************************************
 * Copyright (C) 2018 Maxim Integrated Products, Inc., All Rights Reserved.
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
 * $Date: 2021-08-03 10:16:23 -0500 (Tue, 03 Aug 2021) $
 * $Revision: 58927 $
 *
 ******************************************************************************/

#ifndef WAKEUP_H
#define WAKEUP_H

#define US_TO_BBTICKS(x)    (((x)*(BB_CLK_RATE_HZ/100000)+9)/10)

#if (BACKUP_MODE==1)
#define DS_WAKEUP_TIME_US   (2500)
#else
#define DS_WAKEUP_TIME_US   (1850)
#endif
#define WAKEUP_TIME_US      (5)
#define MIN_DEEPSLEEP_US    (5000)
#define MIN_SLEEP_US        (10)

#ifdef ENABLE_SDMA
#define RUN_VOLTAGE             950
#else
#define RUN_VOLTAGE             910
#endif

#define BG_VOLTAGE              950
#define DS_VOLTAGE              810

/* Enter backup mode instead of deep sleep */
#ifndef BACKUP_MODE
#define BACKUP_MODE             0
#endif

void WUT_SetInt(uint32_t sleep_time);
void WUT_SetWakeup(uint32_t sleep_time);
uint32_t GetWakeDelay(uint32_t wait_ticks);
void DisableUnused(void);
void EnterDeepsleep(void);
void EnterDeepsleepSDMA(void);
void ExitDeepsleep(void);
void ExitDeepsleepSDMA(void);
void EnterBackground(void);
void ExitBackground(void);
void switchToHIRC(void);

#endif
