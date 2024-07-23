/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Maxim Custom service server.
 *
 *  Copyright (c) 2012-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2022-2023 Analog Devices, Inc.
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

#include <stdbool.h>
#include "mcs_api.h"
#include "app_ui.h"
#include "pal_led.h"
#include "i2c.h"
#include "mxc_delay.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define PMIC_LED_BLUE	    0x1
#define PMIC_LED_RED		0x2
#define PMIC_LED_GREEN		0x4

#define PMIC_I2C      	    MXC_I2C1
#define I2C_FREQ            100000
#define PMIC_SLAVE_ADDR     (0x28)
#define INIT_LEN            2
#define LED_SET_LEN         2
#define LED_CFG_REG_ADDR    0x2C
#define LED_SET_REG_ADDR    0x2D

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! \brief Connection control block */
typedef struct {
    dmConnId_t connId; /*! \brief Connection ID */
    bool_t mcsToSend; /*! \brief mcs measurement ready to be sent on this channel */
    uint8_t sentMcsBtnState; /*! \brief value of last sent mcs button state */
} mcsConn_t;

/*! \brief Control block */
static struct {
    mcsConn_t conn[DM_CONN_MAX]; /*! \brief connection control block */
    wsfTimer_t btnStateChkTimer; /*! \brief periodic check timer */
    mcsCfg_t cfg; /*! \brief configurable parameters */
    uint16_t currCount; /*! \brief current measurement period count */
    bool_t txReady; /*! \brief TRUE if ready to send notifications */
    uint8_t btnState; /*! \brief value of last button state */
} mcsCb;

/*************************************************************************************************/
/*!
 *  \brief  Initialize PMIC for LEDs.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void pmicI2CInit(void)
{
	int error;
	uint8_t tx_buf[INIT_LEN];

	MXC_Delay(MXC_DELAY_MSEC(500)); 		//Wait for PMIC to power-up

    //Setup the I2CM
    error = MXC_I2C_Init(PMIC_I2C, 1, 0);
    if (error != E_NO_ERROR) {
        while(1);
    }

    MXC_I2C_SetFrequency(PMIC_I2C, I2C_FREQ);

    // Set the LED current strength
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = PMIC_I2C;
    reqMaster.addr = PMIC_SLAVE_ADDR;
    reqMaster.tx_buf = tx_buf;
    reqMaster.tx_len = INIT_LEN;
    reqMaster.rx_buf = NULL;
    reqMaster.rx_len = 0;
    reqMaster.restart = 0;

    tx_buf[0] = LED_CFG_REG_ADDR;
    tx_buf[1] = 1;					//Set LED current to 1mA

    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        // Insert error handling here.
    }   
}

/*************************************************************************************************/
/*!
 *  \brief  Sets the color of the LED.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void setLED(uint8_t color, uint8_t state) {
    int error;
    uint8_t tx_buf[LED_SET_LEN];

    // Set the LED Color
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = PMIC_I2C;
    reqMaster.addr = PMIC_SLAVE_ADDR;
    reqMaster.tx_buf = tx_buf;
    reqMaster.tx_len = LED_SET_LEN;
    reqMaster.rx_buf = NULL;
    reqMaster.rx_len = 0;
    reqMaster.restart = 0;

    switch(color) {
        case PMIC_LED_BLUE:
            tx_buf[0] = LED_SET_REG_ADDR; // 0x2D
            tx_buf[1] = (uint8_t) (state ? (1 << 5) : 0);		//Set Blue LED?
            break;
        
        case PMIC_LED_RED:
            tx_buf[0] = LED_SET_REG_ADDR + 1; // 0x2E
            tx_buf[1] = (uint8_t) (state ? (1 << 5) : 0);		//Set Red LED?
            break;
        
        case PMIC_LED_GREEN:
            tx_buf[0] = LED_SET_REG_ADDR + 2; // 0x2F
            tx_buf[1] = (uint8_t) (state ? (1 << 5) : 0);		//Set Green LED?
            break;
        
        default:
            // Insert error handling here.
            break;
    }

    // TODO: Update pal_twi and don't make this blocking.
    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        // Insert error handling here.
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Return TRUE if no connections with active measurements.
 *
 *  \return TRUE if no connections active.
 */
/*************************************************************************************************/
static bool_t mcsNoConnActive(void)
{
    mcsConn_t *pConn = mcsCb.conn;
    uint8_t i;

    for (i = 0; i < DM_CONN_MAX; i++, pConn++) {
        if (pConn->connId != DM_CONN_ID_NONE) {
            return FALSE;
        }
    }
    return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  ATTS write callback for maxim custom service.  Use this function as a parameter
 *          to SvcMcsCbackRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t McsWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation, uint16_t offset,
                      uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
    AttsSetAttr(handle, sizeof(*pValue), (uint8_t *)pValue);
    /* Turn LED on if non-zero value was written */
    bool on = *pValue != 0;

    /* Get LED ID */
    switch (handle) {
    case MCS_R_HDL:
        setLED(PMIC_LED_RED, on);
        break;
    case MCS_B_HDL:
        setLED(PMIC_LED_BLUE, on);
        break;
    case MCS_G_HDL:
        setLED(PMIC_LED_GREEN, on);
        break;
    }

    return ATT_SUCCESS;
}

/*************************************************************************************************/

/*!
 *  \brief  Setting characteristic value and send the button value to the peer device
 *
 *  \return None.
 */
/*************************************************************************************************/
void McsSetFeatures(uint8_t features)
{
    AttsSetAttr(MCS_BUTTON_HDL, sizeof(features),
                (uint8_t *)&features); /*Setting mcsButtonVal characteristic value */
    dmConnId_t connId = AppConnIsOpen(); /*Getting connected */
    if (connId != DM_CONN_ID_NONE) {
        AttsHandleValueNtf(connId, MCS_BUTTON_HDL, sizeof(features),
                           (uint8_t *)&features); /*Send notify */
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize the mcs server.
 *
 *  \param  handerId    WSF handler ID of the application using this service.
 *  \param  pCfg        mcs configurable parameters.
 *
 *  \return None.
 */
/*************************************************************************************************/
void McsInit(wsfHandlerId_t handlerId, mcsCfg_t *pCfg)
{
    mcsCb.btnStateChkTimer.handlerId = handlerId;
    mcsCb.cfg = *pCfg;

    pmicI2CInit();

    /* De-init the PAL LEDs so we can control them here */
    PalLedDeInit();
}

/*************************************************************************************************/
/*!
 *  \brief  Start periodic mcs button state check.  This function starts a timer to perform
 *          periodic button checks.
 *
 *  \param  connId      DM connection identifier.
 *  \param  timerEvt    WSF event designated by the application for the timer.
 *  \param  mcsCccIdx   Index of mcs button state CCC descriptor in CCC descriptor handle table.
 *
 *  \return None.
 */
/*************************************************************************************************/
void McsButtonCheckStart(dmConnId_t connId, uint8_t timerEvt, uint8_t mcsCccIdx, uint8_t btnState)
{
    /* if this is first connection */
    if (mcsNoConnActive()) {
        /* initialize control block */
        mcsCb.btnStateChkTimer.msg.event = timerEvt;
        mcsCb.btnStateChkTimer.msg.status = mcsCccIdx;
        mcsCb.btnState = btnState;
        mcsCb.currCount = mcsCb.cfg.count;

        /* start timer */
        WsfTimerStartSec(&mcsCb.btnStateChkTimer, mcsCb.cfg.period);
    }

    /* set conn id and last sent button level */
    mcsCb.conn[connId - 1].connId = connId;
    mcsCb.conn[connId - 1].sentMcsBtnState = btnState;
}

/*************************************************************************************************/
/*!
 *  \brief  Stop periodic button state check.
 *
 *  \param  connId      DM connection identifier.
 *
 *  \return None.
 */
/*************************************************************************************************/
void McsButtonCheckStop(dmConnId_t connId)
{
    /* clear connection */
    mcsCb.conn[connId - 1].connId = DM_CONN_ID_NONE;
    mcsCb.conn[connId - 1].mcsToSend = FALSE;

    /* if no remaining connections */
    if (mcsNoConnActive()) {
        /* stop timer */
        WsfTimerStop(&mcsCb.btnStateChkTimer);
    }
}
